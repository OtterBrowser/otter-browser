/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "BookmarksManager.h"
#include "Console.h"
#include "FeedParser.h"
#include "Job.h"
#include "LongTermTimer.h"
#include "NotificationsManager.h"
#include "SessionsManager.h"
#include "Utils.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

namespace Otter
{

Feed::Feed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval, QObject *parent) : QObject(parent),
	m_updateTimer(nullptr),
	m_parser(nullptr),
	m_title(title),
	m_url(url),
	m_icon(icon),
	m_error(NoError),
	m_updateInterval(0),
	m_updateProgress(-1),
	m_isUpdating(false)
{
	setUpdateInterval(updateInterval);
}

void Feed::markEntryAsRead(const QString &identifier)
{
	for (int i = 0; i < m_entries.count(); ++i)
	{
		if (m_entries.at(i).identifier == identifier)
		{
			m_entries[i].lastReadTime = QDateTime::currentDateTimeUtc();

			emit feedModified(this);

			break;
		}
	}
}

void Feed::markAllEntriesAsRead()
{
	const QDateTime currentDateTime(QDateTime::currentDateTimeUtc());

	for (int i = 0; i < m_entries.count(); ++i)
	{
		if (!m_entries.at(i).lastReadTime.isValid())
		{
			m_entries[i].lastReadTime = currentDateTime;
		}
	}

	emit feedModified(this);
}

void Feed::markEntryAsRemoved(const QString &identifier)
{
	if (m_removedEntries.contains(identifier))
	{
		return;
	}

	for (int i = 0; i < m_entries.count(); ++i)
	{
		if (m_entries.at(i).identifier == identifier)
		{
			m_entries.removeAt(i);

			m_removedEntries.append(identifier);

			emit feedModified(this);

			break;
		}
	}
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

void Feed::setIcon(const QIcon &icon)
{
	m_icon = icon;

	emit feedModified(this);
}

void Feed::setLastUpdateTime(const QDateTime &time)
{
	m_lastUpdateTime = time;
}

void Feed::setLastSynchronizationTime(const QDateTime &time)
{
	m_lastSynchronizationTime = time;
}

void Feed::setCategories(const QMap<QString, QString> &categories)
{
	m_categories = categories;
}

void Feed::setRemovedEntries(const QStringList &removedEntries)
{
	m_removedEntries = removedEntries;
}

void Feed::setEntries(const QVector<Feed::Entry> &entries)
{
	m_entries = entries;
}

void Feed::setUpdateInterval(int interval)
{
	if (interval == m_updateInterval)
	{
		return;
	}

	m_updateInterval = interval;

	if (interval <= 0 && m_updateTimer)
	{
		m_updateTimer->deleteLater();
		m_updateTimer = nullptr;
	}
	else
	{
		if (!m_updateTimer)
		{
			m_updateTimer = new LongTermTimer(this);

			connect(m_updateTimer, &LongTermTimer::timeout, this, &Feed::update);
		}

		m_updateTimer->start(static_cast<quint64>(interval) * 60000);
	}

	emit feedModified(this);
}

void Feed::update()
{
	if (m_parser)
	{
		return;
	}

	m_error = NoError;
	m_isUpdating = true;

	emit feedModified(this);

	DataFetchJob *dataJob(new DataFetchJob(m_url, this));

	connect(dataJob, &DataFetchJob::progressChanged, this, [&](int progress)
	{
		m_updateProgress = progress;

		emit updateProgressChanged(progress);
	});
	connect(dataJob, &DataFetchJob::jobFinished, this, [=](bool isDataFetchSuccess)
	{
		if (!isDataFetchSuccess)
		{
			m_error = DownloadError;
			m_isUpdating = false;

			Console::addMessage(tr("Failed to download feed"), Console::NetworkCategory, Console::ErrorLevel, m_url.toDisplayString());

			emit feedModified(this);

			return;
		}

		m_parser = FeedParser::createParser(this, dataJob);

		if (!m_parser)
		{
			m_error = ParseError;
			m_isUpdating = false;

			Console::addMessage(tr("Failed to parse feed: unknown feed format"), Console::NetworkCategory, Console::ErrorLevel, m_url.toDisplayString());

			emit feedModified(this);

			return;
		}

		m_parser->moveToThread(&m_parserThread);

		connect(m_parser, &FeedParser::parsingFinished, m_parser, [&](bool isParsingSuccess)
		{
			const FeedParser::FeedInformation information(m_parser->getInformation());

			if (!isParsingSuccess)
			{
				m_error = ParseError;
			}

			if (m_icon.isNull() && information.icon.isValid())
			{
				IconFetchJob *iconJob(new IconFetchJob(information.icon, this));

				connect(iconJob, &IconFetchJob::jobFinished, this, [=](bool isIconFetchSuccess)
				{
					if (isIconFetchSuccess)
					{
						setIcon(iconJob->getIcon());
					}
				});

				iconJob->start();
			}

			if (m_title.isEmpty())
			{
				m_title = information.title;
			}

			if (m_description.isEmpty())
			{
				m_description = information.description;
			}

			if (!information.entries.isEmpty())
			{
				QStringList existingRemovedEntries;
				int amount(0);

				for (int i = (information.entries.count() - 1); i >= 0; --i)
				{
					Feed::Entry entry(information.entries.at(i));

					if (m_removedEntries.contains(entry.identifier))
					{
						existingRemovedEntries.append(entry.identifier);

						continue;
					}

					bool hasEntry(false);

					for (int j = 0; j < m_entries.count(); ++j)
					{
						const Feed::Entry existingEntry(m_entries.at(j));

						if (existingEntry.identifier != entry.identifier)
						{
							continue;
						}

						if ((entry.publicationTime.isValid() && existingEntry.publicationTime != entry.publicationTime) || (entry.updateTime.isValid() && existingEntry.updateTime != entry.updateTime))
						{
							++amount;
						}

						entry.lastReadTime = existingEntry.lastReadTime;
						entry.publicationTime = normalizeDateTime(entry.publicationTime);

						if (entry.updateTime.isValid())
						{
							entry.updateTime = normalizeDateTime(entry.updateTime);
						}

						m_entries[j] = entry;

						hasEntry = true;

						break;
					}

					if (!hasEntry)
					{
						++amount;

						entry.publicationTime = normalizeDateTime(entry.publicationTime);
						entry.updateTime = normalizeDateTime(entry.updateTime);

						m_entries.prepend(entry);
					}
				}

				m_removedEntries = existingRemovedEntries;

				if (amount > 0)
				{
					Notification::Message message;
					message.message = getTitle() + QLatin1Char('\n') + tr("%n new message(s)", nullptr, amount);
					message.icon = getIcon();
					message.event = NotificationsManager::FeedUpdatedEvent;

					if (message.icon.isNull())
					{
						message.icon = ThemesManager::createIcon(QLatin1String("application-rss+xml"));
					}

					connect(NotificationsManager::createNotification(message, this), &Notification::clicked, this, [&]()
					{
						Application::getInstance()->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), FeedsManager::createFeedReaderUrl(getUrl())}});
					});
				}

				emit entriesModified(this);
			}

			m_mimeType = information.mimeType;
			m_lastSynchronizationTime = QDateTime::currentDateTimeUtc();
			m_lastUpdateTime = information.lastUpdateTime;
			m_categories = information.categories;

			m_parserThread.exit();

			m_parser->deleteLater();
			m_parser = nullptr;

			m_isUpdating = false;

			emit feedModified(this);
		});

		m_parserThread.start();

		m_parser->parse(dataJob);

		m_updateProgress = -1;

		emit updateProgressChanged(-1);
	});

	dataJob->start();
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

QDateTime Feed::normalizeDateTime(const QDateTime &time)
{
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());

	return ((time.isValid() && time < currentTime) ? time : currentTime);
}

QMimeType Feed::getMimeType() const
{
	return m_mimeType;
}

QMap<QString, QString> Feed::getCategories() const
{
	return m_categories;
}

QStringList Feed::getRemovedEntries() const
{
	return m_removedEntries;
}

QVector<Feed::Entry> Feed::getEntries(const QStringList &categories) const
{
	if (categories.isEmpty())
	{
		return m_entries;
	}

	QVector<Entry> entries;
	entries.reserve(entries.count() / 2);

	for (int i = 0; i < m_entries.count(); ++i)
	{
		const Feed::Entry entry(m_entries.at(i));

		if (!entry.categories.isEmpty())
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

	entries.squeeze();

	return entries;
}

Feed::FeedError Feed::getError() const
{
	return m_error;
}

int Feed::getUnreadEntriesAmount() const
{
	int amount(0);

	for (int i = 0; i < m_entries.count(); ++i)
	{
		if (m_entries.at(i).lastReadTime.isNull())
		{
			++amount;
		}
	}

	return amount;
}

int Feed::getUpdateInterval() const
{
	return m_updateInterval;
}

int Feed::getUpdateProgress() const
{
	return m_updateProgress;
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

		save();
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

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("feeds.json")));

	if (file.open(QIODevice::ReadOnly))
	{
		const QJsonArray feedsArray(QJsonDocument::fromJson(file.readAll()).array());

		file.close();

		for (int i = 0; i < feedsArray.count(); ++i)
		{
			const QJsonObject feedObject(feedsArray.at(i).toObject());
			Feed *feed(createFeed(QUrl(feedObject.value(QLatin1String("url")).toString()), feedObject.value(QLatin1String("title")).toString(), Utils::loadPixmapFromDataUri(feedObject.value(QLatin1String("icon")).toString()), feedObject.value(QLatin1String("updateInterval")).toInt()));
			feed->setDescription(feedObject.value(QLatin1String("description")).toString());
			feed->setLastUpdateTime(QDateTime::fromString(feedObject.value(QLatin1String("lastUpdateTime")).toString(), Qt::ISODate));
			feed->setLastSynchronizationTime(QDateTime::fromString(feedObject.value(QLatin1String("lastSynchronizationTime")).toString(), Qt::ISODate));
			feed->setRemovedEntries(feedObject.value(QLatin1String("removedEntries")).toVariant().toStringList());

			if (feedObject.contains(QLatin1String("categories")))
			{
				QMap<QString, QString> categories;
				const QVariantMap rawCategories(feedObject.value(QLatin1String("categories")).toVariant().toMap());
				QVariantMap::const_iterator iterator;

				for (iterator = rawCategories.begin(); iterator != rawCategories.end(); ++iterator)
				{
					categories[iterator.key()] = iterator.value().toString();
				}

				feed->setCategories(categories);
			}

			const QJsonArray entriesArray(feedObject.value(QLatin1String("entries")).toArray());
			QVector<Feed::Entry> entries;
			entries.reserve(entriesArray.count());

			for (int j = 0; j < entriesArray.count(); ++j)
			{
				const QJsonObject entryObject(entriesArray.at(j).toObject());
				Feed::Entry entry;
				entry.identifier = entryObject.value(QLatin1String("identifier")).toString();
				entry.title = entryObject.value(QLatin1String("title")).toString();
				entry.summary = entryObject.value(QLatin1String("summary")).toString();
				entry.content = entryObject.value(QLatin1String("content")).toString();
				entry.author = entryObject.value(QLatin1String("author")).toString();
				entry.email = entryObject.value(QLatin1String("email")).toString();
				entry.url = entryObject.value(QLatin1String("url")).toString();
				entry.lastReadTime = QDateTime::fromString(entryObject.value(QLatin1String("lastReadTime")).toString(), Qt::ISODate);
				entry.publicationTime = QDateTime::fromString(entryObject.value(QLatin1String("publicationTime")).toString(), Qt::ISODate);
				entry.updateTime = QDateTime::fromString(entryObject.value(QLatin1String("updateTime")).toString(), Qt::ISODate);
				entry.categories = entryObject.value(QLatin1String("categories")).toVariant().toStringList();

				entries.append(entry);
			}

			feed->setEntries(entries);
		}
	}

	if (!m_model)
	{
		m_model = new FeedsModel(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")), m_instance);

		connect(m_model, &FeedsModel::modelModified, m_instance, &FeedsManager::scheduleSave);
	}
}

void FeedsManager::scheduleSave()
{
	if (Application::isAboutToQuit())
	{
		if (m_saveTimer != 0)
		{
			killTimer(m_saveTimer);

			m_saveTimer = 0;
		}

		save();
	}
	else if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void FeedsManager::save()
{
	if (m_model)
	{
		m_model->save(SessionsManager::getWritableDataPath(QLatin1String("feeds.opml")));
	}

	if (SessionsManager::isReadOnly())
	{
		return;
	}

	QSaveFile file(SessionsManager::getWritableDataPath(QLatin1String("feeds.json")));

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	QJsonArray feedsArray;

	for (int i = 0; i < m_feeds.count(); ++i)
	{
		const Feed *feed(m_feeds.at(i));

		if (!FeedsManager::getModel()->hasFeed(feed->getUrl()) && !BookmarksManager::getModel()->hasFeed(feed->getUrl()))
		{
			continue;
		}

		const QMap<QString, QString> categories(feed->getCategories());
		QJsonObject feedObject({{QLatin1String("title"), feed->getTitle()}, {QLatin1String("url"), feed->getUrl().toString()}, {QLatin1String("updateInterval"), QString::number(feed->getUpdateInterval())}, {QLatin1String("lastSynchronizationTime"), feed->getLastUpdateTime().toString(Qt::ISODate)}, {QLatin1String("lastUpdateTime"), feed->getLastSynchronizationTime().toString(Qt::ISODate)}});

		if (!feed->getDescription().isEmpty())
		{
			feedObject.insert(QLatin1String("description"), feed->getDescription());
		}

		if (!feed->getIcon().isNull())
		{
			feedObject.insert(QLatin1String("icon"), Utils::savePixmapAsDataUri(feed->getIcon().pixmap(feed->getIcon().availableSizes().value(0, {16, 16}))));
		}

		if (!categories.isEmpty())
		{
			QMap<QString, QString>::const_iterator iterator;
			QJsonObject categoriesObject;

			for (iterator = categories.begin(); iterator != categories.end(); ++iterator)
			{
				categoriesObject.insert(iterator.key(), iterator.value());
			}

			feedObject.insert(QLatin1String("categories"), categoriesObject);
		}

		if (!feed->getRemovedEntries().isEmpty())
		{
			feedObject.insert(QLatin1String("removedEntries"), QJsonArray::fromStringList(feed->getRemovedEntries()));
		}

		const QVector<Feed::Entry> entries(feed->getEntries());
		QJsonArray entriesArray;

		for (int j = 0; j < entries.count(); ++j)
		{
			const Feed::Entry &entry(entries.at(j));
			QJsonObject entryObject({{QLatin1String("identifier"), entry.identifier}, {QLatin1String("title"), entry.title}});

			if (!entry.summary.isEmpty())
			{
				entryObject.insert(QLatin1String("summary"), entry.summary);
			}

			if (!entry.content.isEmpty())
			{
				entryObject.insert(QLatin1String("content"), entry.content);
			}

			if (!entry.author.isEmpty())
			{
				entryObject.insert(QLatin1String("author"), entry.author);
			}

			if (!entry.email.isEmpty())
			{
				entryObject.insert(QLatin1String("email"), entry.email);
			}

			if (!entry.url.isEmpty())
			{
				entryObject.insert(QLatin1String("url"), entry.url.toString());
			}

			if (entry.lastReadTime.isValid())
			{
				entryObject.insert(QLatin1String("lastReadTime"), entry.lastReadTime.toString(Qt::ISODate));
			}

			if (entry.publicationTime.isValid())
			{
				entryObject.insert(QLatin1String("publicationTime"), entry.publicationTime.toString(Qt::ISODate));
			}

			if (entry.updateTime.isValid())
			{
				entryObject.insert(QLatin1String("updateTime"), entry.updateTime.toString(Qt::ISODate));
			}

			if (!entry.categories.isEmpty())
			{
				entryObject.insert(QLatin1String("categories"), QJsonArray::fromStringList(entry.categories));
			}

			entriesArray.append(entryObject);
		}

		feedObject.insert(QLatin1String("entries"), entriesArray);

		feedsArray.append(feedObject);
	}

	QJsonDocument document;
	document.setArray(feedsArray);

	file.write(document.toJson());
	file.commit();
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

	connect(feed, &Feed::feedModified, m_instance, [=]()
	{
		m_instance->scheduleSave();

		emit m_instance->feedModified(feed->getUrl());
	});

	return feed;
}

Feed* FeedsManager::getFeed(const QUrl &url)
{
	ensureInitialized();

	const QUrl normalizedUrl(Utils::normalizeUrl(url));

	for (int i = 0; i < m_feeds.count(); ++i)
	{
		Feed *feed(m_feeds.at(i));

		if (feed->getUrl() == url || feed->getUrl() == normalizedUrl)
		{
			return feed;
		}
	}

	return nullptr;
}

QUrl FeedsManager::createFeedReaderUrl(const QUrl &url)
{
	return {QLatin1String("feed:") + url.toDisplayString()};
}

QVector<Feed*> FeedsManager::getFeeds()
{
	ensureInitialized();

	return m_feeds;
}

}
