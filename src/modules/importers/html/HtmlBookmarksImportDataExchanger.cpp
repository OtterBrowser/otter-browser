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

#include "HtmlBookmarksImportDataExchanger.h"
#include "../../../core/BookmarksManager.h"
#include "../../../core/WebBackend.h"
#include "../../../ui/BookmarksImportOptionsWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Otter
{

HtmlBookmarksImportDataExchanger::HtmlBookmarksImportDataExchanger(QObject *parent) : ImportDataExchanger(parent),
	m_optionsWidget(nullptr)
{
}

QWidget* HtmlBookmarksImportDataExchanger::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImportOptionsWidget(parent);
	}

	return m_optionsWidget;
}

QString HtmlBookmarksImportDataExchanger::getName() const
{
	return QLatin1String("html");
}

QString HtmlBookmarksImportDataExchanger::getTitle() const
{
	return tr("Import HTML Bookmarks");
}

QString HtmlBookmarksImportDataExchanger::getDescription() const
{
	return tr("Imports bookmarks from HTML file (Netscape format).");
}

QString HtmlBookmarksImportDataExchanger::getVersion() const
{
	return QLatin1String("1.0");
}

QString HtmlBookmarksImportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("bookmarks.html"));
	}

	return path;
}

QString HtmlBookmarksImportDataExchanger::getGroup() const
{
	return QLatin1String("other");
}

QUrl HtmlBookmarksImportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList HtmlBookmarksImportDataExchanger::getFileFilters() const
{
	return {tr("HTML files (*.htm *.html)")};
}

DataExchanger::ExchangeType HtmlBookmarksImportDataExchanger::getExchangeType() const
{
	return BookmarksExchange;
}

bool HtmlBookmarksImportDataExchanger::hasOptions() const
{
	return true;
}

bool HtmlBookmarksImportDataExchanger::importData(const QString &path)
{
	WebBackend *webBackend(AddonsManager::getWebBackend());

	if (!webBackend)
	{
		emit exchangeFinished(BookmarksExchange, FailedOperation, 0);

		return false;
	}

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

	BookmarksImportJob *job(webBackend->createBookmarksImportJob(folder, getSuggestedPath(path), areDuplicatesAllowed));

	if (!job)
	{
		emit exchangeFinished(BookmarksExchange, FailedOperation, 0);

		return false;
	}

	connect(job, &BookmarksImportJob::importStarted, this, &HtmlBookmarksImportDataExchanger::exchangeStarted);
	connect(job, &BookmarksImportJob::importProgress, this, &HtmlBookmarksImportDataExchanger::exchangeProgress);
	connect(job, &BookmarksImportJob::importFinished, this, &HtmlBookmarksImportDataExchanger::exchangeFinished);

	job->start();

	return true;
}

}
