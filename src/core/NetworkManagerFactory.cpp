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
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QSslSocket>

namespace Otter
{

NetworkManagerFactory* NetworkManagerFactory::m_instance(nullptr);
NetworkManager* NetworkManagerFactory::m_networkManager(nullptr);
NetworkCache* NetworkManagerFactory::m_cache(nullptr);
CookieJar* NetworkManagerFactory::m_cookieJar(nullptr);
QString NetworkManagerFactory::m_acceptLanguage;
QMap<QString, UserAgentInformation> NetworkManagerFactory::m_userAgents;
NetworkManagerFactory::DoNotTrackPolicy NetworkManagerFactory::m_doNotTrackPolicy(NetworkManagerFactory::SkipTrackPolicy);
QList<QSslCipher> NetworkManagerFactory::m_defaultCiphers;
bool NetworkManagerFactory::m_canSendReferrer(true);
bool NetworkManagerFactory::m_isWorkingOffline(false);
bool NetworkManagerFactory::m_isInitialized(false);
bool NetworkManagerFactory::m_isUsingSystemProxyAuthentication(false);

NetworkManagerFactory::NetworkManagerFactory(QObject *parent) : QObject(parent)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Google Chrome 50 (Windows)"))
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Mozilla Firefox 45 (Windows)"))
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Microsoft Edge 25 (Windows)"))
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Internet Explorer 10.0 (Windows)"))
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Opera 12.16 (Windows)"))
	Q_UNUSED(QT_TRANSLATE_NOOP("userAgents", "Mask as Safari 9.0 (MacOS)"))

	QNetworkConfigurationManager *networkConfigurationManager(new QNetworkConfigurationManager(this));

	connect(networkConfigurationManager, SIGNAL(onlineStateChanged(bool)), this, SIGNAL(onlineStateChanged(bool)));
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

	m_defaultCiphers = QSslSocket::defaultCiphers();

	for (int i = (m_defaultCiphers.count() - 1); i >= 0; --i)
	{
		if (m_defaultCiphers.at(i).isNull() || (m_defaultCiphers.at(i).keyExchangeMethod() == QLatin1String("DH") && m_defaultCiphers.at(i).supportedBits() < 1024) || m_defaultCiphers.at(i).supportedBits() < 128 || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("PSK") || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("EXP") || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("nullptr") || m_defaultCiphers.at(i).encryptionMethod().startsWith(QLatin1String("RC4(")) || m_defaultCiphers.at(i).authenticationMethod() == QLatin1String("ADH"))
		{
			m_defaultCiphers.removeAt(i);
		}
	}

	loadUserAgents();

	m_instance->optionChanged(SettingsManager::Network_AcceptLanguageOption, SettingsManager::getValue(SettingsManager::Network_AcceptLanguageOption));
	m_instance->optionChanged(SettingsManager::Network_DoNotTrackPolicyOption, SettingsManager::getValue(SettingsManager::Network_DoNotTrackPolicyOption));
	m_instance->optionChanged(SettingsManager::Network_EnableReferrerOption, SettingsManager::getValue(SettingsManager::Network_EnableReferrerOption));
	m_instance->optionChanged(SettingsManager::Network_WorkOfflineOption, SettingsManager::getValue(SettingsManager::Network_WorkOfflineOption));
	m_instance->optionChanged(SettingsManager::Proxy_UseSystemAuthenticationOption, SettingsManager::getValue(SettingsManager::Proxy_UseSystemAuthenticationOption));
	m_instance->optionChanged(SettingsManager::Security_CiphersOption, SettingsManager::getValue(SettingsManager::Security_CiphersOption));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), m_instance, SLOT(optionChanged(int,QVariant)));
}

void NetworkManagerFactory::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Network_AcceptLanguageOption)
	{
		m_acceptLanguage = ((value.toString().isEmpty()) ? QLatin1String(" ") : value.toString().replace(QLatin1String("system"), QLocale::system().bcp47Name()));
	}
	else if (identifier == SettingsManager::Network_DoNotTrackPolicyOption)
	{
		const QString policyValue(value.toString());

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
	else if (identifier == SettingsManager::Network_EnableReferrerOption)
	{
		m_canSendReferrer = value.toBool();
	}
	else if (identifier == SettingsManager::Network_WorkOfflineOption)
	{
		m_isWorkingOffline = value.toBool();
	}
	else if (identifier == SettingsManager::Proxy_UseSystemAuthenticationOption)
	{
		m_isUsingSystemProxyAuthentication = value.toBool();
	}
	else if (identifier == SettingsManager::Security_CiphersOption)
	{
		if (value.toString() == QLatin1String("default"))
		{
			QSslSocket::setDefaultCiphers(m_defaultCiphers);

			return;
		}

		const QStringList selectedCiphers(value.toStringList());
		QList<QSslCipher> ciphers;

		for (int i = 0; i < selectedCiphers.count(); ++i)
		{
			const QSslCipher cipher(selectedCiphers.at(i));

			if (!cipher.isNull())
			{
				ciphers.append(cipher);
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
	m_userAgents.clear();

	QFile file(SessionsManager::getReadableDataPath(QLatin1String("userAgents.json")));

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	const QJsonArray userAgents(QJsonDocument::fromJson(file.readAll()).array());
	UserAgentInformation root;

	for (int i = 0; i < userAgents.count(); ++i)
	{
		if (userAgents.at(i).isObject())
		{
			const QJsonObject object(userAgents.at(i).toObject());
			const QString identifier(object.value(QLatin1String("identifier")).toString());

			if (!m_userAgents.contains(identifier))
			{
				UserAgentInformation userAgent;
				userAgent.identifier = identifier;
				userAgent.title = object.value(QLatin1String("title")).toString();
				userAgent.value = object.value(QLatin1String("value")).toString();

				m_userAgents[identifier] = userAgent;
			}

			root.children.append(identifier);
		}
		else if (userAgents.at(i).isString() && userAgents.at(i).toString() == QLatin1String("separator"))
		{
			root.children.append(QString());
		}
	}

	m_userAgents[QLatin1String("root")] = root;

	file.close();
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

	return m_userAgents[QLatin1String("root")].children;
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
