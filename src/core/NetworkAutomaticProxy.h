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

#ifndef OTTER_NETWORKAUTOMATICPROXY_H
#define OTTER_NETWORKAUTOMATICPROXY_H

#include <QtNetwork/QNetworkProxy>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

namespace Otter
{

class NetworkAutomaticProxy
{
public:
	explicit NetworkAutomaticProxy();

	QList<QNetworkProxy> getProxy(const QString &url, const QString &host);
	bool setup(const QString &script);

protected:
	static QScriptValue alert(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue dnsDomainIs(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue isInNet(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue myIpAddress(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue dnsResolve(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue isPlainHostName(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue shExpMatch(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue isResolvable(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue localHostOrDomainIs(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue dnsDomainLevels(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue weekdayRange(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue dateRange(QScriptContext *context, QScriptEngine *engine);
	static QScriptValue timeRange(QScriptContext *context, QScriptEngine *engine);
	static QDateTime getDateTime(QScriptContext *context, int *numberOfArguments = NULL);
	static bool compareRange(const QVariant &valueOne, const QVariant &valueTwo, const QVariant &actualValue);

private:
	QScriptEngine m_engine;
	QScriptValue m_findProxy;
	QHash<QString, QList<QNetworkProxy> > m_proxies;

	static QStringList m_months;
	static QStringList m_days;
};

}

#endif
