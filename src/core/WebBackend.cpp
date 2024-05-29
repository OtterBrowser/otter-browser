/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

namespace Otter
{

WebPageThumbnailJob::WebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent) : Job(parent)
{
	Q_UNUSED(url)
	Q_UNUSED(size)
}

WebBackend::WebBackend(QObject *parent) : QObject(parent)
{
}

BookmarksImportJob* WebBackend::createBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed)
{
	Q_UNUSED(folder)
	Q_UNUSED(path)
	Q_UNUSED(areDuplicatesAllowed)

	return nullptr;
}

WebPageThumbnailJob* WebBackend::createPageThumbnailJob(const QUrl &url, const QSize &size)
{
	Q_UNUSED(url)
	Q_UNUSED(size)

	return nullptr;
}

CookieJar* WebBackend::getCookieJar() const
{
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

WebBackend::CapabilityScopes WebBackend::getCapabilityScopes(WebBackend::BackendCapability capability) const
{
	Q_UNUSED(capability)

	return NoScope;
}

bool WebBackend::hasCapability(WebBackend::BackendCapability capability) const
{
	return (getCapabilityScopes(capability) != NoScope);
}

}
