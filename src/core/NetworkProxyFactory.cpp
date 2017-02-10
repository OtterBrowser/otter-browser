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
#include "Console.h"
#include "NetworkAutomaticProxy.h"
#include "NetworkManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtNetwork/QNetworkProxy>

namespace Otter
{

NetworkProxyFactory::NetworkProxyFactory(QObject *parent) : QObject(parent), QNetworkProxyFactory(),
	m_automaticProxy(nullptr),
	m_pacNetworkReply(nullptr),
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

		if (!m_automaticProxy)
		{
			m_automaticProxy = new NetworkAutomaticProxy();
		}

		const QString path(SettingsManager::getValue(SettingsManager::Proxy_AutomaticConfigurationPathOption).toString());

		if (QFile::exists(path))
		{
			QFile file(path);

			if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || !m_automaticProxy->setup(file.readAll()))
			{
				Console::addMessage(tr("Failed to load proxy auto-config (PAC): %1").arg(file.errorString()), Console::NetworkCategory, Console::ErrorLevel, path);

				m_proxyMode = ProxyDefinition::SystemProxy;
			}

			file.close();
		}
		else
		{
			const QUrl url(path);

			if (url.isValid())
			{
				QNetworkRequest request(url);
				request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

				m_pacNetworkReply = NetworkManagerFactory::getNetworkManager()->get(request);

				connect(m_pacNetworkReply, SIGNAL(finished()), this, SLOT(setupAutomaticProxy()));
			}
			else
			{
				Console::addMessage(tr("Failed to load proxy auto-config (PAC). Invalid URL: %1").arg(url.url()), Console::NetworkCategory, Console::ErrorLevel);

				m_proxyMode = ProxyDefinition::SystemProxy;
			}
		}
	}
	else if ((identifier == SettingsManager::Network_ProxyModeOption && SettingsManager::getValue(identifier) == QLatin1String("manual")) || (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Proxy/")) && m_proxyMode == ProxyDefinition::ManualProxy))
	{
		m_proxyMode = ProxyDefinition::ManualProxy;

		m_proxies.clear();
		m_proxies[-1] = QList<QNetworkProxy>({QNetworkProxy(QNetworkProxy::NoProxy)});

		const bool useCommon(SettingsManager::getValue(SettingsManager::Proxy_UseCommonOption).toBool());
		const QList<QPair<ProxyDefinition::ProtocolType, QNetworkProxy::ProxyType> > proxyTypes({qMakePair(ProxyDefinition::HttpProtocol, QNetworkProxy::HttpProxy), qMakePair(ProxyDefinition::HttpsProtocol, QNetworkProxy::HttpProxy), qMakePair(ProxyDefinition::FtpProtocol, QNetworkProxy::FtpCachingProxy), qMakePair(ProxyDefinition::SocksProtocol, QNetworkProxy::Socks5Proxy)});

		for (int i = 0; i < proxyTypes.count(); ++i)
		{
			if (useCommon && proxyTypes.at(i).second != QNetworkProxy::Socks5Proxy)
			{
				m_proxies[proxyTypes.at(i).first] = QList<QNetworkProxy>({QNetworkProxy(proxyTypes.at(i).second, SettingsManager::getValue(SettingsManager::Proxy_CommonServersOption).toString(), SettingsManager::getValue(SettingsManager::Proxy_CommonPortOption).toInt())});
			}

			QString proxyName;

			switch (proxyTypes.at(i).first)
			{
				case ProxyDefinition::HttpProtocol:
					proxyName = QLatin1String("Http");

					break;
				case ProxyDefinition::HttpsProtocol:
					proxyName = QLatin1String("Https");

					break;
				case ProxyDefinition::FtpProtocol:
					proxyName = QLatin1String("Ftp");

					break;
				case ProxyDefinition::SocksProtocol:
					proxyName = QLatin1String("Socks");

					break;
				default:
					break;
			}

			if (!proxyName.isEmpty() && SettingsManager::getValue(SettingsManager::getOptionIdentifier(QLatin1String("Proxy/Use") + proxyName)).toBool())
			{
				m_proxies[proxyTypes.at(i).first] = QList<QNetworkProxy>({QNetworkProxy(proxyTypes.at(i).second, SettingsManager::getValue(SettingsManager::getOptionIdentifier(QStringLiteral("Proxy/%1Servers").arg(proxyName))).toString(), SettingsManager::getValue(SettingsManager::getOptionIdentifier(QStringLiteral("Proxy/%1Port").arg(proxyName))).toInt())});
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

void NetworkProxyFactory::setupAutomaticProxy()
{
	if (m_pacNetworkReply->error() != QNetworkReply::NoError || !m_automaticProxy->setup(m_pacNetworkReply->readAll()))
	{
		Console::addMessage(tr("Failed to load proxy auto-config (PAC): %1").arg(m_pacNetworkReply->errorString()), Console::NetworkCategory, Console::ErrorLevel, m_pacNetworkReply->url().url());

		m_proxyMode = ProxyDefinition::SystemProxy;
	}

	m_pacNetworkReply->deleteLater();
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
			if (m_automaticProxy)
			{
				return m_automaticProxy->getProxy(query.url().toString(), query.peerHostName());
			}

			break;
		default:
			break;
	}

	return m_proxies[-1];
}

}

