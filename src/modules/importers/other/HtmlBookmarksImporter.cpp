/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

//TODO QtWebKit should not be used directly
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElementCollection>

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent): BookmarksImporter(parent),
	m_file(NULL),
	m_optionsWidget(new BookmarksImporterWidget),
	m_duplicate(true)
{
}

HtmlBookmarksImporter::~HtmlBookmarksImporter()
{
	delete m_optionsWidget;
}

void HtmlBookmarksImporter::handleOptions()
{
	if (m_optionsWidget->removeExisting())
	{
		removeAllBookmarks();

		if (m_optionsWidget->importIntoSubfolder())
		{
			BookmarkInformation *folder = new BookmarkInformation;
			folder->title = m_optionsWidget->subfolderName();
			folder->type = FolderBookmark;

			BookmarksManager::addBookmark(folder);
			setImportFolder(folder);
		}
		else
		{
			setImportFolder(BookmarksManager::getBookmark(0));
		}
	}
	else
	{
		setImportFolder(BookmarksManager::getBookmark(m_optionsWidget->targetFolder()));

		m_duplicate = m_optionsWidget->duplicate();
	}
}

void HtmlBookmarksImporter::processElement(QWebElement element)
{
	BookmarkInformation bookmark;
	uint time;

	if (element.tagName().toLower() == QLatin1String("a"))
	{
		bookmark.url = element.attribute(QLatin1String("href"));
		bookmark.title = element.toPlainText();
		bookmark.keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		if (element.parent().nextSibling().tagName().toLower() == QLatin1String("dd"))
		{
			bookmark.description = element.parent().nextSibling().toPlainText();
		}

		time = element.attribute(QLatin1String("ADD_DATE")).toUInt();

		if (time)
		{
			bookmark.added = QDateTime::fromTime_t(time);
		}

		time = element.attribute(QLatin1String("LAST_MODIFIED")).toUInt();

		if (time);
		{
			bookmark.modified= QDateTime::fromTime_t(time);
		}

		time = element.attribute(QLatin1String("LAST_VISITED")).toUInt();

		if (time)
		{
			bookmark.visited = QDateTime::fromTime_t(time);
		}

		addUrl(bookmark, m_duplicate);
	}
	else if (element.tagName().toLower() == QLatin1String("hr"))
	{
		addSeparator(bookmark);
	}
	else if (element.tagName().toLower() == QLatin1String("h3"))
	{
		bookmark.title = element.toPlainText();
		bookmark.keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		time = element.attribute(QLatin1String("ADD_DATE")).toUInt();

		if (time)
		{
			bookmark.added = QDateTime::fromTime_t(time);
		}

		time = element.attribute(QLatin1String("ADD_DATE")).toUInt();

		if (time)
		{
			bookmark.modified= QDateTime::fromTime_t(time);
		}

		enterNewFolder(bookmark);
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
	return m_optionsWidget;
}

QString HtmlBookmarksImporter::getTitle() const
{
	return QString(tr("HTML bookmarks"));
}

QString HtmlBookmarksImporter::getDescription() const
{
	return QString(tr("Gets bookmarks from HTML file. Many browsers are able to create such file."));
}

QString HtmlBookmarksImporter::getVersion() const
{
	return QLatin1String("1.0");
}

QString HtmlBookmarksImporter::getSuggestedPath() const
{
	return QString();
}

QString HtmlBookmarksImporter::getBrowser() const
{
	return QLatin1String("other");
}

bool HtmlBookmarksImporter::setPath(const QString &path, bool isPrefix)
{
	QUrl url(path);

	if (isPrefix)
	{
		url = url.resolved(QUrl(QLatin1String("bookmarks.html")));
	}

	if (m_file)
	{
		m_file->close();
		m_file->deleteLater();
	}

	m_file = new QFile(url.url(), this);

	return m_file->open(QIODevice::ReadOnly);
}

bool HtmlBookmarksImporter::import()
{
	QWebPage page;
	QWebFrame* frame = page.mainFrame();

	handleOptions();

	frame->setHtml(m_file->readAll());
	processElement(frame->documentElement());
}

}
