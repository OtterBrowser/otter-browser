/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "HtmlBookmarksImporter.h"
#include "../../../core/BookmarksManager.h"

#include <QtCore/QDir>
//TODO QtWebKit should not be used directly
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElementCollection>

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent) : BookmarksImporter(parent),
	m_file(NULL),
	m_optionsWidget(NULL)
{
}

HtmlBookmarksImporter::~HtmlBookmarksImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

void HtmlBookmarksImporter::handleOptions()
{
	if (!m_optionsWidget)
	{
		setImportFolder(BookmarksManager::getModel()->getRootItem());

		return;
	}

	if (m_optionsWidget->hasToRemoveExisting())
	{
		removeAllBookmarks();

		if (m_optionsWidget->isImportingIntoSubfolder())
		{
			BookmarksItem *folder = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), m_optionsWidget->getSubfolderName());

			setImportFolder(folder);
		}
		else
		{
			setImportFolder(BookmarksManager::getModel()->getRootItem());
		}
	}
	else
	{
		setAllowDuplicates(m_optionsWidget->allowDuplicates());
		setImportFolder(m_optionsWidget->getTargetFolder());
	}
}

void HtmlBookmarksImporter::processElement(const QWebElement &element)
{
	if (element.tagName().toLower() == QLatin1String("h3"))
	{
		BookmarksItem *bookmark = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), element.toPlainText(), getCurrentFolder());
		const QString keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		if (!BookmarksManager::hasKeyword(keyword))
		{
			bookmark->setData(keyword, BookmarksModel::KeywordRole);
		}

		if (!element.attribute(QLatin1String("ADD_DATE")).isEmpty())
		{
			const QDateTime time = QDateTime::fromTime_t(element.attribute(QLatin1String("ADD_DATE")).toUInt());

			bookmark->setData(time, BookmarksModel::TimeAddedRole);
			bookmark->setData(time, BookmarksModel::TimeModifiedRole);
		}

		setCurrentFolder(bookmark);
	}
	else if (element.tagName().toLower() == QLatin1String("a"))
	{
		const QUrl url(element.attribute(QLatin1String("href")));

		if (!allowDuplicates() && BookmarksManager::hasBookmark(url))
		{
			return;
		}

		BookmarksItem *bookmark = BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, url, element.toPlainText(), getCurrentFolder());
		const QString keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		if (!BookmarksManager::hasKeyword(keyword))
		{
			bookmark->setData(keyword, BookmarksModel::KeywordRole);
		}

		if (element.parent().nextSibling().tagName().toLower() == QLatin1String("dd"))
		{
			bookmark->setData(element.parent().nextSibling().toPlainText(), BookmarksModel::DescriptionRole);
		}

		if (!element.attribute(QLatin1String("ADD_DATE")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("ADD_DATE")).toUInt()), BookmarksModel::TimeAddedRole);
		}

		if (!element.attribute(QLatin1String("LAST_MODIFIED")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("LAST_MODIFIED")).toUInt()), BookmarksModel::TimeModifiedRole);
		}

		if (!element.attribute(QLatin1String("LAST_VISITED")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("LAST_VISITED")).toUInt()), BookmarksModel::TimeVisitedRole);
		}
	}
	else if (element.tagName().toLower() == QLatin1String("hr"))
	{
		BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, QUrl(), QString(), getCurrentFolder());
	}

	const QWebElementCollection descendants = element.findAll(QLatin1String("*"));

	for (int i = 0; i < descendants.count(); ++i)
	{
		if (descendants.at(i).parent() == element)
		{
			processElement(descendants.at(i));
		}
	}

	if (element.tagName().toLower() == QLatin1String("dl"))
	{
		goToParent();
	}
}

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

QString HtmlBookmarksImporter::getFileFilter() const
{
	return tr("HTML files (*.htm *.html)");
}

QString HtmlBookmarksImporter::getSuggestedPath() const
{
	return QString();
}

QString HtmlBookmarksImporter::getBrowser() const
{
	return QLatin1String("other");
}

bool HtmlBookmarksImporter::import()
{
	QWebPage page;

	handleOptions();

	page.mainFrame()->setHtml(m_file->readAll());

	processElement(page.mainFrame()->documentElement());

	return true;
}

bool HtmlBookmarksImporter::setPath(const QString &path)
{
	QString fileName = path;

	if (QFileInfo(path).isDir())
	{
		fileName = QDir(path).filePath(QLatin1String("bookmarks.html"));
	}

	if (m_file)
	{
		m_file->close();
		m_file->deleteLater();
	}

	m_file = new QFile(fileName, this);

	return m_file->open(QIODevice::ReadOnly);
}

}
