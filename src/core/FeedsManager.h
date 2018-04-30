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

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

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

	struct Entry
	{
		QString identifier;
		QString title;
		QString summary;
		QString content;
		QString author;
		QString email;
		QUrl url;
		QDateTime publicationTime;
		QDateTime updateTime;
	};

	explicit Feed(const QString &title, const QUrl &url, int updateInterval, QObject *parent = nullptr);

	QString getTitle() const;
	QString getDescription() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QDateTime getLastUpdateTime() const;
	QDateTime getLastSynchronizationTime() const;
	QVector<Entry> getEntries();
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
	QVector<Entry> m_entries;
	FeedError m_error;
	int m_updateInterval;

signals:
	void feedModified(const QUrl &url);
};

class FeedsManager final : public QObject
{
	Q_OBJECT

public:
	static void createInstance();
	static FeedsManager* getInstance();
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
	static QVector<Feed*> m_feeds;
	static bool m_isInitialized;

signals:
	void feedAdded(const QUrl &url);
	void feedModified(const QUrl &url);
	void feedRemoved(const QUrl &url);
};

}

#endif
