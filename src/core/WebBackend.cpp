/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

QString WebPageThumbnailJob::getTitle()
{
	return {};
}

QPixmap WebPageThumbnailJob::getThumbnail()
{
	return {};
}

WebBackend::WebBackend(QObject *parent) : QObject(parent), Addon()
{
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
