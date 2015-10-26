/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NetworkManagerFactory.h"
#include "AddonsManager.h"
#include "ContentBlockingManager.h"
#include "CookieJar.h"
#include "NetworkCache.h"
#include "NetworkManager.h"
#include "NetworkProxyFactory.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "WebBackend.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtNetwork/QSslSocket>

namespace Otter
{

NetworkManagerFactory* NetworkManagerFactory::m_instance = NULL;
NetworkManager* NetworkManagerFactory::m_networkManager = NULL;
NetworkCache* NetworkManagerFactory::m_cache = NULL;
CookieJar* NetworkManagerFactory::m_cookieJar = NULL;
QString NetworkManagerFactory::m_acceptLanguage;
QStringList NetworkManagerFactory::m_userAgentsOrder;
QMap<QString, UserAgentInformation> NetworkManagerFactory::m_userAgents;
NetworkManagerFactory::DoNotTrackPolicy NetworkManagerFactory::m_doNotTrackPolicy = NetworkManagerFactory::SkipTrackPolicy;
QList<QSslCipher> NetworkManagerFactory::m_defaultCiphers;
bool NetworkManagerFactory::m_canSendReferrer = true;
bool NetworkManagerFactory::m_isWorkingOffline = false;
bool NetworkManagerFactory::m_isInitialized = false;
bool NetworkManagerFactory::m_isUsingSystemProxyAuthentication = false;

NetworkManagerFactory::NetworkManagerFactory(QObject *parent) : QObject(parent)
{
}

void NetworkManagerFactory::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		QNetworkProxyFactory::setApplicationProxyFactory(new NetworkProxyFactory());

		m_instance = new NetworkManagerFactory(parent);

		ContentBlockingManager::createInstance(m_instance);
	}
}

void NetworkManagerFactory::initialize()
{
	if (m_isInitialized)
	{
		return;
	}

	m_isInitialized = true;

#if QT_VERSION < 0x050300
	m_defaultCiphers = QSslSocket::supportedCiphers();
#else
	m_defaultCiphers = QSslSocket::defaultCiphers();
#endif

	for (int i = (m_defaultCiphers.count() - 1); i >= 0; --i)
	{
		if ((m_defaultCiphers.at(i).keyExchangeMethod() == QLatin1String("DH") && m_defaultCiphers.at(i).supportedBits() < 1024) || m_defaultCiphers.at(i).supportedBits() < 128 || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("PSK") || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("EXP") || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("NULL") || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("ADH") || m_defaultCiphers.at(i).isNull())
		{
			m_defaultCiphers.removeAt(i);
		}
	}

	loadUserAgents();

	m_instance->optionChanged(QLatin1String("Network/AcceptLanguage"), SettingsManager::getValue(QLatin1String("Network/AcceptLanguage")));
	m_instance->optionChanged(QLatin1String("Network/DoNotTrackPolicy"), SettingsManager::getValue(QLatin1String("Network/DoNotTrackPolicy")));
	m_instance->optionChanged(QLatin1String("Network/EnableReferrer"), SettingsManager::getValue(QLatin1String("Network/EnableReferrer")));
	m_instance->optionChanged(QLatin1String("Network/WorkOffline"), SettingsManager::getValue(QLatin1String("Network/WorkOffline")));
	m_instance->optionChanged(QLatin1String("Proxy/UseSystemAuthentication"), SettingsManager::getValue(QLatin1String("Proxy/UseSystemAuthentication")));
	m_instance->optionChanged(QLatin1String("Security/Ciphers"), SettingsManager::getValue(QLatin1String("Security/Ciphers")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), m_instance, SLOT(optionChanged(QString,QVariant)));
}

void NetworkManagerFactory::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/AcceptLanguage"))
	{
		m_acceptLanguage = ((value.toString().isEmpty()) ? QLatin1String(" ") : value.toString().replace(QLatin1String("system"), QLocale::system().bcp47Name()));
	}
	else if (option == QLatin1String("Network/DoNotTrackPolicy"))
	{
		const QString policyValue = value.toString();

		if (policyValue == QLatin1String("allow"))
		{
			m_doNotTrackPolicy = AllowToTrackPolicy;
		}
		else if (policyValue == QLatin1String("doNotAllow"))
		{
			m_doNotTrackPolicy = DoNotAllowToTrackPolicy;
		}
		else
		{
			m_doNotTrackPolicy = SkipTrackPolicy;
		}
	}
	else if (option == QLatin1String("Network/EnableReferrer"))
	{
		m_canSendReferrer = value.toBool();
	}
	else if (option == QLatin1String("Network/WorkOffline"))
	{
		m_isWorkingOffline = value.toBool();
	}
	else if (option == QLatin1String("Proxy/UseSystemAuthentication"))
	{
		m_isUsingSystemProxyAuthentication = value.toBool();
	}
	else if (option == QLatin1String("Security/Ciphers"))
	{
		if (value.toString() == QLatin1String("default"))
		{
			QSslSocket::setDefaultCiphers(m_defaultCiphers);

			return;
		}

		const QStringList selectedCiphers = value.toStringList();
		const QList<QSslCipher> supportedCiphers = QSslSocket::supportedCiphers();
		QList<QSslCipher> ciphers;

		for (int i = 0; i < supportedCiphers.count(); ++i)
		{
			if (selectedCiphers.contains(supportedCiphers.at(i).name()))
			{
				ciphers.append(supportedCiphers.at(i));
			}
		}

		QSslSocket::setDefaultCiphers(ciphers);
	}
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
	if (m_cache)
	{
		m_cache->clearCache(period);
	}
}

void NetworkManagerFactory::loadUserAgents()
{
	const QSettings settings(SessionsManager::getReadableDataPath(QLatin1String("userAgents.ini")), QSettings::IniFormat);
	const QStringList userAgentsOrder = settings.childGroups();
	QMap<QString, UserAgentInformation> userAgents;

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

void NetworkManagerFactory::notifyAuthenticated(QAuthenticator *authenticator, bool wasAccepted)
{
	emit m_instance->authenticated(authenticator, wasAccepted);
}

NetworkManagerFactory* NetworkManagerFactory::getInstance()
{
	return m_instance;
}

NetworkManager* NetworkManagerFactory::getNetworkManager()
{
	if (!m_networkManager)
	{
		m_networkManager = new NetworkManager(true, QCoreApplication::instance());
	}

	return m_networkManager;
}

NetworkCache* NetworkManagerFactory::getCache()
{
	if (!m_cache)
	{
		m_cache = new NetworkCache(QCoreApplication::instance());
	}

	return m_cache;
}

CookieJar* NetworkManagerFactory::getCookieJar()
{
	if (!m_cookieJar)
	{
		m_cookieJar = new CookieJar(false, QCoreApplication::instance());
	}

	return m_cookieJar;
}

QString NetworkManagerFactory::getAcceptLanguage()
{
	return m_acceptLanguage;
}

QString NetworkManagerFactory::getUserAgent()
{
	return AddonsManager::getWebBackend()->getUserAgent();
}

QStringList NetworkManagerFactory::getUserAgents()
{
	if (!m_isInitialized)
	{
		m_instance->initialize();
	}

	return m_userAgentsOrder;
}

QList<QSslCipher> NetworkManagerFactory::getDefaultCiphers()
{
	return m_defaultCiphers;
}

UserAgentInformation NetworkManagerFactory::getUserAgent(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("custom;")))
	{
		UserAgentInformation userAgent;
		userAgent.identifier = QLatin1String("custom");
		userAgent.title = tr("Custom");
		userAgent.value = identifier.mid(7);

		return userAgent;
	}

	if (!m_isInitialized)
	{
		m_instance->initialize();
	}

	if (identifier.isEmpty() || !m_userAgents.contains(identifier))
	{
		UserAgentInformation userAgent;
		userAgent.identifier = QLatin1String("default");
		userAgent.title = tr("Default");
		userAgent.value = QLatin1String("Mozilla/5.0 {platform} {engineVersion} {applicationVersion}");

		return userAgent;
	}

	return m_userAgents[identifier];
}

NetworkManagerFactory::DoNotTrackPolicy NetworkManagerFactory::getDoNotTrackPolicy()
{
	return m_doNotTrackPolicy;
}

bool NetworkManagerFactory::canSendReferrer()
{
	return m_canSendReferrer;
}

bool NetworkManagerFactory::isWorkingOffline()
{
	return m_isWorkingOffline;
}

bool NetworkManagerFactory::isUsingSystemProxyAuthentication()
{
	return m_isUsingSystemProxyAuthentication;
}

}
