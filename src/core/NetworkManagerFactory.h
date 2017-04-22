/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TreeModel.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QSslCipher>

namespace Otter
{

struct ProxyDefinition
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

	struct ProxyServer
	{
		QString hostName;
		int port = 8080;

		explicit ProxyServer(const QString &hostNameValue, int portValue)
		{
			hostName = hostNameValue;
			port = portValue;
		}

		ProxyServer()
		{
		}
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
		return (title.isEmpty() ? QCoreApplication::translate("proxies", "(Untitled)") : QCoreApplication::translate("proxies", title.toUtf8().constData()));
	}

	bool isValid() const
	{
		return !identifier.isEmpty();
	}
};

struct UserAgentDefinition
{
	QString identifier;
	QString title;
	QString value;
	QStringList children;
	bool isFolder = false;

	QString getTitle() const
	{
		return (title.isEmpty() ? QCoreApplication::translate("userAgents", "(Untitled)") : QCoreApplication::translate("userAgents", title.toUtf8().constData()));
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

class ProxiesModel final : public TreeModel
{
public:
	enum ItemRole
	{
		TitleRole = TreeModel::TitleRole,
		IdentifierRole = TreeModel::UserRole
	};

	explicit ProxiesModel(const QString &selectedProxy, bool isEditor, QObject *parent = nullptr);

protected:
	void populateProxies(const QStringList &proxies, QStandardItem *parent, const QString &selectedProxy);

private:
	bool m_isEditor;
};

class UserAgentsModel final : public TreeModel
{
public:
	enum ItemRole
	{
		TitleRole = TreeModel::TitleRole,
		IdentifierRole = TreeModel::UserRole,
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
	Q_ENUMS(DoNotTrackPolicy)

public:
	enum DoNotTrackPolicy
	{
		SkipTrackPolicy = 0,
		AllowToTrackPolicy,
		DoNotAllowToTrackPolicy
	};

	static void createInstance(QObject *parent = nullptr);
	static void initialize();
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static void loadProxies();
	static void loadUserAgents();
	static void notifyAuthenticated(QAuthenticator *authenticator, bool wasAccepted);
	static NetworkManagerFactory* getInstance();
	static NetworkManager* getNetworkManager();
	static NetworkCache* getCache();
	static CookieJar* getCookieJar();
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
	static NetworkManager *m_networkManager;
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
	void onlineStateChanged(bool isOnline);

friend class NetworkManager;
};

}

#endif
