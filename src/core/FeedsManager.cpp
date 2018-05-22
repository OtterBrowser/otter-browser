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
#include "Console.h"
#include "SessionsManager.h"
#include "Utils.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QXmlStreamReader>
#include <QtWidgets/QMessageBox>

namespace Otter
{

Feed::Feed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval, QObject *parent) : QObject(parent),
	m_title(title),
	m_url(url),
	m_icon(icon),
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

QStringList Feed::getCategories() const
{
	return m_categories;
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
	return m_updateInterval;
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

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")));

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		Console::addMessage(tr("Failed to open feeds file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, path);

		return;
	}

	QXmlStreamReader reader(&file);

	if (reader.readNextStartElement() && reader.name() == QLatin1String("opml") && reader.attributes().value(QLatin1String("version")).toString() == QLatin1String("1.0"))
	{
		while (reader.readNextStartElement())
		{
			if (reader.name() == QLatin1String("outline"))
			{
				const QUrl url(Utils::normalizeUrl(QUrl(reader.attributes().value(QLatin1String("xmlUrl")).toString())));

				if (url.isValid())
				{
					createFeed(reader.attributes().value(QLatin1String("title")).toString(), url, Utils::loadPixmapFromDataUri(reader.attributes().value(QLatin1String("icon")).toString()), reader.attributes().value(QLatin1String("updateInterval")).toInt());
				}
			}

			if (reader.name() != QLatin1String("body"))
			{
				reader.skipCurrentElement();
			}

			if (reader.hasError())
			{
				Console::addMessage(tr("Failed to load feeds file: %1").arg(reader.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

				QMessageBox::warning(nullptr, tr("Error"), tr("Failed to load feeds file."), QMessageBox::Close);

				return;
			}
		}
	}

	file.close();

	m_instance->scheduleSave();
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
	if (!m_model)
	{
		m_model = new FeedsModel(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")), m_instance);
	}

	return m_model;
}

Feed* FeedsManager::createFeed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval)
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
