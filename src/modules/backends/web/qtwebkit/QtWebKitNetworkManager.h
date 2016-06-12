/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitWebWidget.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/WindowsManager.h"

#include <QtNetwork/QNetworkRequest>

namespace Otter
{

class QtWebKitCookieJar;
class QtWebKitWebWidget;
class WebBackend;

class QtWebKitNetworkManager : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit QtWebKitNetworkManager(bool isPrivate, QtWebKitCookieJar *cookieJarProxy, QtWebKitWebWidget *parent);

	CookieJar* getCookieJar();
	WebWidget::SslInformation getSslInformation() const;
	QStringList getBlockedElements() const;
	QHash<QByteArray, QByteArray> getHeaders() const;
	QVariantHash getStatistics() const;
	WindowsManager::ContentStates getContentState() const;
	bool isLoading() const;

protected:
	void timerEvent(QTimerEvent *event);
	void addContentBlockingException(const QUrl &url, ContentBlockingManager::ResourceType resourceType);
	void resetStatistics();
	void registerTransfer(QNetworkReply *reply);
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
	void handleOnlineStateChanged(bool isOnline);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void requestFinished(QNetworkReply *reply);
	void transferFinished();

private:
	QtWebKitWebWidget *m_widget;
	CookieJar *m_cookieJar;
	QtWebKitCookieJar *m_cookieJarProxy;
	QNetworkReply *m_baseReply;
	QString m_acceptLanguage;
	QString m_userAgent;
	QUrl m_formRequestUrl;
	QDateTime m_dateDownloaded;
	WebWidget::SslInformation m_sslInformation;
	QStringList m_blockedElements;
	QList<QNetworkReply*> m_transfers;
	QList<ContentBlockingManager::CheckResult> m_blockedRequests;
	QVector<int> m_contentBlockingProfiles;
	QSet<QUrl> m_contentBlockingExceptions;
	QHash<QNetworkReply*, QPair<qint64, bool> > m_replies;
	WindowsManager::ContentStates m_contentState;
	qint64 m_speed;
	qint64 m_bytesReceivedDifference;
	qint64 m_bytesReceived;
	qint64 m_bytesTotal;
	int m_isSecure;
	int m_finishedRequests;
	int m_startedRequests;
	int m_updateTimer;
	NetworkManagerFactory::DoNotTrackPolicy m_doNotTrackPolicy;
	bool m_areImagesEnabled;
	bool m_canSendReferrer;

	static WebBackend *m_backend;

signals:
	void messageChanged(const QString &message = QString());
	void contentStateChanged(WindowsManager::ContentStates state);
	void documentLoadProgressChanged(int progress);
	void statusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);

friend class QtWebKitPage;
friend class QtWebKitWebWidget;
};

}

#endif
