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

#ifndef OTTER_FEEDSMANAGER_H
#define OTTER_FEEDSMANAGER_H

#include "FeedsModel.h"

#include <QtCore/QDateTime>
#include <QtCore/QMimeType>
#include <QtCore/QThread>

namespace Otter
{

class FeedsManager;
class FeedParser;
class LongTermTimer;

class Feed final : public QObject
{
	Q_OBJECT

public:
	enum FeedError
	{
		NoError = 0,
		DownloadError,
		ParseError
	};

	struct Entry final
	{
		QString identifier;
		QString title;
		QString summary;
		QString content;
		QString author;
		QString email;
		QUrl url;
		QDateTime lastReadTime;
		QDateTime publicationTime;
		QDateTime updateTime;
		QStringList categories;
	};

	explicit Feed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval, QObject *parent = nullptr);

	void markEntryAsRead(const QString &identifier);
	void markAllEntriesAsRead();
	void markEntryAsRemoved(const QString &identifier);
	void setTitle(const QString &title);
	void setDescription(const QString &description);
	void setUrl(const QUrl &url);
	void setIcon(const QIcon &icon);
	void setLastUpdateTime(const QDateTime &time);
	void setLastSynchronizationTime(const QDateTime &time);
	void setUpdateInterval(int interval);
	QString getTitle() const;
	QString getDescription() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QDateTime getLastUpdateTime() const;
	QDateTime getLastSynchronizationTime() const;
	QMimeType getMimeType() const;
	QMap<QString, QString> getCategories() const;
	QStringList getRemovedEntries() const;
	QVector<Entry> getEntries(const QStringList &categories = {}) const;
	FeedError getError() const;
	int getUnreadEntriesAmount() const;
	int getUpdateInterval() const;
	int getUpdateProgress() const;
	bool isUpdating() const;

public slots:
	void update();

protected:
	void setCategories(const QMap<QString, QString> &categories);
	void setRemovedEntries(const QStringList &removedEntries);
	void setEntries(const QVector<Entry> &entries);
	static QDateTime normalizeDateTime(const QDateTime &time);

private:
	LongTermTimer *m_updateTimer;
	FeedParser *m_parser;
	QThread m_parserThread;
	QString m_title;
	QString m_description;
	QUrl m_url;
	QIcon m_icon;
	QDateTime m_lastUpdateTime;
	QDateTime m_lastSynchronizationTime;
	QMimeType m_mimeType;
	QMap<QString, QString> m_categories;
	QStringList m_removedEntries;
	QVector<Entry> m_entries;
	FeedError m_error;
	int m_updateInterval;
	int m_updateProgress;
	bool m_isUpdating;

signals:
	void feedModified(Feed *feed);
	void entriesModified(Feed *feed);
	void updateProgressChanged(int progress);

friend class FeedsManager;
};

class FeedsManager final : public QObject
{
	Q_OBJECT

public:
	static void createInstance();
	static FeedsManager* getInstance();
	static FeedsModel* getModel();
	static Feed* createFeed(const QUrl &url, const QString &title = {}, const QIcon &icon = {}, int updateInterval = -1);
	static Feed* getFeed(const QUrl &url);
	static QUrl createFeedReaderUrl(const QUrl &url);
	static QVector<Feed*> getFeeds();

protected:
	explicit FeedsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void ensureInitialized();
	void save();

protected slots:
	void scheduleSave();

private:
	int m_saveTimer;

	static FeedsManager *m_instance;
	static FeedsModel *m_model;
	static QVector<Feed*> m_feeds;
	static bool m_isInitialized;

signals:
	void feedAdded(const QUrl &url);
	void feedModified(const QUrl &url);
	void feedRemoved(const QUrl &url);
};

}

#endif
