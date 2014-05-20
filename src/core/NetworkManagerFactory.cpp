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

#include "NetworkManagerFactory.h"
#include "CookieJar.h"
#include "NetworkCache.h"
#include "NetworkProxyFactory.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>

namespace Otter
{

NetworkManagerFactory* NetworkManagerFactory::m_instance = NULL;
CookieJar* NetworkManagerFactory::m_cookieJar = NULL;
QNetworkCookieJar* NetworkManagerFactory::m_privateCookieJar = NULL;
NetworkCache* NetworkManagerFactory::m_cache = NULL;
QStringList NetworkManagerFactory::m_userAgentsOrder;
QHash<QString, UserAgentInformation> NetworkManagerFactory::m_userAgents;
bool NetworkManagerFactory::m_userAgentsInitialized = false;

NetworkManagerFactory::NetworkManagerFactory(QObject *parent) : QObject(parent)
{
}

void NetworkManagerFactory::createInstance(QObject *parent)
{
	QNetworkProxyFactory::setApplicationProxyFactory(new NetworkProxyFactory());

	m_instance = new NetworkManagerFactory(parent);
}

void NetworkManagerFactory::clearCookies(int period)
{
	if (!m_cookieJar)
	{
		m_cookieJar = new CookieJar(QCoreApplication::instance());
	}

	m_cookieJar->clearCookies(period);
}

void NetworkManagerFactory::clearCache(int period)
{
	m_cache->clearCache(period);
}

void NetworkManagerFactory::loadUserAgents()
{
	const QString path = (SessionsManager::getProfilePath() + QLatin1String("/userAgents.ini"));
	const QSettings settings((QFile::exists(path) ? path : QLatin1String(":/other/userAgents.ini")), QSettings::IniFormat);
	const QStringList userAgentsOrder = settings.childGroups();
	QHash<QString, UserAgentInformation> userAgents;

	for (int i = 0; i < userAgentsOrder.count(); ++i)
	{
		UserAgentInformation userAgent;
		userAgent.identifier = userAgentsOrder.at(i);
		userAgent.title = settings.value(QString("%1/title").arg(userAgentsOrder.at(i))).toString();
		userAgent.value = settings.value(QString("%1/value").arg(userAgentsOrder.at(i))).toString();

		userAgents[userAgentsOrder.at(i)] = userAgent;
	}

	m_userAgentsOrder = userAgentsOrder;
	m_userAgents = userAgents;
}

NetworkManagerFactory *NetworkManagerFactory::getInstance()
{
	return m_instance;
}

QNetworkCookieJar* NetworkManagerFactory::getCookieJar(bool privateCookieJar)
{
	if (!m_cookieJar && !privateCookieJar)
	{
		m_cookieJar = new CookieJar(QCoreApplication::instance());
	}

	if (!m_privateCookieJar && privateCookieJar)
	{
		m_privateCookieJar = new QNetworkCookieJar(QCoreApplication::instance());
	}

	return (privateCookieJar ? m_privateCookieJar : m_cookieJar);
}

NetworkCache* NetworkManagerFactory::getCache()
{
	if (!m_cache)
	{
		m_cache = new NetworkCache(QCoreApplication::instance());
	}

	return m_cache;
}

UserAgentInformation NetworkManagerFactory::getUserAgent(const QString &identifier)
{
	if (!m_userAgentsInitialized)
	{
		m_userAgentsInitialized = true;

		loadUserAgents();
	}

	if (identifier.isEmpty() || !m_userAgents.contains(identifier))
	{
		UserAgentInformation userAgent;
		userAgent.identifier = QLatin1String("default");
		userAgent.title = tr("Default");

		return userAgent;
	}

	return m_userAgents[identifier];
}

QStringList NetworkManagerFactory::getUserAgents()
{
	if (!m_userAgentsInitialized)
	{
		m_userAgentsInitialized = true;

		loadUserAgents();
	}

	return m_userAgentsOrder;
}

}
