/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "HtmlBookmarksImporter.h"
#include "../../../core/BookmarksManager.h"

#include <QtCore/QDir>
#ifdef OTTER_ENABLE_QTWEBKIT
//TODO QtWebKit should not be used directly
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElementCollection>
#endif

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent) : BookmarksImporter(parent),
	m_optionsWidget(nullptr)
{
}

HtmlBookmarksImporter::~HtmlBookmarksImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

#ifdef OTTER_ENABLE_QTWEBKIT
void HtmlBookmarksImporter::processElement(const QWebElement &element)
{
	QWebElement entryElement(element.findFirst(QLatin1String("dt, hr")));

	while (!entryElement.isNull())
	{
		if (entryElement.tagName().toLower() == QLatin1String("hr"))
		{
			BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, QUrl(), QString(), getCurrentFolder());
		}
		else
		{
			BookmarksModel::BookmarkType type(BookmarksModel::UnknownBookmark);
			QWebElement matchedElement(entryElement.findFirst(QLatin1String("dt > h3")));

			if (matchedElement.isNull())
			{
				matchedElement = entryElement.findFirst(QLatin1String("dt > a"));

				if (!matchedElement.isNull())
				{
					type = BookmarksModel::UrlBookmark;
				}
			}
			else
			{
				type = BookmarksModel::FolderBookmark;
			}

			if (type != BookmarksModel::UnknownBookmark && !matchedElement.isNull())
			{
				const QString keyword(matchedElement.attribute(QLatin1String("SHORTCUTURL")));
				const QUrl url((type == BookmarksModel::UrlBookmark) ? matchedElement.attribute(QLatin1String("HREF")) : QUrl());

				if (type == BookmarksModel::UrlBookmark && !allowDuplicates() && BookmarksManager::hasBookmark(url))
				{
					entryElement = entryElement.nextSibling();

					continue;
				}

				BookmarksItem *bookmark(BookmarksManager::addBookmark(type, url, matchedElement.toPlainText(), getCurrentFolder()));

				if (!keyword.isEmpty() && !BookmarksManager::hasKeyword(keyword))
				{
					bookmark->setData(keyword, BookmarksModel::KeywordRole);
				}

				if (!matchedElement.attribute(QLatin1String("ADD_DATE")).isEmpty())
				{
					const QDateTime time(QDateTime::fromTime_t(matchedElement.attribute(QLatin1String("ADD_DATE")).toUInt()));

					bookmark->setData(time, BookmarksModel::TimeAddedRole);
					bookmark->setData(time, BookmarksModel::TimeModifiedRole);
				}

				if (!matchedElement.attribute(QLatin1String("ADD_DATE")).isEmpty())
				{
					bookmark->setData(QDateTime::fromTime_t(matchedElement.attribute(QLatin1String("ADD_DATE")).toUInt()), BookmarksModel::TimeAddedRole);
				}

				if (!matchedElement.attribute(QLatin1String("LAST_MODIFIED")).isEmpty())
				{
					bookmark->setData(QDateTime::fromTime_t(matchedElement.attribute(QLatin1String("LAST_MODIFIED")).toUInt()), BookmarksModel::TimeModifiedRole);
				}

				if (!matchedElement.attribute(QLatin1String("LAST_VISITED")).isEmpty())
				{
					bookmark->setData(QDateTime::fromTime_t(matchedElement.attribute(QLatin1String("LAST_VISITED")).toUInt()), BookmarksModel::TimeVisitedRole);
				}

				if (type == BookmarksModel::FolderBookmark)
				{
					setCurrentFolder(bookmark);
					processElement(entryElement);
				}

				if (entryElement.nextSibling().tagName().toLower() == QLatin1String("dd"))
				{
					bookmark->setData(entryElement.nextSibling().toPlainText(), BookmarksModel::DescriptionRole);

					entryElement = entryElement.nextSibling();
				}
			}
		}

		entryElement = entryElement.nextSibling();
	}

	goToParent();
}
#endif

QWidget* HtmlBookmarksImporter::getOptionsWidget()
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImporterWidget();
	}

	return m_optionsWidget;
}

QString HtmlBookmarksImporter::getTitle() const
{
	return QString(tr("HTML Bookmarks"));
}

QString HtmlBookmarksImporter::getDescription() const
{
	return QString(tr("Imports bookmarks from HTML file (Netscape format)."));
}

QString HtmlBookmarksImporter::getVersion() const
{
	return QLatin1String("1.0");
}

QString HtmlBookmarksImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("bookmarks.html"));
	}

	return path;
}

QString HtmlBookmarksImporter::getBrowser() const
{
	return QLatin1String("other");
}

QUrl HtmlBookmarksImporter::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList HtmlBookmarksImporter::getFileFilters() const
{
	return QStringList(tr("HTML files (*.htm *.html)"));
}

bool HtmlBookmarksImporter::import(const QString &path)
{
#ifdef OTTER_ENABLE_QTWEBKIT
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	if (m_optionsWidget)
	{
		if (m_optionsWidget->hasToRemoveExisting())
		{
			removeAllBookmarks();

			if (m_optionsWidget->isImportingIntoSubfolder())
			{
				setImportFolder(BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), m_optionsWidget->getSubfolderName()));
			}
		}
		else
		{
			setAllowDuplicates(m_optionsWidget->allowDuplicates());
			setImportFolder(m_optionsWidget->getTargetFolder());
		}
	}

	QWebPage page;
	page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	page.mainFrame()->setHtml(file.readAll());

	processElement(page.mainFrame()->documentElement().findFirst(QLatin1String("dl")));

	file.close();

	return true;
#else
	return false;
#endif
}

}
