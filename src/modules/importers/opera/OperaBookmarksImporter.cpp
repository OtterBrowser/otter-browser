/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/BookmarksManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>

namespace Otter
{

OperaBookmarksImporter::OperaBookmarksImporter(QObject *parent): BookmarksImporter(parent),
	m_optionsWidget(NULL)
{
}

OperaBookmarksImporter::~OperaBookmarksImporter()
{
	if (m_optionsWidget)
	{
		m_optionsWidget->deleteLater();
	}
}

QWidget* OperaBookmarksImporter::getOptionsWidget()
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImporterWidget();
	}

	return m_optionsWidget;
}

QString OperaBookmarksImporter::getTitle() const
{
	return QString(tr("Opera Bookmarks"));
}

QString OperaBookmarksImporter::getDescription() const
{
	return QString(tr("Imports bookmarks from Opera Browser version 12 or earlier"));
}

QString OperaBookmarksImporter::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaBookmarksImporter::getFileFilter() const
{
	return tr("Opera bookmarks files (bookmarks.adr)");
}

QString OperaBookmarksImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("bookmarks.adr"));
		}
		else
		{
			return path;
		}
	}
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	const QString homePath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/bookmarks.adr"));
	}
#endif

	return path;
}

QString OperaBookmarksImporter::getBrowser() const
{
	return QLatin1String("opera");
}

QUrl OperaBookmarksImporter::getHomePage() const
{
	return QUrl(QLatin1String("http://otter-browser.org/"));
}

QIcon OperaBookmarksImporter::getIcon() const
{
	return QIcon();
}

bool OperaBookmarksImporter::import(const QString &path)
{
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QString line = stream.readLine();

	if (line != QLatin1String("Opera Hotlist version 2.0"))
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
				setImportFolder(BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), m_optionsWidget->getSubfolderName(), BookmarksManager::getModel()->getRootItem()));
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

	BookmarksItem *bookmark = NULL;
	OperaBookmarkEntry type = NoEntry;
	bool isHeader = true;

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
			bookmark = BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, QUrl(), QString(), getCurrentFolder());
			type = UrlEntry;
		}
		else if (line.startsWith(QLatin1String("#FOLDER")))
		{
			bookmark = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), QString(), getCurrentFolder());
			type = FolderStartEntry;
		}
		else if (line.startsWith(QLatin1String("#SEPERATOR")))
		{
			bookmark = BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, QUrl(), QString(), getCurrentFolder());
			type = SeparatorEntry;
		}
		else if (line == QLatin1String("-"))
		{
			type = FolderEndEntry;
		}
		else if (line.startsWith(QLatin1String("\tURL=")) && bookmark)
		{
			const QUrl url(line.section(QLatin1Char('='), 1, -1));

			if (!allowDuplicates() && BookmarksManager::hasBookmark(url))
			{
				bookmark->remove();
				bookmark = NULL;
			}
			else
			{
				bookmark->setData(url, BookmarksModel::UrlRole);
			}
		}
		else if (line.startsWith(QLatin1String("\tNAME=")) && bookmark)
		{
			bookmark->setData(line.section(QLatin1Char('='), 1, -1), BookmarksModel::TitleRole);
		}
		else if (line.startsWith(QLatin1String("\tDESCRIPTION=")) && bookmark)
		{
			bookmark->setData(line.section(QLatin1Char('='), 1, -1).replace(QLatin1String("\x02\x02"), QLatin1String("\n")), BookmarksModel::DescriptionRole);
		}
		else if (line.startsWith(QLatin1String("\tSHORT NAME=")) && bookmark)
		{
			const QString keyword = line.section(QLatin1Char('='), 1, -1);

			if (!BookmarksManager::hasKeyword(keyword))
			{
				bookmark->setData(keyword, BookmarksModel::KeywordRole);
			}
		}
		else if (line.startsWith(QLatin1String("\tCREATED=")) && bookmark)
		{
			bookmark->setData(QDateTime::fromTime_t(line.section(QLatin1Char('='), 1, -1).toUInt()), BookmarksModel::TimeAddedRole);
		}
		else if (line.startsWith(QLatin1String("\tVISITED=")) && bookmark)
		{
			bookmark->setData(QDateTime::fromTime_t(line.section(QLatin1Char('='), 1, -1).toUInt()), BookmarksModel::TimeVisitedRole);
		}
		else if (line.isEmpty())
		{
			if (bookmark)
			{
				if (type == FolderStartEntry)
				{
					setCurrentFolder(bookmark);
				}

				bookmark = NULL;
			}
			else if (type == FolderEndEntry)
			{
				goToParent();
			}

			type = NoEntry;
		}
	}

	file.close();

	return true;
}

}
