/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "NetworkProxyFactory.h"
#include "SettingsManager.h"

#include <QtNetwork/QNetworkProxy>
#include <QFile>

namespace Otter
{

NetworkProxyFactory::NetworkProxyFactory() : QObject(), QNetworkProxyFactory()
{
	optionChanged(QLatin1String("Proxy/ProxyMode"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery & query) 
{
	if (m_proxyMode == ManualProxy)
	{
		const QString protocol = query.protocolTag().toLower();

		if (!m_proxies[QNetworkProxy::Socks5Proxy].isEmpty())
		{
			return m_proxies[QNetworkProxy::Socks5Proxy];
		}
		else if (protocol == QLatin1String("http") && !m_proxies[QNetworkProxy::HttpProxy].isEmpty())
		{
			return m_proxies[QNetworkProxy::HttpProxy];
		}
		else if (protocol == QLatin1String("https") && !m_proxies[QNetworkProxy::HttpCachingProxy].isEmpty())
		{
			return m_proxies[QNetworkProxy::HttpCachingProxy];
		}
		else if (protocol == QLatin1String("ftp") && !m_proxies[QNetworkProxy::FtpCachingProxy].isEmpty())
		{
			return m_proxies[QNetworkProxy::FtpCachingProxy];
		}
	}

	return m_proxies[QNetworkProxy::DefaultProxy];
}

void NetworkProxyFactory::setManualProxy()
{
	if (SettingsManager::getValue(QLatin1String("Proxy/UseCommon")).toBool())
	{
		setupProxy(QNetworkProxy::DefaultProxy, QLatin1String("Proxy/CommonAddress"), QLatin1String("Proxy/CommonPort"));
	}

	if (SettingsManager::getValue(QLatin1String("Proxy/UseHttp")).toBool())
	{
		setupProxy(QNetworkProxy::HttpProxy, QLatin1String("Proxy/HttpAddress"), QLatin1String("Proxy/HttpPort"));
	}

	if (SettingsManager::getValue(QLatin1String("Proxy/UseHttps")).toBool())
	{
		setupProxy(QNetworkProxy::HttpCachingProxy, QLatin1String("Proxy/HttpsAddress"), QLatin1String("Proxy/HttpsPort"));
	}

	if (SettingsManager::getValue(QLatin1String("Proxy/UseFtp")).toBool())
	{
		setupProxy(QNetworkProxy::FtpCachingProxy, QLatin1String("Proxy/FtpAddress"), QLatin1String("Proxy/FtpPort"));
	}

	if (SettingsManager::getValue(QLatin1String("Proxy/UseSocks")).toBool())
	{
		setupProxy(QNetworkProxy::Socks5Proxy, QLatin1String("Proxy/SocksAddress"), QLatin1String("Proxy/SocksPort"));
	}
}

void NetworkProxyFactory::setSystemProxy()
{
	const QString proxyTestAddress = SettingsManager::getValue(QLatin1String("Proxy/AutomaticTestAddress")).toString();

	QList<QNetworkProxy> listOfProxies = systemProxyForQuery(QNetworkProxyQuery(QUrl(proxyTestAddress)));
	if (listOfProxies.size())
	{
		m_proxies[QNetworkProxy::DefaultProxy] << listOfProxies;
	}
}

void NetworkProxyFactory::setPACProxy()
{
	QFile scriptFile(SettingsManager::getValue(QLatin1String("Network/PACFilePath")).toString());
	const QString loadedScript = scriptFile.readAll();
	scriptFile.close();

	// TODO:
}

void NetworkProxyFactory::setupProxy(const QNetworkProxy::ProxyType initType, const QString address, const QString port)
{
	const QString host = SettingsManager::getValue(address).toString();
	const int portNumber = SettingsManager::getValue(port).toInt();

	if (initType == QNetworkProxy::DefaultProxy) // Common proxy
	{
		m_proxies[initType].clear();
		m_proxies[initType] << QNetworkProxy(QNetworkProxy::HttpProxy, host, portNumber);
		m_proxies[initType] << QNetworkProxy(QNetworkProxy::FtpCachingProxy, host, portNumber);
		m_proxies[initType] << QNetworkProxy(QNetworkProxy::Socks5Proxy, host, portNumber);
		return;
	}
	else if (initType == QNetworkProxy::HttpCachingProxy) // Https proxy
	{
		m_proxies[initType] << QNetworkProxy(QNetworkProxy::HttpProxy, host, portNumber);
		return;
	}
	
	m_proxies[initType] << QNetworkProxy(initType, host, portNumber);
}

void NetworkProxyFactory::optionChanged(const QString &option)
{
	if (option == QLatin1String("Proxy/ProxyMode"))
	{
		m_proxies.clear();
		const QString value = SettingsManager::getValue(option).toString();

		if (value == QLatin1String("manual"))
		{
			m_proxyMode = ManualProxy;
			m_proxies[QNetworkProxy::DefaultProxy] << QNetworkProxy(QNetworkProxy::DefaultProxy);
			setManualProxy();
		}
		else if (value == QLatin1String("system"))
		{
			m_proxyMode = SystemProxy;
			setSystemProxy();
		}
		else if (value == QLatin1String("automatic"))
		{
			m_proxyMode = AutomaticProxy;
			setPACProxy();
		}
		else
		{
			m_proxyMode = NoProxy;
			m_proxies[QNetworkProxy::DefaultProxy] << QNetworkProxy(QNetworkProxy::NoProxy);
		}
	}
	else if (option.contains(QLatin1String("Proxy/")) && m_proxyMode == ManualProxy)
	{
		m_proxies.clear();
		m_proxies[QNetworkProxy::DefaultProxy] << QNetworkProxy(QNetworkProxy::DefaultProxy);
		setManualProxy();
	}
}

}

