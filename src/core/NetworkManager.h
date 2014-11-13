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

#ifndef OTTER_NETWORKMANAGER_H
#define OTTER_NETWORKMANAGER_H

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>

namespace Otter
{

class ContentsWidget;
class CookieJar;

class NetworkManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit NetworkManager(bool isPrivate = false, bool useSimpleMode = false, ContentsWidget *widget = NULL);

	void resetStatistics();
	void setUserAgent(const QString &identifier, const QString &value);
	NetworkManager* clone(ContentsWidget *parent);
	CookieJar* getCookieJar();
	QPair<QString, QString> getUserAgent() const;
	QHash<QByteArray, QByteArray> getHeaders() const;
	QVariantHash getStatistics() const;

protected:
	void timerEvent(QTimerEvent *event);
	void updateStatus();
	QNetworkReply* createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
	ContentsWidget *m_widget;
	CookieJar *m_cookieJar;
	QNetworkReply *m_baseReply;
	QString m_userAgentIdentifier;
	QString m_userAgentValue;
	QUrl m_baseUrl;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;
	bool m_useSimpleMode;

signals:
	void messageChanged(const QString &message = QString());
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
