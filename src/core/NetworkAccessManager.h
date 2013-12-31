/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_NETWORKACCESSMANAGER_H
#define OTTER_NETWORKACCESSMANAGER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>

namespace Otter
{

class ContentsWidget;
class CookieJar;
class NetworkCache;

class NetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT
	Q_ENUMS(DoNotTrackPolicy)

public:
	explicit NetworkAccessManager(bool privateWindow = false, bool simpleMode = false, ContentsWidget *widget = NULL);

	enum DoNotTrackPolicy
	{
		SkipTrackPolicy = 0,
		AllowToTrackPolicy = 1,
		DoNotAllowToTrackPolicy = 2
	};

	void resetStatistics();
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static QNetworkCookieJar* getCookieJar(bool privateCookieJar = false);
	static NetworkCache* getCache();

protected:
	void timerEvent(QTimerEvent *event);
	void updateStatus();
	QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
	ContentsWidget *m_widget;
	QNetworkReply *m_mainReply;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	DoNotTrackPolicy m_doNotTrackPolicy;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;
	bool m_simpleMode;

	static CookieJar *m_cookieJar;
	static QNetworkCookieJar *m_privateCookieJar;
	static NetworkCache *m_cache;

signals:
	void messageChanged(const QString &message = QString());
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
