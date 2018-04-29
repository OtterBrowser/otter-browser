/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "FeedsManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

Feed::Feed(const QString &title, const QUrl &url, int updateInterval, QObject *parent) : QObject(parent),
	m_title(title),
	m_url(url),
	m_error(NoError),
	m_updateInterval(updateInterval)
{
}

void Feed::update()
{
// TODO
}

QString Feed::getTitle() const
{
	return m_title;
}

QString Feed::getDescription() const
{
	return m_description;
}

QUrl Feed::getUrl() const
{
	return m_url;
}

QIcon Feed::getIcon() const
{
	return m_icon;
}

QDateTime Feed::getLastUpdateTime() const
{
	return m_lastUpdateTime;
}

QDateTime Feed::getLastSynchronizationTime() const
{
	return m_lastSynchronizationTime;
}

QVector<Feed::Entry> Feed::getEntries()
{
	return m_entries;
}

Feed::FeedError Feed::getError() const
{
	return m_error;
}

int Feed::getUpdateInterval() const
{
	return m_updateInterval;;
}

FeedsManager* FeedsManager::m_instance(nullptr);
QVector<Feed*> FeedsManager::m_feeds;

FeedsManager::FeedsManager(QObject *parent) : QObject(parent)
{
}

void FeedsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new FeedsManager(QCoreApplication::instance());
	}
}

FeedsManager* FeedsManager::getInstance()
{
	return m_instance;
}

Feed* FeedsManager::getFeed(const QUrl &url)
{
	const QUrl normalizedUrl(Utils::normalizeUrl(url));

	for (int i = 0; i < m_feeds.count(); ++i)
	{
		Feed *feed(m_feeds.at(i));

		if (m_feeds.at(i)->getUrl() == url || m_feeds.at(i)->getUrl() == normalizedUrl)
		{
			return feed;
		}
	}

	return nullptr;
}

QVector<Feed*> FeedsManager::getFeeds()
{
	return m_feeds;
}

}
