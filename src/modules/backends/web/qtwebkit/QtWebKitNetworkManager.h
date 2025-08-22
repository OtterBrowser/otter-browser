/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBKITNETWORKMANAGER_H
#define OTTER_QTWEBKITNETWORKMANAGER_H

#include "QtWebKitWebWidget.h"
#include "../../../../core/NetworkManager.h"
#include "../../../../core/NetworkManagerFactory.h"

#include <QtNetwork/QNetworkRequest>

namespace Otter
{

class NetworkProxyFactory;
class QtWebKitCookieJar;
class WebBackend;

class QtWebKitNetworkManager final : public QNetworkAccessManager
{
	Q_OBJECT

public:
	explicit QtWebKitNetworkManager(bool isPrivate, QtWebKitCookieJar *cookieJarProxy, QtWebKitWebWidget *parent);

	CookieJar* getCookieJar() const;
	QVariant getPageInformation(WebWidget::PageInformation key) const;
	WebWidget::SslInformation getSslInformation() const;
	QStringList getBlockedElements() const;
	QVector<NetworkManager::ResourceInformation> getBlockedRequests() const;
	QMap<QByteArray, QByteArray> getHeaders() const;
	WebWidget::ContentStates getContentState() const;

protected:
	struct ReplyInformation final
	{
		qint64 bytesReceived = 0;
		bool hasTotalBytes = false;
	};

	void timerEvent(QTimerEvent *event) override;
	void addContentBlockingException(const QUrl &url, NetworkManager::ResourceType resourceType);
	void resetStatistics();
	void registerTransfer(QNetworkReply *reply);
	void updateLoadingSpeed();
	void updateOptions(const QUrl &url);
	void setPageInformation(WebWidget::PageInformation key, const QVariant &value);
	void setFormRequest(const QUrl &url);
	void setMainRequest(const QUrl &url);
	void setWidget(QtWebKitWebWidget *widget);
	QtWebKitNetworkManager* clone() const;
	QNetworkReply* createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData) override;
	QString getUserAgent() const;
	QVariant getOption(int identifier, const QUrl &url) const;

protected slots:
	void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void handleRequestFinished(QNetworkReply *reply);
	void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
	void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
	void handleLoadFinished(bool result);

private:
	QPointer<QtWebKitWebWidget> m_widget;
	CookieJar *m_cookieJar;
	QtWebKitCookieJar *m_cookieJarProxy;
	NetworkProxyFactory *m_proxyFactory;
	QNetworkReply *m_baseReply;
	QString m_acceptLanguage;
	QString m_userAgent;
	QUrl m_formRequestUrl;
	QUrl m_mainRequestUrl;
	WebWidget::SslInformation m_sslInformation;
	QStringList m_blockedElements;
	QStringList m_unblockedHosts;
	QVector<QNetworkReply*> m_transfers;
	QVector<NetworkManager::ResourceInformation> m_blockedRequests;
	QVector<int> m_contentBlockingProfiles;
	QSet<QUrl> m_contentBlockingExceptions;
	QHash<QNetworkReply*, ReplyInformation> m_replies;
	QMap<QByteArray, QByteArray> m_headers;
	QMap<WebWidget::PageInformation, QVariant> m_pageInformation;
	WebWidget::ContentStates m_contentState;
	NetworkManagerFactory::DoNotTrackPolicy m_doNotTrackPolicy;
	TrileanValue m_isSecureValue;
	qint64 m_bytesReceivedDifference;
	int m_loadingSpeedTimer;
	bool m_areImagesEnabled;
	bool m_canSendReferrer;
	bool m_isWorkingOffline;

	static WebBackend *m_backend;

signals:
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void contentStateChanged(WebWidget::ContentStates state);

friend class QtWebKitPage;
friend class QtWebKitWebWidget;
};

}

#endif
