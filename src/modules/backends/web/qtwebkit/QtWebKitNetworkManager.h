/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITNETWORKMANAGER_H
#define OTTER_QTWEBKITNETWORKMANAGER_H

#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"

#include <QtNetwork/QNetworkRequest>

namespace Otter
{

class CookieJarProxy;
class QtWebKitWebWidget;
class WebBackend;

class QtWebKitNetworkManager : public NetworkManager
{
	Q_OBJECT

public:
	explicit QtWebKitNetworkManager(bool isPrivate, QtWebKitWebWidget *widget);

	QHash<QByteArray, QByteArray> getHeaders() const;
	QVariantHash getStatistics() const;

protected:
	void timerEvent(QTimerEvent *event);
	void resetStatistics();
	void updateStatus();
	void updateOptions(const QUrl &url);
	void setFormRequest(const QUrl &url);
	void setWidget(QtWebKitWebWidget *widget);
	QtWebKitNetworkManager *clone();
	QNetworkReply* createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData);

protected slots:
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);

private:
	QtWebKitWebWidget *m_widget;
	CookieJarProxy *m_cookieJar;
	QNetworkReply *m_baseReply;
	QString m_acceptLanguage;
	QString m_userAgent;
	QUrl m_formRequestUrl;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;
	NetworkManagerFactory::DoNotTrackPolicy m_doNotTrackPolicy;
	bool m_canSendReferrer;

	static WebBackend *m_backend;

signals:
	void messageChanged(const QString &message = QString());
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);

friend class QtWebKitPage;
friend class QtWebKitWebWidget;
};

}

#endif
