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

#include <QtCore/QFile>
#include <QtNetwork/QNetworkProxy>

namespace Otter
{

NetworkProxyFactory::NetworkProxyFactory() : QObject(), QNetworkProxyFactory(),
	m_automaticProxy(NULL),
	m_proxyMode(SystemProxy)
{
	optionChanged(QLatin1String("Network/ProxyMode"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

NetworkProxyFactory::~NetworkProxyFactory()
{
	if (m_automaticProxy)
	{
		delete m_automaticProxy;
	}
}

void NetworkProxyFactory::optionChanged(const QString &option)
{
	if ((option == QLatin1String("Network/ProxyMode") && SettingsManager::getValue(option) == QLatin1String("automatic")) || (option == QLatin1String("Proxy/AutomaticConfigurationPath") && m_proxyMode == AutomaticProxy))
	{
		m_proxyMode = AutomaticProxy;

		if (!m_automaticProxy)
		{
			m_automaticProxy = new NetworkAutomaticProxy();
		}

		QFile file(SettingsManager::getValue(QLatin1String("Proxy/AutomaticConfigurationPath")).toString());

		if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || !m_automaticProxy->setup(file.readAll()))
		{
//TODO: there was an error during opening or parsing the file (send message to debug console)
			m_proxyMode = SystemProxy;
		}

		file.close();
	}
	else if ((option == QLatin1String("Network/ProxyMode") && SettingsManager::getValue(option) == QLatin1String("manual")) || (option.startsWith(QLatin1String("Proxy/")) && m_proxyMode == ManualProxy))
	{
		m_proxyMode = ManualProxy;

		m_proxies.clear();
		m_proxies[QNetworkProxy::NoProxy] << QNetworkProxy(QNetworkProxy::NoProxy);

		const bool useCommon = SettingsManager::getValue(QLatin1String("Proxy/UseCommon")).toBool();
		QList<QPair<QNetworkProxy::ProxyType, QString> > proxyTypes;
		proxyTypes << qMakePair(QNetworkProxy::HttpProxy, QLatin1String("Http")) << qMakePair(QNetworkProxy::HttpCachingProxy, QLatin1String("Https")) << qMakePair(QNetworkProxy::FtpCachingProxy, QLatin1String("Ftp")) << qMakePair(QNetworkProxy::Socks5Proxy, QLatin1String("Proxy/Socks"));

		for (int i = 0; i < proxyTypes.count(); ++i)
		{
			if (useCommon && proxyTypes.at(i).first != QNetworkProxy::Socks5Proxy)
			{
				m_proxies[proxyTypes.at(i).first] << QNetworkProxy(proxyTypes.at(i).first, SettingsManager::getValue(QStringLiteral("Proxy/CommonServers")).toString(), SettingsManager::getValue(QStringLiteral("Proxy/CommonPort")).toInt());
			}

			if (SettingsManager::getValue(QStringLiteral("Proxy/Use%1").arg(proxyTypes.at(i).second)).toBool())
			{
				m_proxies[proxyTypes.at(i).first] << QNetworkProxy(proxyTypes.at(i).first, SettingsManager::getValue(QStringLiteral("Proxy/%1Servers").arg(proxyTypes.at(i).second)).toString(), SettingsManager::getValue(QStringLiteral("Proxy/%1Port").arg(proxyTypes.at(i).second)).toInt());
			}
		}
	}
	else if (option == QLatin1String("Network/ProxyMode"))
	{
		m_proxies.clear();

		const QString value = SettingsManager::getValue(option).toString();

		if (value == QLatin1String("system"))
		{
			m_proxyMode = SystemProxy;
		}
		else
		{
			m_proxyMode = NoProxy;
			m_proxies[QNetworkProxy::NoProxy] << QNetworkProxy(QNetworkProxy::NoProxy);
		}
	}
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
	if (m_proxyMode == SystemProxy)
	{
		return QNetworkProxyFactory::systemProxyForQuery(query);
	}

	if (m_proxyMode == ManualProxy)
	{
		const QString protocol = query.protocolTag().toLower();

		if (m_proxies.contains(QNetworkProxy::Socks5Proxy))
		{
			return m_proxies[QNetworkProxy::Socks5Proxy];
		}
		else if (protocol == QLatin1String("http") && m_proxies.contains(QNetworkProxy::HttpProxy))
		{
			return m_proxies[QNetworkProxy::HttpProxy];
		}
		else if (protocol == QLatin1String("https") && m_proxies.contains(QNetworkProxy::HttpCachingProxy))
		{
			return m_proxies[QNetworkProxy::HttpCachingProxy];
		}
		else if (protocol == QLatin1String("ftp") && m_proxies.contains(QNetworkProxy::FtpCachingProxy))
		{
			return m_proxies[QNetworkProxy::FtpCachingProxy];
		}
		else
		{
			return m_proxies[QNetworkProxy::NoProxy];
		}
	}

	if (m_proxyMode == AutomaticProxy && m_automaticProxy)
	{
		return m_automaticProxy->getProxy(query.url().toString(), query.peerHostName());
	}

	return m_proxies[QNetworkProxy::NoProxy];
}

}

