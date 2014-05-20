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

#ifndef OTTER_NETWORKMANAGERFACTORY_H
#define OTTER_NETWORKMANAGERFACTORY_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkDiskCache>

namespace Otter
{

struct UserAgentInformation
{
	QString identifier;
	QString title;
	QString value;
};

class CookieJar;
class NetworkCache;

class NetworkManagerFactory : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void clearCookies(int period = 0);
	static void clearCache(int period = 0);
	static void loadUserAgents();
	static NetworkManagerFactory* getInstance();
	static QNetworkCookieJar* getCookieJar(bool privateCookieJar = false);
	static NetworkCache* getCache();
	static UserAgentInformation getUserAgent(const QString &identifier);
	static QStringList getUserAgents();

protected:
	explicit NetworkManagerFactory(QObject *parent = NULL);

private:
	static NetworkManagerFactory *m_instance;
	static CookieJar *m_cookieJar;
	static QNetworkCookieJar *m_privateCookieJar;
	static NetworkCache *m_cache;
	static QStringList m_userAgentsOrder;
	static QHash<QString, UserAgentInformation> m_userAgents;
	static bool m_userAgentsInitialized;
};

}

#endif
