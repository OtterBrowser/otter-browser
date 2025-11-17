/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_NETWORKPROXYFACTORY_H
#define OTTER_NETWORKPROXYFACTORY_H

#include "NetworkManagerFactory.h"

#include <QtNetwork/QNetworkProxy>

namespace Otter
{

class NetworkAutomaticProxy;

class NetworkProxyFactory final : public QObject, public QNetworkProxyFactory
{
	Q_OBJECT

public:
	explicit NetworkProxyFactory(QObject *parent = nullptr);
	~NetworkProxyFactory();

	void setProxy(const QString &identifier);
	QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query) override;
	bool usesSystemAuthentication();

protected:
	QNetworkProxy::ProxyType getProxyType(ProxyDefinition::ProtocolType protocol);

private:
	NetworkAutomaticProxy *m_automaticProxy;
	ProxyDefinition m_definition;
	QList<QNetworkProxy> m_noProxy;
	QMap<int, QList<QNetworkProxy> > m_proxies;
};

}

#endif
