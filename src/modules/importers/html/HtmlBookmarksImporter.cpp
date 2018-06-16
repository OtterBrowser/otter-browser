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
#include "../../../ui/BookmarksImporterWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#ifdef OTTER_ENABLE_QTWEBKIT
//TODO QtWebKit should not be used directly
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElementCollection>
#endif

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent) : BookmarksImporter(parent),
	m_optionsWidget(nullptr),
	m_currentAmount(0),
	m_totalAmount(-1)
{
}

#ifdef OTTER_ENABLE_QTWEBKIT
void HtmlBookmarksImporter::processElement(const QWebElement &element)
{
	QWebElement entryElement(element.findFirst(QLatin1String("dt, hr")));

	while (!entryElement.isNull())
	{
		if (entryElement.tagName().toLower() == QLatin1String("hr"))
		{
			BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, {}, getCurrentFolder());

			++m_currentAmount;

			emit importProgress(BookmarksImport, m_totalAmount, m_currentAmount);
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
					type = (matchedElement.hasAttribute(QLatin1String("FEEDURL")) ? BookmarksModel::FeedBookmark : BookmarksModel::UrlBookmark);
				}
			}
			else
			{
				type = BookmarksModel::FolderBookmark;
			}

			if (type != BookmarksModel::UnknownBookmark && !matchedElement.isNull())
			{
				QMap<int, QVariant> metaData({{BookmarksModel::TitleRole, matchedElement.toPlainText()}});
				const bool isUrlBookmark(type == BookmarksModel::UrlBookmark || BookmarksModel::FeedBookmark);

				if (isUrlBookmark)
				{
					const QUrl url(matchedElement.attribute(QLatin1String("HREF")));

					if (!areDuplicatesAllowed() && BookmarksManager::hasBookmark(url))
					{
						entryElement = entryElement.nextSibling();

						continue;
					}

					metaData[BookmarksModel::UrlRole] = url;
				}

				if (matchedElement.hasAttribute(QLatin1String("SHORTCUTURL")))
				{
					const QString keyword(matchedElement.attribute(QLatin1String("SHORTCUTURL")));

					if (!keyword.isEmpty() && !BookmarksManager::hasKeyword(keyword))
					{
						metaData[BookmarksModel::KeywordRole] = keyword;
					}
				}

				if (matchedElement.hasAttribute(QLatin1String("ADD_DATE")))
				{
					const QDateTime dateTime(getDateTime(matchedElement, QLatin1String("ADD_DATE")));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeAddedRole] = dateTime;
						metaData[BookmarksModel::TimeModifiedRole] = dateTime;
					}
				}

				if (matchedElement.hasAttribute(QLatin1String("LAST_MODIFIED")))
				{
					const QDateTime dateTime(getDateTime(matchedElement, QLatin1String("LAST_MODIFIED")));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeModifiedRole] = dateTime;
					}
				}

				if (isUrlBookmark && matchedElement.hasAttribute(QLatin1String("LAST_VISITED")))
				{
					const QDateTime dateTime(getDateTime(matchedElement, QLatin1String("LAST_VISITED")));

					if (dateTime.isValid())
					{
						metaData[BookmarksModel::TimeVisitedRole] = dateTime;
					}
				}

				BookmarksModel::Bookmark *bookmark(BookmarksManager::addBookmark(type, metaData, getCurrentFolder()));

				++m_currentAmount;

				emit importProgress(BookmarksImport, m_totalAmount, m_currentAmount);

				if (type == BookmarksModel::FolderBookmark)
				{
					setCurrentFolder(bookmark);
					processElement(entryElement);
				}

				if (entryElement.nextSibling().tagName().toLower() == QLatin1String("dd"))
				{
					bookmark->setItemData(entryElement.nextSibling().toPlainText(), BookmarksModel::DescriptionRole);

					entryElement = entryElement.nextSibling();
				}
			}
		}

		entryElement = entryElement.nextSibling();
	}

	goToParent();
}
#endif

QWidget* HtmlBookmarksImporter::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImporterWidget(parent);
	}

	return m_optionsWidget;
}

QString HtmlBookmarksImporter::getTitle() const
{
	return tr("HTML Bookmarks");
}

QString HtmlBookmarksImporter::getDescription() const
{
	return tr("Imports bookmarks from HTML file (Netscape format).");
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

#ifdef OTTER_ENABLE_QTWEBKIT
QDateTime HtmlBookmarksImporter::getDateTime(const QWebElement &element, const QString &attribute)
{
#if QT_VERSION < 0x050800
	const uint seconds(element.attribute(attribute).toUInt());

	return ((seconds > 0) ? QDateTime::fromTime_t(seconds) : QDateTime());
#else
	const qint64 seconds(element.attribute(attribute).toLongLong());

	return ((seconds != 0) ? QDateTime::fromSecsSinceEpoch(seconds) : QDateTime());
#endif
}
#endif

QStringList HtmlBookmarksImporter::getFileFilters() const
{
	return {tr("HTML files (*.htm *.html)")};
}

bool HtmlBookmarksImporter::import(const QString &path)
{
#ifdef OTTER_ENABLE_QTWEBKIT
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		emit importFinished(BookmarksImport, FailedImport, 0);

		return false;
	}

	if (m_optionsWidget)
	{
		if (m_optionsWidget->hasToRemoveExisting())
		{
			removeAllBookmarks();

			if (m_optionsWidget->isImportingIntoSubfolder())
			{
				setImportFolder(BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, m_optionsWidget->getSubfolderName()}}));
			}
		}
		else
		{
			setAllowDuplicates(m_optionsWidget->areDuplicatesAllowed());
			setImportFolder(m_optionsWidget->getTargetFolder());
		}
	}

	QWebPage page;
	page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	page.mainFrame()->setHtml(file.readAll());

	m_totalAmount = page.mainFrame()->findAllElements(QLatin1String("dt, hr")).count();

	emit importStarted(BookmarksImport, m_totalAmount);

	BookmarksManager::getModel()->beginImport(getImportFolder(), page.mainFrame()->findAllElements(QLatin1String("a[href]")).count(), page.mainFrame()->findAllElements(QLatin1String("a[shortcuturl]")).count());

	processElement(page.mainFrame()->documentElement().findFirst(QLatin1String("dl")));

	BookmarksManager::getModel()->endImport();

	emit importFinished(BookmarksImport, SuccessfullImport, m_totalAmount);

	file.close();

	return true;
#else
	return false;
#endif
}

}
