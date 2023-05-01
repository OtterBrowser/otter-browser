/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "DataExchanger.h"
#include "BookmarksManager.h"

namespace Otter
{

DataExchanger::DataExchanger(QObject *parent) : QObject(parent)
{
}

void DataExchanger::cancel()
{
}

QWidget* DataExchanger::createOptionsWidget(QWidget *parent)
{
	Q_UNUSED(parent)

	return nullptr;
}

Addon::AddonType DataExchanger::getType() const
{
	return ImporterType;
}

bool DataExchanger::canCancel() const
{
	return false;
}

bool DataExchanger::hasOptions() const
{
	return false;
}

ExportDataExchanger::ExportDataExchanger(QObject *parent) : DataExchanger(parent)
{
}

DataExchanger::ExchangeDirection ExportDataExchanger::getExchangeDirection() const
{
	return ExportDirection;
}

ImportDataExchanger::ImportDataExchanger(QObject *parent) : DataExchanger(parent)
{
}

DataExchanger::ExchangeDirection ImportDataExchanger::getExchangeDirection() const
{
	return ImportDirection;
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
	const qint64 seconds(timestamp.toLongLong());

	return ((seconds != 0) ? QDateTime::fromSecsSinceEpoch(seconds) : QDateTime());
}

bool BookmarksImportJob::areDuplicatesAllowed() const
{
	return m_areDuplicatesAllowed;
}

}
