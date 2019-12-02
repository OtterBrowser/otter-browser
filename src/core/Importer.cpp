/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "Importer.h"
#include "BookmarksManager.h"

namespace Otter
{

Importer::Importer(QObject *parent) : QObject(parent)
{
}

void Importer::cancel()
{
}

void Importer::notifyImportStarted(int type, int total)
{
	emit importStarted(static_cast<ImportType>(type), total);
}

void Importer::notifyImportProgress(int type, int total, int amount)
{
	emit importProgress(static_cast<ImportType>(type), total, amount);
}

void Importer::notifyImportFinished(int type, int result, int total)
{
	emit importFinished(static_cast<ImportType>(type), static_cast<ImportResult>(result), total);
}

Addon::AddonType Importer::getType() const
{
	return ImporterType;
}

bool Importer::canCancel()
{
	return false;
}

ImportJob::ImportJob(QObject *parent) : Job(parent)
{
}

BookmarksImportJob::BookmarksImportJob(BookmarksModel::Bookmark *folder, bool areDuplicatesAllowed, QObject *parent) : ImportJob(parent),
	m_currentFolder(folder),
	m_importFolder(folder),
	m_areDuplicatesAllowed(areDuplicatesAllowed)
{
}

void BookmarksImportJob::goToParent()
{
	if (m_currentFolder != m_importFolder)
	{
		if (m_currentFolder)
		{
			m_currentFolder = m_currentFolder->getParent();
		}

		if (!m_currentFolder)
		{
			m_currentFolder = (m_importFolder ? m_importFolder : BookmarksManager::getModel()->getRootItem());
		}
	}
}

void BookmarksImportJob::setCurrentFolder(BookmarksModel::Bookmark *folder)
{
	m_currentFolder = folder;
}

BookmarksModel::Bookmark* BookmarksImportJob::getCurrentFolder() const
{
	return m_currentFolder;
}

BookmarksModel::Bookmark* BookmarksImportJob::getImportFolder() const
{
	return m_importFolder;
}

QDateTime BookmarksImportJob::getDateTime(const QString &timestamp) const
{
#if QT_VERSION < 0x050800
	const uint seconds(timestamp.toUInt());

	return ((seconds > 0) ? QDateTime::fromTime_t(seconds) : QDateTime());
#else
	const qint64 seconds(timestamp.toLongLong());

	return ((seconds != 0) ? QDateTime::fromSecsSinceEpoch(seconds) : QDateTime());
#endif
}

bool BookmarksImportJob::areDuplicatesAllowed() const
{
	return m_areDuplicatesAllowed;
}

}
