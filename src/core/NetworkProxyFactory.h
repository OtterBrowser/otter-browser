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

#ifndef OTTER_NETWORKPROXYFACTORY_H
#define OTTER_NETWORKPROXYFACTORY_H

#include "NetworkAutomaticProxy.h"

#include <QtNetwork/QNetworkProxy>

namespace Otter
{

class NetworkProxyFactory : public QObject, public QNetworkProxyFactory
{
	Q_OBJECT
	Q_ENUMS(ProxyMode)

public:
	explicit NetworkProxyFactory();
	~NetworkProxyFactory();

	enum ProxyMode
	{
		NoProxy = 0,
		ManualProxy = 1,
		SystemProxy = 2,
		AutomaticProxy = 3
	};

	QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query);

protected slots:
	void optionChanged(const QString &option);

private:
	NetworkAutomaticProxy *m_automaticProxy;
	QHash<QString, QList<QNetworkProxy> > m_proxies;
	ProxyMode m_proxyMode;
};

}

#endif
