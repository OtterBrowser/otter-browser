/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "NetworkProxyFactory.h"
#include "NetworkAutomaticProxy.h"
#include "SettingsManager.h"

namespace Otter
{

NetworkProxyFactory::NetworkProxyFactory(QObject *parent) : QObject(parent), QNetworkProxyFactory(),
	m_automaticProxy(nullptr),
	m_proxyMode(ProxyDefinition::SystemProxy)
{
	optionChanged(SettingsManager::Network_ProxyModeOption);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int)));
}

NetworkProxyFactory::~NetworkProxyFactory()
{
	if (m_automaticProxy)
	{
		delete m_automaticProxy;
	}
}

void NetworkProxyFactory::optionChanged(int identifier)
{
	if ((identifier == SettingsManager::Network_ProxyModeOption && SettingsManager::getValue(identifier) == QLatin1String("automatic")) || (identifier == SettingsManager::Proxy_AutomaticConfigurationPathOption && m_proxyMode == ProxyDefinition::AutomaticProxy))
	{
		m_proxyMode = ProxyDefinition::AutomaticProxy;

		const QString path(SettingsManager::getValue(SettingsManager::Proxy_AutomaticConfigurationPathOption).toString());

		if (m_automaticProxy)
		{
			m_automaticProxy->setPath(path);
		}
		else
		{
			m_automaticProxy = new NetworkAutomaticProxy(path);
		}
	}
	else if ((identifier == SettingsManager::Network_ProxyModeOption && SettingsManager::getValue(identifier) == QLatin1String("manual")) || (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Proxy/")) && m_proxyMode == ProxyDefinition::ManualProxy))
	{
		m_proxyMode = ProxyDefinition::ManualProxy;

		m_proxies.clear();
		m_proxies[-1] = QList<QNetworkProxy>({QNetworkProxy(QNetworkProxy::NoProxy)});

		const bool useCommon(SettingsManager::getValue(SettingsManager::Proxy_UseCommonOption).toBool());
		const QList<ProxyDefinition::ProtocolType> protocols({ProxyDefinition::HttpProtocol, ProxyDefinition::HttpsProtocol, ProxyDefinition::FtpProtocol, ProxyDefinition::SocksProtocol});

		for (int i = 0; i < protocols.count(); ++i)
		{
			QString proxyName;
			QNetworkProxy::ProxyType proxyType(QNetworkProxy::HttpProxy);

			switch (protocols.at(i))
			{
				case ProxyDefinition::HttpProtocol:
					proxyName = QLatin1String("Http");

					break;
				case ProxyDefinition::HttpsProtocol:
					proxyName = QLatin1String("Https");

					break;
				case ProxyDefinition::FtpProtocol:
					proxyName = QLatin1String("Ftp");
					proxyType = QNetworkProxy::FtpCachingProxy;

					break;
				case ProxyDefinition::SocksProtocol:
					proxyName = QLatin1String("Socks");
					proxyType = QNetworkProxy::Socks5Proxy;

					break;
				default:
					break;
			}

			if (useCommon && protocols.at(i) != ProxyDefinition::SocksProtocol)
			{
				m_proxies[protocols.at(i)] = QList<QNetworkProxy>({QNetworkProxy(proxyType, SettingsManager::getValue(SettingsManager::Proxy_CommonServersOption).toString(), SettingsManager::getValue(SettingsManager::Proxy_CommonPortOption).toInt())});
			}
			else if (!proxyName.isEmpty() && SettingsManager::getValue(SettingsManager::getOptionIdentifier(QLatin1String("Proxy/Use") + proxyName)).toBool())
			{
				m_proxies[protocols.at(i)] = QList<QNetworkProxy>({QNetworkProxy(proxyType, SettingsManager::getValue(SettingsManager::getOptionIdentifier(QStringLiteral("Proxy/%1Servers").arg(proxyName))).toString(), SettingsManager::getValue(SettingsManager::getOptionIdentifier(QStringLiteral("Proxy/%1Port").arg(proxyName))).toInt())});
			}
		}

		m_proxyExceptions = SettingsManager::getValue(SettingsManager::Proxy_ExceptionsOption).toStringList();
	}
	else if (identifier == SettingsManager::Network_ProxyModeOption)
	{
		m_proxies.clear();

		const QString value(SettingsManager::getValue(identifier).toString());

		if (value == QLatin1String("system"))
		{
			m_proxyMode = ProxyDefinition::SystemProxy;
		}
		else
		{
			m_proxyMode = ProxyDefinition::NoProxy;
			m_proxies[-1] = QList<QNetworkProxy>({QNetworkProxy(QNetworkProxy::NoProxy)});
		}
	}
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
	switch (m_proxyMode)
	{
		case ProxyDefinition::SystemProxy:
			return QNetworkProxyFactory::systemProxyForQuery(query);

		case ProxyDefinition::ManualProxy:
			{
				const QString host(query.peerHostName());

				for (int i = 0; i < m_proxyExceptions.count(); ++i)
				{
					if (m_proxyExceptions.at(i).contains(QLatin1Char('/')))
					{
						const QHostAddress address(host);
						const QPair<QHostAddress, int> subnet(QHostAddress::parseSubnet(m_proxyExceptions.at(i)));

						if (!address.isNull() && subnet.second != -1 && address.isInSubnet(subnet))
						{
							return m_proxies[-1];
						}
					}
					else if (host.contains(m_proxyExceptions.at(i), Qt::CaseInsensitive))
					{
						return m_proxies[-1];
					}
				}

				if (m_proxies.contains(ProxyDefinition::SocksProtocol))
				{
					return m_proxies[ProxyDefinition::SocksProtocol];
				}

				const QString protocol(query.protocolTag().toLower());

				if (protocol == QLatin1String("http") && m_proxies.contains(ProxyDefinition::HttpProtocol))
				{
					return m_proxies[ProxyDefinition::HttpProtocol];
				}

				if (protocol == QLatin1String("https") && m_proxies.contains(ProxyDefinition::HttpsProtocol))
				{
					return m_proxies[ProxyDefinition::HttpsProtocol];
				}

				if (protocol == QLatin1String("ftp") && m_proxies.contains(ProxyDefinition::FtpProtocol))
				{
					return m_proxies[ProxyDefinition::FtpProtocol];
				}

				return m_proxies[-1];
			}

			break;
		case ProxyDefinition::AutomaticProxy:
			if (m_automaticProxy && m_automaticProxy->isValid())
			{
				return m_automaticProxy->getProxy(query.url().toString(), query.peerHostName());
			}

			return QNetworkProxyFactory::systemProxyForQuery(query);
		default:
			break;
	}

	return m_proxies[-1];
}

}

