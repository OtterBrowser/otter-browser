/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/WebBackend.h"
#include "../../../ui/BookmarksImporterWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent) : Importer(parent),
	m_optionsWidget(nullptr)
{
}

QWidget* HtmlBookmarksImporter::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new BookmarksImporterWidget(parent);
	}

	return m_optionsWidget;
}

QString HtmlBookmarksImporter::getName() const
{
	return QLatin1String("html");
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

QString HtmlBookmarksImporter::getGroup() const
{
	return QLatin1String("other");
}

QUrl HtmlBookmarksImporter::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList HtmlBookmarksImporter::getFileFilters() const
{
	return {tr("HTML files (*.htm *.html)")};
}

Importer::ImportType HtmlBookmarksImporter::getImportType() const
{
	return BookmarksImport;
}

bool HtmlBookmarksImporter::import(const QString &path)
{
	WebBackend *webBackend(AddonsManager::getWebBackend());

	if (!webBackend)
	{
		emit importFinished(BookmarksImport, FailedImport, 0);

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
		emit importFinished(BookmarksImport, FailedImport, 0);

		return false;
	}

	connect(job, &BookmarksImportJob::importStarted, this, &HtmlBookmarksImporter::importStarted);
	connect(job, &BookmarksImportJob::importProgress, this, &HtmlBookmarksImporter::importProgress);
	connect(job, &BookmarksImportJob::importFinished, this, &HtmlBookmarksImporter::importFinished);

	job->start();

	return true;
}

}
