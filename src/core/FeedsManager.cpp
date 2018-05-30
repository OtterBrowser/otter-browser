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
#include "Application.h"
#include "Console.h"
#include "FeedParser.h"
#include "Job.h"
#include "NotificationsManager.h"
#include "SessionsManager.h"
#include "Utils.h"

namespace Otter
{

Feed::Feed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval, QObject *parent) : QObject(parent),
	m_title(title),
	m_url(url),
	m_icon(icon),
	m_error(NoError),
	m_updateInterval(updateInterval),
	m_isUpdating(false)
{
}

void Feed::setTitle(const QString &title)
{
	if (title != m_title)
	{
		m_title = title;

		emit feedModified(this);
	}
}

void Feed::setDescription(const QString &description)
{
	if (description != m_description)
	{
		m_description = description;

		emit feedModified(this);
	}
}

void Feed::setUrl(const QUrl &url)
{
	if (url != m_url)
	{
		m_url = url;

		update();

		emit feedModified(this);
	}
}

void Feed::setIcon(const QUrl &url)
{
	const IconFetchJob *job(new IconFetchJob(url, this));

	connect(job, &IconFetchJob::jobFinished, this, [=]()
	{
		setIcon(job->getIcon());
	});
}

void Feed::setIcon(const QIcon &icon)
{
	m_icon = icon;

	emit feedModified(this);
}

void Feed::setLastUpdateTime(const QDateTime &time)
{
	if (time != m_lastUpdateTime)
	{
		m_lastUpdateTime = time;

		emit feedModified(this);
	}
}

void Feed::setLastSynchronizationTime(const QDateTime &time)
{
	if (time != m_lastSynchronizationTime)
	{
		m_lastSynchronizationTime = time;

		emit feedModified(this);
	}
}

void Feed::setMimeType(const QMimeType &mimeType)
{
	m_mimeType = mimeType;
}

void Feed::setCategories(const QMap<QString, QString> &categories)
{
	if (categories != m_categories)
	{
		m_categories = categories;

		emit feedModified(this);
	}
}

void Feed::setEntries(const QVector<Feed::Entry> &entries)
{
	m_entries = entries;

	connect(NotificationsManager::createNotification(NotificationsManager::TransferCompletedEvent, tr("Feed updated:\n%1").arg(getTitle()), Notification::InformationLevel, this), &Notification::clicked, [&]()
	{
		Application::getInstance()->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(QLatin1String("view-feed:") + getUrl().toDisplayString())}});
	});

	emit feedModified(this);
}

void Feed::setError(Feed::FeedError error)
{
	m_error = error;

	emit feedModified(this);
}

void Feed::setUpdateInterval(int interval)
{
	if (interval != m_updateInterval)
	{
		m_updateInterval = interval;

		emit feedModified(this);
	}
}

void Feed::update()
{
	m_error = NoError;
	m_isUpdating = true;

	emit feedModified(this);

	DataFetchJob *job(new DataFetchJob(m_url, this));

	connect(job, &DataFetchJob::jobFinished, this, [=](bool isSuccess)
	{
		if (isSuccess)
		{
			FeedParser *parser(FeedParser::createParser(this, job));

			if (!parser || !parser->parse(job))
			{
				m_error = ParseError;

				Console::addMessage(tr("Failed to parse feed: %1").arg(m_url.toDisplayString()), Console::NetworkCategory, Console::ErrorLevel);
			}
			else
			{
				emit entriesModified(this);
			}
		}
		else
		{
			m_error = DownloadError;
		}

		m_isUpdating = false;

		emit feedModified(this);
	});
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

QMimeType Feed::getMimeType() const
{
	return m_mimeType;
}

QMap<QString, QString> Feed::getCategories() const
{
	return m_categories;
}

QVector<Feed::Entry> Feed::getEntries(const QStringList &categories) const
{
	if (!categories.isEmpty())
	{
		QVector<Entry> entries;

		for (int i = 0; i < m_entries.count(); ++i)
		{
			const Feed::Entry entry(m_entries.at(i));

			if (!m_categories.isEmpty())
			{
				for (int j = 0; j < categories.count(); ++j)
				{
					if (entry.categories.contains(categories.at(j)))
					{
						entries.append(entry);

						break;
					}
				}
			}
		}

		return entries;
	}

	return m_entries;
}

Feed::FeedError Feed::getError() const
{
	return m_error;
}

int Feed::getUpdateInterval() const
{
	return m_updateInterval;
}

bool Feed::isUpdating() const
{
	return m_isUpdating;
}

FeedsManager* FeedsManager::m_instance(nullptr);
FeedsModel* FeedsManager::m_model(nullptr);
QVector<Feed*> FeedsManager::m_feeds;
bool FeedsManager::m_isInitialized(false);

FeedsManager::FeedsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void FeedsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (m_model)
		{
			m_model->save(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")));
		}
	}
}

void FeedsManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new FeedsManager(QCoreApplication::instance());
	}
}

void FeedsManager::ensureInitialized()
{
	if (m_isInitialized)
	{
		return;
	}

	m_isInitialized = true;

	if (!m_model)
	{
		m_model = new FeedsModel(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")), m_instance);
	}
}

void FeedsManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

FeedsManager* FeedsManager::getInstance()
{
	return m_instance;
}

FeedsModel* FeedsManager::getModel()
{
	ensureInitialized();

	return m_model;
}

Feed* FeedsManager::createFeed(const QUrl &url, const QString &title, const QIcon &icon, int updateInterval)
{
	ensureInitialized();

	Feed *feed(getFeed(url));

	if (feed)
	{
		return feed;
	}

	feed = new Feed(title, url, icon, updateInterval, m_instance);

	m_feeds.append(feed);

	connect(feed, &Feed::feedModified, m_instance, &FeedsManager::scheduleSave);

	return feed;
}

Feed* FeedsManager::getFeed(const QUrl &url)
{
	ensureInitialized();

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
	ensureInitialized();

	return m_feeds;
}

}
