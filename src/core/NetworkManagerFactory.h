/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_NETWORKMANAGERFACTORY_H
#define OTTER_NETWORKMANAGERFACTORY_H

#include <QtCore/QObject>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QSslCipher>

namespace Otter
{

struct UserAgentInformation
{
	QString identifier;
	QString title;
	QString value;
	QStringList children;
};

class CookieJar;
class NetworkCache;
class NetworkManager;

class NetworkManagerFactory : public QObject
{
	Q_OBJECT
	Q_ENUMS(DoNotTrackPolicy)

public:
	enum DoNotTrackPolicy
	{
		SkipTrackPolicy = 0,
		AllowToTrackPolicy = 1,
		DoNotAllowToTrackPolicy = 2
	};

	static void createInstance(QObject *parent = NULL);
	static void initialize();
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static void loadUserAgents();
	static void notifyAuthenticated(QAuthenticator *authenticator, bool wasAccepted);
	static NetworkManagerFactory* getInstance();
	static NetworkManager* getNetworkManager();
	static NetworkCache* getCache();
	static CookieJar* getCookieJar();
	static QString getAcceptLanguage();
	static QString getUserAgent();
	static QStringList getUserAgents();
	static QList<QSslCipher> getDefaultCiphers();
	static UserAgentInformation getUserAgent(const QString &identifier);
	static DoNotTrackPolicy getDoNotTrackPolicy();
	static bool canSendReferrer();
	static bool isWorkingOffline();
	static bool isUsingSystemProxyAuthentication();

protected:
	explicit NetworkManagerFactory(QObject *parent = NULL);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);

private:
	static NetworkManagerFactory *m_instance;
	static NetworkManager *m_networkManager;
	static NetworkCache *m_cache;
	static CookieJar *m_cookieJar;
	static QString m_acceptLanguage;
	static QMap<QString, UserAgentInformation> m_userAgents;
	static QList<QSslCipher> m_defaultCiphers;
	static DoNotTrackPolicy m_doNotTrackPolicy;
	static bool m_canSendReferrer;
	static bool m_isWorkingOffline;
	static bool m_isInitialized;
	static bool m_isUsingSystemProxyAuthentication;

signals:
	void authenticated(QAuthenticator *authenticator, bool wasAccepted);
	void onlineStateChanged(bool isOnline);

friend class NetworkManager;
};

}

#endif
