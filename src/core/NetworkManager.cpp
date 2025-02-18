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

#include "NetworkManager.h"
#include "Application.h"
#include "CookieJar.h"
#include "LocalListingNetworkReply.h"
#include "NetworkCache.h"
#include "NetworkManagerFactory.h"
#include "SettingsManager.h"
#include "Utils.h"
#include "../ui/AuthenticationDialog.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFileInfo>
#include <QtWidgets/QMessageBox>
#include <QtNetwork/QNetworkProxy>

namespace Otter
{

NetworkManager::NetworkManager(bool isPrivate, QObject *parent) : QNetworkAccessManager(parent),
	m_cookieJar(nullptr)
{
	NetworkManagerFactory::initialize();

	if (isPrivate)
	{
		m_cookieJar = new DiskCookieJar({}, this);

		setCookieJar(m_cookieJar);
	}
	else
	{
		m_cookieJar = NetworkManagerFactory::getCookieJar();

		setCookieJar(m_cookieJar);

		m_cookieJar->setParent(QCoreApplication::instance());

		QNetworkDiskCache *cache(NetworkManagerFactory::getCache());

		setCache(cache);

		cache->setParent(QCoreApplication::instance());
	}

	connect(this, &NetworkManager::authenticationRequired, this, &NetworkManager::handleAuthenticationRequired);
	connect(this, &NetworkManager::proxyAuthenticationRequired, this, &NetworkManager::handleProxyAuthenticationRequired);
	connect(this, &NetworkManager::sslErrors, this, &NetworkManager::handleSslErrors);
#if QT_VERSION < 0x060000
	connect(NetworkManagerFactory::getInstance(), &NetworkManagerFactory::onlineStateChanged, this, [&](bool isOnline)
	{
		if (isOnline)
		{
			setNetworkAccessible(QNetworkAccessManager::Accessible);
		}
	});
#endif
}

void NetworkManager::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	AuthenticationDialog dialog(reply->url(), authenticator, AuthenticationDialog::HttpAuthentication, Application::getActiveWindow());
	dialog.exec();
}

void NetworkManager::handleProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
	if (NetworkManagerFactory::usesSystemProxyAuthentication())
	{
		authenticator->setUser({});

		return;
	}

	AuthenticationDialog dialog(QUrl(proxy.hostName()), authenticator, AuthenticationDialog::ProxyAuthentication, Application::getActiveWindow());
	dialog.exec();
}

void NetworkManager::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	if (errors.isEmpty())
	{
		reply->ignoreSslErrors(errors);

		return;
	}

	const QStringList exceptions(SettingsManager::getOption(SettingsManager::Security_IgnoreSslErrorsOption, Utils::extractHost(reply->url())).toStringList());
	QList<QSslError> errorsToIgnore;
	QStringList messages;
	messages.reserve(errors.count());

	for (int i = 0; i < errors.count(); ++i)
	{
		const QSslError error(errors.at(i));

		if (error.error() == QSslError::NoError)
		{
			continue;
		}

		if (exceptions.contains(QString::fromLatin1(error.certificate().digest().toBase64())))
		{
			errorsToIgnore.append(error);
		}
		else
		{
			messages.append(error.errorString());
		}
	}

	if (!errorsToIgnore.isEmpty())
	{
		reply->ignoreSslErrors(errorsToIgnore);
	}

	if (!messages.isEmpty() && QMessageBox::warning(Application::getActiveWindow(), tr("Warning"), tr("SSL errors occurred:\n\n%1\n\nDo you want to continue?").arg(messages.join(QLatin1Char('\n'))), (QMessageBox::Yes | QMessageBox::No)) == QMessageBox::Yes)
	{
		reply->ignoreSslErrors(errors);
	}
}

CookieJar* NetworkManager::getCookieJar() const
{
	return m_cookieJar;
}

QNetworkReply* NetworkManager::createRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QIODevice *outgoingData)
{
	if (operation == GetOperation && request.url().isLocalFile() && QFileInfo(request.url().toLocalFile()).isDir())
	{
		return new LocalListingNetworkReply(request, this);
	}

	QNetworkRequest mutableRequest(request);

	if (!NetworkManagerFactory::canSendReferrer())
	{
		mutableRequest.setRawHeader(QByteArrayLiteral("Referer"), {});
	}

	if (operation == PostOperation && mutableRequest.header(QNetworkRequest::ContentTypeHeader).isNull())
	{
		mutableRequest.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
	}

	if (NetworkManagerFactory::isWorkingOffline())
	{
		mutableRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
	}
	else if (NetworkManagerFactory::getDoNotTrackPolicy() != NetworkManagerFactory::SkipTrackPolicy)
	{
		mutableRequest.setRawHeader(QByteArrayLiteral("DNT"), QByteArray::number((NetworkManagerFactory::getDoNotTrackPolicy() == NetworkManagerFactory::DoNotAllowToTrackPolicy) ? 1 : 0));
	}

	mutableRequest.setRawHeader(QByteArrayLiteral("Accept-Language"), NetworkManagerFactory::getAcceptLanguage().toLatin1());

	return QNetworkAccessManager::createRequest(operation, mutableRequest, outgoingData);
}

NetworkManager::ResourceType NetworkManager::getResourceType(const QNetworkRequest &request, const QUrl &firstPartyUrl)
{
	if (request.url() == firstPartyUrl)
	{
		return MainFrameType;
	}

	const QString path(request.url().path());
	const QByteArray acceptHeader(request.rawHeader(QByteArrayLiteral("Accept")));

	if (acceptHeader.contains(QByteArrayLiteral("text/html")) || acceptHeader.contains(QByteArrayLiteral("application/xhtml+xml")) || acceptHeader.contains(QByteArrayLiteral("application/xml")) || path.endsWith(QLatin1String(".htm")) || path.endsWith(QLatin1String(".html")))
	{
		return SubFrameType;
	}

	if (acceptHeader.contains(QByteArrayLiteral("image/")) || path.endsWith(QLatin1String(".png")) || path.endsWith(QLatin1String(".jpg")) || path.endsWith(QLatin1String(".jpeg")) || path.endsWith(QLatin1String(".gif")) || path.endsWith(QLatin1String(".webp")))
	{
		return ImageType;
	}

	if (acceptHeader.contains(QByteArrayLiteral("script/")) || path.endsWith(QLatin1String(".js")))
	{
		return ScriptType;
	}

	if (acceptHeader.contains(QByteArrayLiteral("text/css")) || path.endsWith(QLatin1String(".css")))
	{
		return StyleSheetType;
	}

	if (acceptHeader.contains(QByteArrayLiteral("object")))
	{
		return ObjectType;
	}

	if (request.rawHeader(QByteArrayLiteral("X-Requested-With")) == QByteArrayLiteral("XMLHttpRequest"))
	{
		return XmlHttpRequestType;
	}

	if (request.hasRawHeader(QByteArrayLiteral("Sec-WebSocket-Protocol")))
	{
		return WebSocketType;
	}

	return OtherType;
}

}
