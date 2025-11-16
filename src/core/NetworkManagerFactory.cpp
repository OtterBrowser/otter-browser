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

#include "NetworkManagerFactory.h"
#include "AddonsManager.h"
#include "Application.h"
#include "ContentFiltersManager.h"
#include "CookieJar.h"
#include "NetworkCache.h"
#include "NetworkManager.h"
#include "NetworkProxyFactory.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "WebBackend.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#if QT_VERSION < 0x060000
#include <QtNetwork/QNetworkConfigurationManager>
#endif
#include <QtNetwork/QSslConfiguration>

namespace Otter
{

NetworkManagerFactory* NetworkManagerFactory::m_instance(nullptr);
NetworkManager* NetworkManagerFactory::m_privateNetworkManager(nullptr);
NetworkManager* NetworkManagerFactory::m_standardNetworkManager(nullptr);
NetworkProxyFactory* NetworkManagerFactory::m_proxyFactory(nullptr);
NetworkCache* NetworkManagerFactory::m_cache(nullptr);
CookieJar* NetworkManagerFactory::m_cookieJar(nullptr);
QString NetworkManagerFactory::m_acceptLanguage;
QMap<QString, ProxyDefinition> NetworkManagerFactory::m_proxies;
QMap<QString, UserAgentDefinition> NetworkManagerFactory::m_userAgents;
NetworkManagerFactory::DoNotTrackPolicy NetworkManagerFactory::m_doNotTrackPolicy(NetworkManagerFactory::SkipTrackPolicy);
QList<QSslCipher> NetworkManagerFactory::m_defaultCiphers;
bool NetworkManagerFactory::m_canSendReferrer(true);
bool NetworkManagerFactory::m_isInitialized(false);
bool NetworkManagerFactory::m_isWorkingOffline(false);

ProxiesModel::ProxiesModel(const QString &selectedProxy, bool isEditor, QObject *parent) : ItemModel(parent),
	m_isEditor(isEditor)
{
	if (isEditor)
	{
		setExclusive(true);
	}

	populateProxies(NetworkManagerFactory::getProxies(), invisibleRootItem(), selectedProxy);
}

void ProxiesModel::populateProxies(const QStringList &proxies, QStandardItem *parent, const QString &selectedProxy)
{
	for (int i = 0; i < proxies.count(); ++i)
	{
		const QString identifier(proxies.at(i));
		const ProxyDefinition proxy(identifier.isEmpty() ? ProxyDefinition() : NetworkManagerFactory::getProxy(identifier));
		ItemType type(EntryType);
		QStandardItem *item(new QStandardItem(proxy.isValid() ? proxy.getTitle() : QString()));
		item->setData(item->data(Qt::DisplayRole), Qt::ToolTipRole);

		if (m_isEditor)
		{
			item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
		}

		if (proxy.isFolder)
		{
			item->setData(identifier, IdentifierRole);

			type = FolderType;

			if (!m_isEditor)
			{
				item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
			}

			populateProxies(proxy.children, item, selectedProxy);
		}
		else
		{
			if (identifier.isEmpty())
			{
				type = SeparatorType;

				if (!m_isEditor)
				{
					item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
				}
			}
			else
			{
				item->setData(identifier, IdentifierRole);

				if (m_isEditor)
				{
					item->setCheckable(true);

					if (proxy.identifier == selectedProxy)
					{
						item->setData(Qt::Checked, Qt::CheckStateRole);
					}
				}
			}

			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
		}

		insertRow(item, parent, -1, type);
	}
}

UserAgentsModel::UserAgentsModel(const QString &selectedUserAgent, bool isEditor, QObject *parent) : ItemModel(parent),
	m_isEditor(isEditor)
{
	if (isEditor)
	{
		setExclusive(true);
	}

	populateUserAgents(NetworkManagerFactory::getUserAgents(), invisibleRootItem(), selectedUserAgent);
}

void UserAgentsModel::populateUserAgents(const QStringList &userAgents, QStandardItem *parent, const QString &selectedUserAgent)
{
	for (int i = 0; i < userAgents.count(); ++i)
	{
		const QString identifier(userAgents.at(i));
		const UserAgentDefinition userAgent(identifier.isEmpty() ? UserAgentDefinition() : NetworkManagerFactory::getUserAgent(identifier));
		ItemType type(EntryType);
		QList<QStandardItem*> items({new QStandardItem(userAgent.isValid() ? userAgent.getTitle() : QString())});
		items[0]->setData(items[0]->data(Qt::DisplayRole), Qt::ToolTipRole);

		if (m_isEditor)
		{
			items.append(new QStandardItem(userAgent.value));
			items[0]->setFlags(items[0]->flags() | Qt::ItemIsDragEnabled);
			items[1]->setData(items[1]->data(Qt::DisplayRole), Qt::ToolTipRole);
			items[1]->setFlags(items[1]->flags() | Qt::ItemIsDragEnabled);
		}

		if (userAgent.isFolder)
		{
			type = FolderType;

			if (!m_isEditor)
			{
				items[0]->setFlags(items[0]->flags() & ~Qt::ItemIsSelectable);
			}

			populateUserAgents(userAgent.children, items[0], selectedUserAgent);
		}
		else
		{
			if (identifier.isEmpty())
			{
				type = SeparatorType;

				if (!m_isEditor)
				{
					items[0]->setFlags(items[0]->flags() & ~Qt::ItemIsSelectable);
				}
			}
			else
			{
				items[0]->setData(identifier, IdentifierRole);
				items[0]->setData(userAgent.value, UserAgentRole);

				if (m_isEditor)
				{
					items[0]->setCheckable(true);

					if (userAgent.identifier == selectedUserAgent)
					{
						items[0]->setData(Qt::Checked, Qt::CheckStateRole);
					}
				}
			}

			items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);

			if (m_isEditor)
			{
				items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
			}
		}

		insertRow(items, parent, -1, type);
	}
}

NetworkManagerFactory::NetworkManagerFactory(QObject *parent) : QObject(parent)
{
	QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());
	const QStringList paths({QDir(Application::getApplicationDirectoryPath()).filePath(QLatin1String("certificates")), SessionsManager::getWritableDataPath(QLatin1String("certificates"))});

	for (int i = 0; i < paths.count(); ++i)
	{
		const QString path(paths.at(i));

		if (QFile::exists(path))
		{
			configuration.addCaCertificates(QDir(path).filePath(QLatin1String("*")), QSsl::Pem, QSslCertificate::PatternSyntax::Wildcard);
		}
	}

	QSslConfiguration::setDefaultConfiguration(configuration);

#if QT_VERSION < 0x060000
	connect(new QNetworkConfigurationManager(this), &QNetworkConfigurationManager::onlineStateChanged, this, &NetworkManagerFactory::onlineStateChanged);
#endif
}

void NetworkManagerFactory::createInstance()
{
	if (!m_instance)
	{
		m_proxyFactory = new NetworkProxyFactory();
		m_instance = new NetworkManagerFactory(QCoreApplication::instance());

		QNetworkProxyFactory::setApplicationProxyFactory(m_proxyFactory);

		ContentFiltersManager::createInstance();
	}
}

void NetworkManagerFactory::initialize()
{
	if (m_isInitialized)
	{
		return;
	}

	m_isInitialized = true;
	m_defaultCiphers = QSslConfiguration::defaultConfiguration().ciphers();

	for (int i = (m_defaultCiphers.count() - 1); i >= 0; --i)
	{
		const QSslCipher cipher(m_defaultCiphers.at(i));

		if (cipher.isNull() || cipher.supportedBits() < 128 || (cipher.keyExchangeMethod() == QLatin1String("DH") && cipher.supportedBits() < 1024) || cipher.authenticationMethod() == QLatin1String("PSK") || cipher.authenticationMethod() == QLatin1String("EXP") || cipher.authenticationMethod() == QLatin1String("nullptr") || cipher.encryptionMethod().startsWith(QLatin1String("RC4(")) || cipher.authenticationMethod() == QLatin1String("ADH"))
		{
			m_defaultCiphers.removeAt(i);
		}
	}

	loadProxies();
	loadUserAgents();

	m_instance->handleOptionChanged(SettingsManager::Network_AcceptLanguageOption, SettingsManager::getOption(SettingsManager::Network_AcceptLanguageOption));
	m_instance->handleOptionChanged(SettingsManager::Network_DoNotTrackPolicyOption, SettingsManager::getOption(SettingsManager::Network_DoNotTrackPolicyOption));
	m_instance->handleOptionChanged(SettingsManager::Network_EnableReferrerOption, SettingsManager::getOption(SettingsManager::Network_EnableReferrerOption));
	m_instance->handleOptionChanged(SettingsManager::Network_ProxyOption, SettingsManager::getOption(SettingsManager::Network_ProxyOption));
	m_instance->handleOptionChanged(SettingsManager::Network_WorkOfflineOption, SettingsManager::getOption(SettingsManager::Network_WorkOfflineOption));
	m_instance->handleOptionChanged(SettingsManager::Security_CiphersOption, SettingsManager::getOption(SettingsManager::Security_CiphersOption));

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, m_instance, &NetworkManagerFactory::handleOptionChanged);
}

void NetworkManagerFactory::clearCookies(int period)
{
	if (!m_cookieJar)
	{
		m_cookieJar = new DiskCookieJar(SessionsManager::getWritableDataPath(QLatin1String("cookies.dat")), QCoreApplication::instance());
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

void NetworkManagerFactory::loadProxies()
{
	m_proxies.clear();

	QFile file(SessionsManager::getReadableDataPath(QLatin1String("proxies.json")));

	if (!file.open(QIODevice::ReadOnly))
	{
		m_proxyFactory->setProxy(SettingsManager::getOption(SettingsManager::Network_ProxyOption).toString());

		return;
	}

	const QJsonArray proxiesArray(QJsonDocument::fromJson(file.readAll()).array());
	ProxyDefinition root;

	for (int i = 0; i < proxiesArray.count(); ++i)
	{
		readProxy(proxiesArray.at(i), &root);
	}

	file.close();

	m_proxies[QLatin1String("root")] = root;

	updateProxiesOption();

	m_proxyFactory->setProxy(SettingsManager::getOption(SettingsManager::Network_ProxyOption).toString());
}

void NetworkManagerFactory::loadUserAgents()
{
	m_userAgents.clear();

	QFile file(SessionsManager::getReadableDataPath(QLatin1String("userAgents.json")));

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	const QJsonArray userAgentsArray(QJsonDocument::fromJson(file.readAll()).array());
	UserAgentDefinition root;

	for (int i = 0; i < userAgentsArray.count(); ++i)
	{
		readUserAgent(userAgentsArray.at(i), &root);
	}

	file.close();

	m_userAgents[QLatin1String("root")] = root;

	updateUserAgentsOption();
}

void NetworkManagerFactory::readProxy(const QJsonValue &value, ProxyDefinition *parent)
{
	if (!value.isObject())
	{
		if (value.isString() && value.toString() == QLatin1String("separator"))
		{
			parent->children.append(QString());
		}

		return;
	}

	const QJsonObject proxyObject(value.toObject());
	const QString identifier(proxyObject.value(QLatin1String("identifier")).toString());

	if (!m_proxies.contains(identifier))
	{
		ProxyDefinition proxy;
		proxy.identifier = identifier;
		proxy.title = proxyObject.value(QLatin1String("title")).toString();

		if (proxyObject.contains(QLatin1String("children")))
		{
			proxy.isFolder = true;

			const QJsonArray childrenArray(proxyObject.value(QLatin1String("children")).toArray());

			for (int i = 0; i < childrenArray.count(); ++i)
			{
				readProxy(childrenArray.at(i), &proxy);
			}
		}
		else
		{
			const QString type(proxyObject.value(QLatin1String("type")).toString());

			if (type == QLatin1String("noProxy"))
			{
				proxy.type = ProxyDefinition::NoProxy;
			}
			else if (type == QLatin1String("manualProxy"))
			{
				proxy.type = ProxyDefinition::ManualProxy;
			}
			else if (type == QLatin1String("automaticProxy"))
			{
				proxy.type = ProxyDefinition::AutomaticProxy;
			}
			else
			{
				proxy.type = ProxyDefinition::SystemProxy;
			}

			if (proxy.type == ProxyDefinition::ManualProxy && proxyObject.contains(QLatin1String("servers")))
			{
				const QJsonArray serversArray(proxyObject.value(QLatin1String("servers")).toArray());

				for (int i = 0; i < serversArray.count(); ++i)
				{
					const QJsonObject serverObject(serversArray.at(i).toObject());
					const QString protocol(serverObject.value(QLatin1String("protocol")).toString());
					ProxyDefinition::ProxyServer server;
					server.hostName = serverObject.value(QLatin1String("hostName")).toString();
					server.port = static_cast<quint16>(serverObject.value(QLatin1String("port")).toInt());

					if (protocol == QLatin1String("http"))
					{
						proxy.servers[ProxyDefinition::HttpProtocol] = server;
					}
					else if (protocol == QLatin1String("https"))
					{
						proxy.servers[ProxyDefinition::HttpsProtocol] = server;
					}
					else if (protocol == QLatin1String("ftp"))
					{
						proxy.servers[ProxyDefinition::FtpProtocol] = server;
					}
					else if (protocol == QLatin1String("socks"))
					{
						proxy.servers[ProxyDefinition::SocksProtocol] = server;
					}
					else
					{
						proxy.servers[ProxyDefinition::AnyProtocol] = server;
					}
				}
			}

			proxy.path = proxyObject.value(QLatin1String("path")).toString();
			proxy.exceptions = proxyObject.value(QLatin1String("exceptions")).toVariant().toStringList();
			proxy.usesSystemAuthentication = proxyObject.value(QLatin1String("usesSystemAuthentication")).toBool(false);
		}

		m_proxies[identifier] = proxy;
	}

	parent->children.append(identifier);
}

void NetworkManagerFactory::readUserAgent(const QJsonValue &value, UserAgentDefinition *parent)
{
	if (!value.isObject())
	{
		if (value.isString() && value.toString() == QLatin1String("separator"))
		{
			parent->children.append(QString());
		}

		return;
	}

	const QJsonObject userAgentObject(value.toObject());
	const QString identifier(userAgentObject.value(QLatin1String("identifier")).toString());

	if (!m_userAgents.contains(identifier))
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = identifier;
		userAgent.name = userAgentObject.value(QLatin1String("name")).toString();
		userAgent.title = userAgentObject.value(QLatin1String("title")).toString();

		if (identifier == QLatin1String("default"))
		{
			userAgent.value = QLatin1String("Mozilla/5.0 {platform} {engineVersion} {applicationVersion}");
		}
		else if (userAgentObject.contains(QLatin1String("children")))
		{
			userAgent.isFolder = true;

			const QJsonArray childrenArray(userAgentObject.value(QLatin1String("children")).toArray());

			for (int i = 0; i < childrenArray.count(); ++i)
			{
				readUserAgent(childrenArray.at(i), &userAgent);
			}
		}
		else
		{
			userAgent.value = userAgentObject.value(QLatin1String("value")).toString();
		}

		m_userAgents[identifier] = userAgent;
	}

	parent->children.append(identifier);
}

void NetworkManagerFactory::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Network_AcceptLanguageOption:
			m_acceptLanguage = ((value.toString().isEmpty()) ? QLatin1String(" ") : value.toString().replace(QLatin1String("system"), QLocale::system().bcp47Name()));

			break;
		case SettingsManager::Network_DoNotTrackPolicyOption:
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

			break;
		case SettingsManager::Network_EnableReferrerOption:
			m_canSendReferrer = value.toBool();

			break;
		case SettingsManager::Network_ProxyOption:
			m_proxyFactory->setProxy(value.toString());

			break;
		case SettingsManager::Network_WorkOfflineOption:
			m_isWorkingOffline = value.toBool();

			break;
		case SettingsManager::Security_CiphersOption:
			{
				QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

				if (value.toString() == QLatin1String("default"))
				{
					configuration.setCiphers(m_defaultCiphers);
				}
				else
				{
					const QStringList selectedCiphers(value.toStringList());
					QList<QSslCipher> ciphers;
					ciphers.reserve(selectedCiphers.count());

					for (int i = 0; i < selectedCiphers.count(); ++i)
					{
						const QSslCipher cipher(selectedCiphers.at(i));

						if (!cipher.isNull())
						{
							ciphers.append(cipher);
						}
					}

					configuration.setCiphers(ciphers);
				}

				QSslConfiguration::setDefaultConfiguration(configuration);
			}

			break;
		default:
			break;
	}
}

void NetworkManagerFactory::notifyAuthenticated(QAuthenticator *authenticator, bool wasAccepted)
{
	emit m_instance->authenticated(authenticator, wasAccepted);
}

void NetworkManagerFactory::updateProxiesOption()
{
	SettingsManager::OptionDefinition proxiesOption(SettingsManager::getOptionDefinition(SettingsManager::Network_ProxyOption));
	proxiesOption.choices.clear();
	proxiesOption.choices.reserve(qRound(m_proxies.count() * 0.75));

	QMap<QString, ProxyDefinition>::iterator iterator;

	for (iterator = m_proxies.begin(); iterator != m_proxies.end(); ++iterator)
	{
		const ProxyDefinition proxy(iterator.value());

		if (!proxy.isFolder && !proxy.identifier.isEmpty())
		{
			proxiesOption.choices.append({proxy.getTitle(), proxy.identifier, {}});
		}
	}

	proxiesOption.choices.squeeze();

	SettingsManager::updateOptionDefinition(SettingsManager::Network_ProxyOption, proxiesOption);
}

void NetworkManagerFactory::updateUserAgentsOption()
{
	SettingsManager::OptionDefinition userAgentsOption(SettingsManager::getOptionDefinition(SettingsManager::Network_UserAgentOption));
	userAgentsOption.choices = {{QCoreApplication::translate("userAgents", "Default User Agent"), QLatin1String("default"), {}}};
	userAgentsOption.choices.reserve(qRound(m_userAgents.count() * 0.75));

	QMap<QString, UserAgentDefinition>::iterator iterator;

	for (iterator = m_userAgents.begin(); iterator != m_userAgents.end(); ++iterator)
	{
		const UserAgentDefinition userAgent(iterator.value());

		if (!userAgent.isFolder && !userAgent.identifier.isEmpty() && userAgent.identifier != QLatin1String("default"))
		{
			userAgentsOption.choices.append({userAgent.getTitle(), userAgent.identifier, {}});
		}
	}

	userAgentsOption.choices.squeeze();

	SettingsManager::updateOptionDefinition(SettingsManager::Network_UserAgentOption, userAgentsOption);
}

NetworkManagerFactory* NetworkManagerFactory::getInstance()
{
	return m_instance;
}

NetworkManager* NetworkManagerFactory::getNetworkManager(bool isPrivate)
{
	if (isPrivate && !m_privateNetworkManager)
	{
		m_privateNetworkManager = new NetworkManager(true, QCoreApplication::instance());
	}
	else if (!isPrivate && !m_standardNetworkManager)
	{
		m_standardNetworkManager = new NetworkManager(false, QCoreApplication::instance());
	}

	return (isPrivate ? m_privateNetworkManager : m_standardNetworkManager);
}

NetworkCache* NetworkManagerFactory::getCache()
{
	if (!m_cache)
	{
		m_cache = new NetworkCache(SessionsManager::getCachePath(), QCoreApplication::instance());
	}

	return m_cache;
}

CookieJar* NetworkManagerFactory::getCookieJar()
{
	if (!m_cookieJar)
	{
		m_cookieJar = new DiskCookieJar(SessionsManager::getWritableDataPath(QLatin1String("cookies.dat")), QCoreApplication::instance());
	}

	return m_cookieJar;
}

QNetworkReply* NetworkManagerFactory::createRequest(const QUrl &url, QNetworkAccessManager::Operation operation, bool isPrivate, QIODevice *outgoingData)
{
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());

	return getNetworkManager(isPrivate)->createRequest(operation, request, outgoingData);
}

QString NetworkManagerFactory::getAcceptLanguage()
{
	return m_acceptLanguage;
}

QString NetworkManagerFactory::getUserAgent()
{
	const WebBackend *webBackend(AddonsManager::getWebBackend());

	return (webBackend ? webBackend->getUserAgent() : QString());
}

QStringList NetworkManagerFactory::getProxies()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_proxies[QLatin1String("root")].children;
}

QStringList NetworkManagerFactory::getUserAgents()
{
	if (!m_isInitialized)
	{
		initialize();
	}

	return m_userAgents[QLatin1String("root")].children;
}

QList<QSslCipher> NetworkManagerFactory::getDefaultCiphers()
{
	return m_defaultCiphers;
}

ProxyDefinition NetworkManagerFactory::getProxy(const QString &identifier)
{
	if (!m_isInitialized)
	{
		initialize();
	}

	if (identifier.isEmpty() || !m_proxies.contains(identifier))
	{
		ProxyDefinition proxy;
		proxy.identifier = QLatin1String("systemProxy");
		proxy.title = QT_TRANSLATE_NOOP("proxies", "System Configuration");
		proxy.type = ProxyDefinition::SystemProxy;

		return proxy;
	}

	return m_proxies[identifier];
}

UserAgentDefinition NetworkManagerFactory::getUserAgent(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("custom;")))
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = QLatin1String("custom");
		userAgent.title = tr("Custom");
		userAgent.value = identifier.mid(7);

		return userAgent;
	}

	if (!m_isInitialized)
	{
		initialize();
	}

	if (identifier.isEmpty() || !m_userAgents.contains(identifier))
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = QLatin1String("default");
		userAgent.title = QT_TRANSLATE_NOOP("userAgents", "Default User Agent");
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

bool NetworkManagerFactory::usesSystemProxyAuthentication()
{
	return m_proxyFactory->usesSystemAuthentication();
}

bool NetworkManagerFactory::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (!m_proxies.isEmpty())
		{
			updateProxiesOption();
		}

		if (!m_userAgents.isEmpty())
		{
			updateUserAgentsOption();
		}
	}

	return QObject::event(event);
}

}
