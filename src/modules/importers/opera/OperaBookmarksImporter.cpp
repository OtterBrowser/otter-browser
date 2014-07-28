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

#include "OperaBookmarksImporter.h"

#include <QtCore/QTextStream>
#include <QtCore/QUrl>

namespace Otter
{

OperaBookmarksImporter::OperaBookmarksImporter(QObject *parent): BookmarksImporter(parent),
	m_file(NULL),
	m_optionsWidget(new BookmarksImporterWidget),
	m_duplicate(true)
{
}

OperaBookmarksImporter::~OperaBookmarksImporter()
{
	delete m_optionsWidget;
}

void OperaBookmarksImporter::handleOptions()
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

QWidget* OperaBookmarksImporter::getOptionsWidget()
{
	return m_optionsWidget;
}

QString OperaBookmarksImporter::getTitle() const
{
	return QString(tr("Opera bookmarks"));
}

QString OperaBookmarksImporter::getDescription() const
{
	return QString(tr("Imports bookmarks from Opera Browser version 12 or earlier"));
}

QString OperaBookmarksImporter::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaBookmarksImporter::getSuggestedPath() const
{
	return QString();
}

QString OperaBookmarksImporter::getBrowser() const
{
	return QLatin1String("opera");
}

bool OperaBookmarksImporter::setPath(const QString &path, bool isPrefix)
{
	QUrl url(path);

	if (isPrefix)
	{
		url = url.resolved(QUrl(QLatin1String("bookmarks.adr")));
	}

	if (m_file)
	{
		m_file->close();
		m_file->deleteLater();
	}

	m_file = new QFile(url.url(), this);

	return m_file->open(QIODevice::ReadOnly);
}

bool OperaBookmarksImporter::import()
{
	QTextStream stream(m_file);
	QString line;
	BookmarkInformation readedBookmark;
	OperaBookmarkEntry currentEntryType = NoOperaBookmarkEntry;
	bool isHeader = true;

	handleOptions();

	line = stream.readLine();

	if (line != QLatin1String("Opera Hotlist version 2.0"))
	{
		return false;
	}

	while (!stream.atEnd())
	{
		line = stream.readLine();

		if (isHeader && (line.isEmpty() || line.at(0) != QLatin1Char('#')))
		{
			continue;
		}

		isHeader = false;

		if (line.startsWith(QLatin1String("#URL")))
		{
			readedBookmark = BookmarkInformation();
			currentEntryType = UrlOperaBookmarkEntry;
		}
		else if (line.startsWith(QLatin1String("#FOLDER")))
		{
			readedBookmark = BookmarkInformation();
			currentEntryType = FolderOperaBookmarkEntry;
		}
		else if (line.startsWith(QLatin1String("#SEPERATOR")))
		{
			readedBookmark = BookmarkInformation();
			currentEntryType = SeparatorOperaBookmarkEntry;
		}
		else if (line == QLatin1String("-"))
		{
			currentEntryType = FolderEndOperaBookmarkEntry;
		}
		else if (line.startsWith(QLatin1String("\tURL=")))
		{
			readedBookmark.url = line.section(QLatin1Char('='), 1, -1);
		}
		else if (line.startsWith(QLatin1String("\tNAME=")))
		{
			readedBookmark.title = line.section(QLatin1Char('='), 1, -1);
		}
		else if (line.startsWith(QLatin1String("\tDESCRIPTION=")))
		{
			readedBookmark.description = line.section(QLatin1Char('='), 1, -1).replace(QLatin1String("\x02\x02"), QLatin1String("\n"));
		}
		else if (line.startsWith(QLatin1String("\tSHORT NAME=")))
		{
			readedBookmark.keyword = line.section(QLatin1Char('='), 1, -1);
		}
		else if (line.startsWith(QLatin1String("\tCREATED=")))
		{
			readedBookmark.added = QDateTime::fromTime_t(line.section(QLatin1Char('='), 1, -1).toUInt());
		}
		else if (line.startsWith(QLatin1String("\tVISITED=")))
		{
			readedBookmark.visited = QDateTime::fromTime_t(line.section(QLatin1Char('='), 1, -1).toUInt());
		}
		else if (line.isEmpty())
		{
			switch (currentEntryType)
			{
				case UrlOperaBookmarkEntry:
					addUrl(readedBookmark, m_duplicate);

					break;
				case FolderOperaBookmarkEntry:
					enterNewFolder(readedBookmark);

					break;
				case FolderEndOperaBookmarkEntry:
					goToParent();

					break;
				case SeparatorOperaBookmarkEntry:
					addSeparator(readedBookmark);

					break;
				default:
					break;
			}

			currentEntryType = NoOperaBookmarkEntry;
		}
	}

	return true;
}

}
