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

#ifndef OTTER_FEEDSMANAGER_H
#define OTTER_FEEDSMANAGER_H

#include "FeedsModel.h"

#include <QtCore/QDateTime>

namespace Otter
{

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
		QStringList categories;
		QUrl url;
		QDateTime publicationTime;
		QDateTime updateTime;
	};

	explicit Feed(const QString &title, const QUrl &url, const QIcon &icon, int updateInterval, QObject *parent = nullptr);

	void setTitle(const QString &title);
	void setDescription(const QString &description);
	void setUrl(const QUrl &url);
	void setIcon(const QUrl &url);
	void setIcon(const QIcon &icon);
	void setLastUpdateTime(const QDateTime &time);
	void setLastSynchronizationTime(const QDateTime &time);
	void setCategories(const QStringList &categories);
	void setEntries(const QVector<Entry> &entries);
	void setError(FeedError error);
	void setUpdateInterval(int interval);
	QString getTitle() const;
	QString getDescription() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QDateTime getLastUpdateTime() const;
	QDateTime getLastSynchronizationTime() const;
	QStringList getCategories() const;
	QVector<Entry> getEntries() const;
	FeedError getError() const;
	int getUpdateInterval() const;

public slots:
	void update();

private:
	QString m_title;
	QString m_description;
	QUrl m_url;
	QIcon m_icon;
	QDateTime m_lastUpdateTime;
	QDateTime m_lastSynchronizationTime;
	QStringList m_categories;
	QVector<Entry> m_entries;
	FeedError m_error;
	int m_updateInterval;

signals:
	void feedModified(Feed *feed);
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
	static QVector<Feed*> getFeeds();

protected:
	explicit FeedsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void ensureInitialized();

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
