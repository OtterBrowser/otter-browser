/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_NETWORKMANAGERFACTORY_H
#define OTTER_NETWORKMANAGERFACTORY_H

#include "ItemModel.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslCipher>

namespace Otter
{

struct ProxyDefinition final
{
	enum ProxyType
	{
		NoProxy = 0,
		SystemProxy,
		ManualProxy,
		AutomaticProxy
	};

	enum ProtocolType
	{
		AnyProtocol = 0,
		HttpProtocol,
		HttpsProtocol,
		FtpProtocol,
		SocksProtocol
	};

	struct ProxyServer final
	{
		QString hostName;
		quint16 port = 8080;

		explicit ProxyServer(const QString &hostNameValue, quint16 portValue) : hostName(hostNameValue), port(portValue)
		{
		}

		ProxyServer() = default;
	};

	QString identifier;
	QString title;
	QString path;
	QStringList children;
	QStringList exceptions;
	QHash<ProtocolType, ProxyServer> servers;
	ProxyType type = SystemProxy;
	bool isFolder = false;
	bool usesSystemAuthentication = false;

	QString getTitle() const
	{
		if (title.isEmpty())
		{
			if (identifier == QLatin1String("noProxy"))
			{
				return QCoreApplication::translate("proxies", "No Proxy");
			}

			if (identifier == QLatin1String("system"))
			{
				return QCoreApplication::translate("proxies", "System Configuration");
			}
		}

		return (title.isEmpty() ? QCoreApplication::translate("proxies", "(Untitled)") : title);
	}

	bool isValid() const
	{
		return !identifier.isEmpty();
	}
};

struct UserAgentDefinition final
{
	QString identifier;
	QString name;
	QString title;
	QString value;
	QStringList children;
	bool isFolder = false;

	QString getTitle() const
	{
		if (title.isEmpty())
		{
			if (identifier == QLatin1String("default"))
			{
				return QCoreApplication::translate("userAgents", "Default User Agent");
			}

			if (!name.isEmpty())
			{
				return QCoreApplication::translate("userAgents", "Mask as {name}").replace(QLatin1String("{name}"), QString(name).replace(QLatin1Char('&'), QLatin1String("&&")));
			}
		}

		return (title.isEmpty() ? QCoreApplication::translate("userAgents", "(Untitled)") : title);
	}

	bool isValid() const
	{
		return !identifier.isEmpty();
	}
};

class CookieJar;
class NetworkCache;
class NetworkManager;
class NetworkManagerFactory;
class NetworkProxyFactory;

class ProxiesModel final : public ItemModel
{
public:
	enum ItemRole
	{
		TitleRole = ItemModel::TitleRole,
		IdentifierRole = ItemModel::UserRole
	};

	explicit ProxiesModel(const QString &selectedProxy, bool isEditor, QObject *parent = nullptr);

protected:
	void populateProxies(const QStringList &proxies, QStandardItem *parent, const QString &selectedProxy);

private:
	bool m_isEditor;
};

class UserAgentsModel final : public ItemModel
{
public:
	enum ItemRole
	{
		TitleRole = ItemModel::TitleRole,
		IdentifierRole = ItemModel::UserRole,
		UserAgentRole
	};

	explicit UserAgentsModel(const QString &selectedUserAgent, bool isEditor, QObject *parent = nullptr);

protected:
	void populateUserAgents(const QStringList &userAgents, QStandardItem *parent, const QString &selectedUserAgent);

private:
	bool m_isEditor;
};

class NetworkManagerFactory final : public QObject
{
	Q_OBJECT

public:
	enum DoNotTrackPolicy
	{
		SkipTrackPolicy = 0,
		AllowToTrackPolicy,
		DoNotAllowToTrackPolicy
	};

	Q_ENUM(DoNotTrackPolicy)

	static void createInstance();
	static void initialize();
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static void loadProxies();
	static void loadUserAgents();
	static void notifyAuthenticated(QAuthenticator *authenticator, bool wasAccepted);
	static NetworkManagerFactory* getInstance();
	static NetworkManager* getNetworkManager(bool isPrivate = false);
	static NetworkCache* getCache();
	static CookieJar* getCookieJar();
	static QNetworkReply* createRequest(const QUrl &url, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation, bool isPrivate = false, QIODevice *outgoingData = nullptr);
	static QString getAcceptLanguage();
	static QString getUserAgent();
	static QStringList getProxies();
	static QStringList getUserAgents();
	static QList<QSslCipher> getDefaultCiphers();
	static ProxyDefinition getProxy(const QString &identifier);
	static UserAgentDefinition getUserAgent(const QString &identifier);
	static DoNotTrackPolicy getDoNotTrackPolicy();
	static bool canSendReferrer();
	static bool isWorkingOffline();
	static bool usesSystemProxyAuthentication();
	bool event(QEvent *event) override;

protected:
	explicit NetworkManagerFactory(QObject *parent = nullptr);

	static void readProxy(const QJsonValue &value, ProxyDefinition *parent);
	static void readUserAgent(const QJsonValue &value, UserAgentDefinition *parent);
	static void updateProxiesOption();
	static void updateUserAgentsOption();

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	static NetworkManagerFactory *m_instance;
	static NetworkManager *m_privateNetworkManager;
	static NetworkManager *m_standardNetworkManager;
	static NetworkProxyFactory *m_proxyFactory;
	static NetworkCache *m_cache;
	static CookieJar *m_cookieJar;
	static QString m_acceptLanguage;
	static QMap<QString, ProxyDefinition> m_proxies;
	static QMap<QString, UserAgentDefinition> m_userAgents;
	static QList<QSslCipher> m_defaultCiphers;
	static DoNotTrackPolicy m_doNotTrackPolicy;
	static bool m_canSendReferrer;
	static bool m_isInitialized;
	static bool m_isWorkingOffline;

signals:
	void authenticated(QAuthenticator *authenticator, bool wasAccepted);
#if QT_VERSION < 0x060000
	void onlineStateChanged(bool isOnline);
#endif

friend class NetworkManager;
};

}

#endif
