/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WebBackend.h"
#include "BookmarksManager.h"

namespace Otter
{

HtmlBookmarksImportJob::HtmlBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed, QObject *parent) : Job(parent),
	m_currentFolder(folder),
	m_importFolder(folder),
	m_areDuplicatesAllowed(areDuplicatesAllowed)
{
	Q_UNUSED(path)
}

void HtmlBookmarksImportJob::goToParent()
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

void HtmlBookmarksImportJob::setCurrentFolder(BookmarksModel::Bookmark *folder)
{
	m_currentFolder = folder;
}

BookmarksModel::Bookmark* HtmlBookmarksImportJob::getCurrentFolder() const
{
	return m_currentFolder;
}

BookmarksModel::Bookmark* HtmlBookmarksImportJob::getImportFolder() const
{
	return m_importFolder;
}

bool HtmlBookmarksImportJob::areDuplicatesAllowed() const
{
	return m_areDuplicatesAllowed;
}

WebPageThumbnailJob::WebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent) : Job(parent)
{
	Q_UNUSED(url)
	Q_UNUSED(size)
}

WebBackend::WebBackend(QObject *parent) : QObject(parent)
{
}

HtmlBookmarksImportJob* WebBackend::createBookmarksImportJob(const QString &path)
{
	Q_UNUSED(path)

	return nullptr;
}

WebPageThumbnailJob* WebBackend::createPageThumbnailJob(const QUrl &url, const QSize &size)
{
	Q_UNUSED(url)
	Q_UNUSED(size)

	return nullptr;
}

QVector<SpellCheckManager::DictionaryInformation> WebBackend::getDictionaries() const
{
	return {};
}

Addon::AddonType WebBackend::getType() const
{
	return WebBackendType;
}

WebBackend::BackendCapabilities WebBackend::getCapabilities() const
{
	return NoCapabilities;
}

}
