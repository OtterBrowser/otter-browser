/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaBookmarksImportDataExchanger.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/BookmarksImportOptionsWidget.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace Otter
{

OperaBookmarksImportDataExchanger::OperaBookmarksImportDataExchanger(QObject *parent) : ImportDataExchanger(parent),
	m_optionsWidget(nullptr)
{
}

QWidget* OperaBookmarksImportDataExchanger::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImportOptionsWidget(parent);
	}

	return m_optionsWidget;
}

QString OperaBookmarksImportDataExchanger::getName() const
{
	return QLatin1String("opera-bookmarks");
}

QString OperaBookmarksImportDataExchanger::getTitle() const
{
	return tr("Import Opera Bookmarks");
}

QString OperaBookmarksImportDataExchanger::getDescription() const
{
	return tr("Imports bookmarks from Opera Browser version 12 or earlier");
}

QString OperaBookmarksImportDataExchanger::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaBookmarksImportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("bookmarks.adr"));
		}

		return path;
	}
#if !defined(Q_OS_DARWIN) && defined(Q_OS_UNIX)
	const QString homePath(Utils::getStandardLocation(QStandardPaths::HomeLocation));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/bookmarks.adr"));
	}
#endif

	return path;
}

QString OperaBookmarksImportDataExchanger::getGroup() const
{
	return QLatin1String("opera");
}

QUrl OperaBookmarksImportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OperaBookmarksImportDataExchanger::getFileFilters() const
{
	return {tr("Opera bookmarks files (bookmarks.adr)")};
}

DataExchanger::ExchangeType OperaBookmarksImportDataExchanger::getExchangeType() const
{
	return BookmarksExchange;
}

bool OperaBookmarksImportDataExchanger::hasOptions() const
{
	return true;
}

bool OperaBookmarksImportDataExchanger::importData(const QString &path)
{
	BookmarksModel::Bookmark *folder(nullptr);
	bool areDuplicatesAllowed(false);

	if (m_optionsWidget)
	{
		if (m_optionsWidget->hasToRemoveExisting())
		{
			BookmarksManager::getModel()->getRootItem()->removeRows(0, BookmarksManager::getModel()->getRootItem()->rowCount());

			if (m_optionsWidget->isImportingIntoSubfolder())
			{
				folder = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, m_optionsWidget->getSubfolderName()}});
			}
		}
		else
		{
			folder = m_optionsWidget->getTargetFolder();
			areDuplicatesAllowed = m_optionsWidget->areDuplicatesAllowed();
		}
	}

	BookmarksImportJob *job(new OperaBookmarksImportJob(folder, getSuggestedPath(path), areDuplicatesAllowed, this));

	connect(job, &BookmarksImportJob::importStarted, this, &OperaBookmarksImportDataExchanger::exchangeStarted);
	connect(job, &BookmarksImportJob::importProgress, this, &OperaBookmarksImportDataExchanger::exchangeProgress);
	connect(job, &BookmarksImportJob::importFinished, this, &OperaBookmarksImportDataExchanger::exchangeFinished);

	job->start();

	return true;
}

OperaBookmarksImportJob::OperaBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed, QObject *parent) : BookmarksImportJob(folder, areDuplicatesAllowed, parent),
	m_path(path),
	m_isRunning(false)
{
}

void OperaBookmarksImportJob::start()
{
	QFile file(m_path);

	if (!file.open(QIODevice::ReadOnly))
	{
		emit importFinished(DataExchanger::BookmarksExchange, DataExchanger::FailedOperation, 0);
		emit jobFinished(false);

		deleteLater();

		return;
	}

	m_isRunning = true;

	QTextStream stream(&file);
	QString line(stream.readLine());

	if (line != QLatin1String("Opera Hotlist version 2.0"))
	{
		emit importFinished(DataExchanger::BookmarksExchange, DataExchanger::FailedOperation, 0);
		emit jobFinished(false);

		deleteLater();

		return;
	}

	emit importStarted(DataExchanger::BookmarksExchange, -1);

	const int estimatedAmount((file.size() > 0) ? static_cast<int>(file.size() / 250) : 0);
	int totalAmount(0);

	BookmarksManager::getModel()->beginImport(getImportFolder(), estimatedAmount, qMin(estimatedAmount, 100));

	BookmarksModel::Bookmark *bookmark(nullptr);
	OperaBookmarkEntry type(NoEntry);
	bool isHeader(true);

	while (!stream.atEnd())
	{
		line = stream.readLine();

		if (isHeader && (line.isEmpty() || line.at(0) != QLatin1Char('#')))
		{
			continue;
		}

		isHeader = false;

		if (line.isEmpty())
		{
			if (bookmark)
			{
				if (type == FolderStartEntry)
				{
					setCurrentFolder(bookmark);
				}

				bookmark = nullptr;
			}
			else if (type == FolderEndEntry)
			{
				goToParent();
			}

			type = NoEntry;
		}
		else if (line.startsWith(QLatin1String("#URL")))
		{
			bookmark = BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, {}, getCurrentFolder());
			type = UrlEntry;

			++totalAmount;
		}
		else if (line.startsWith(QLatin1String("#FOLDER")))
		{
			bookmark = BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, {}, getCurrentFolder());
			type = FolderStartEntry;

			++totalAmount;
		}
		else if (line.startsWith(QLatin1String("#SEPERATOR")))
		{
			bookmark = BookmarksManager::addBookmark(BookmarksModel::SeparatorBookmark, {}, getCurrentFolder());
			type = SeparatorEntry;

			++totalAmount;
		}
		else if (line == QLatin1String("-"))
		{
			type = FolderEndEntry;
		}
		else if (bookmark)
		{
			if (line.startsWith(QLatin1String("\tURL=")))
			{
				const QUrl url(line.section(QLatin1Char('='), 1, -1));

				if (!areDuplicatesAllowed() && BookmarksManager::hasBookmark(url))
				{
					bookmark->remove();
					bookmark = nullptr;
				}
				else
				{
					bookmark->setData(url, BookmarksModel::UrlRole);
				}
			}
			else if (line.startsWith(QLatin1String("\tNAME=")))
			{
				bookmark->setData(line.section(QLatin1Char('='), 1, -1), BookmarksModel::TitleRole);
			}
			else if (line.startsWith(QLatin1String("\tDESCRIPTION=")))
			{
				bookmark->setData(line.section(QLatin1Char('='), 1, -1).replace(QLatin1String("\x02\x02"), QLatin1String("\n")), BookmarksModel::DescriptionRole);
			}
			else if (line.startsWith(QLatin1String("\tSHORT NAME=")))
			{
				const QString keyword(line.section(QLatin1Char('='), 1, -1));

				if (!BookmarksManager::hasKeyword(keyword))
				{
					bookmark->setData(keyword, BookmarksModel::KeywordRole);
				}
			}
			else if (line.startsWith(QLatin1String("\tCREATED=")))
			{
				bookmark->setData(QDateTime::fromSecsSinceEpoch(line.section(QLatin1Char('='), 1, -1).toUInt()), BookmarksModel::TimeAddedRole);
			}
			else if (line.startsWith(QLatin1String("\tVISITED=")))
			{
				bookmark->setData(QDateTime::fromSecsSinceEpoch(line.section(QLatin1Char('='), 1, -1).toUInt()), BookmarksModel::TimeVisitedRole);
			}
		}
	}

	BookmarksManager::getModel()->endImport();

	emit importFinished(DataExchanger::BookmarksExchange, DataExchanger::SuccessfullOperation, totalAmount);
	emit jobFinished(true);

	file.close();

	m_isRunning = false;

	deleteLater();
}

void OperaBookmarksImportJob::cancel()
{
}

bool OperaBookmarksImportJob::isRunning() const
{
	return m_isRunning;
}

}
