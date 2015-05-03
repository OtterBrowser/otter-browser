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

#include "Console.h"
#include "NetworkAutomaticProxy.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDate>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

namespace Otter
{

QStringList NetworkAutomaticProxy::m_months = QStringList() << QLatin1String("jan") << QLatin1String("feb") << QLatin1String("mar") << QLatin1String("apr") << QLatin1String("may") << QLatin1String("jun") << QLatin1String("jul") << QLatin1String("aug") << QLatin1String("sep") << QLatin1String("oct") << QLatin1String("nov") << QLatin1String("dec");
QStringList NetworkAutomaticProxy::m_days = QStringList() << QLatin1String("mon") << QLatin1String("tue") << QLatin1String("wed") << QLatin1String("thu") << QLatin1String("fri") << QLatin1String("sat") << QLatin1String("sun");

NetworkAutomaticProxy::NetworkAutomaticProxy()
{
	m_proxies.insert(QLatin1String("ERROR"), QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::DefaultProxy));
	m_proxies.insert(QLatin1String("DIRECT"), QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy));

	m_engine.globalObject().setProperty(QLatin1String("alert"), m_engine.newFunction(alert));
	m_engine.globalObject().setProperty(QLatin1String("shExpMatch"), m_engine.newFunction(shExpMatch));
	m_engine.globalObject().setProperty(QLatin1String("dnsDomainIs"), m_engine.newFunction(dnsDomainIs));
	m_engine.globalObject().setProperty(QLatin1String("isInNet"), m_engine.newFunction(isInNet));
	m_engine.globalObject().setProperty(QLatin1String("myIpAddress"), m_engine.newFunction(myIpAddress));
	m_engine.globalObject().setProperty(QLatin1String("dnsResolve"), m_engine.newFunction(dnsResolve));
	m_engine.globalObject().setProperty(QLatin1String("isPlainHostName"), m_engine.newFunction(isPlainHostName));
	m_engine.globalObject().setProperty(QLatin1String("isResolvable"), m_engine.newFunction(isResolvable));
	m_engine.globalObject().setProperty(QLatin1String("localHostOrDomainIs"), m_engine.newFunction(localHostOrDomainIs));
	m_engine.globalObject().setProperty(QLatin1String("dnsDomainLevels"), m_engine.newFunction(dnsDomainLevels));
	m_engine.globalObject().setProperty(QLatin1String("weekdayRange"), m_engine.newFunction(weekdayRange));
	m_engine.globalObject().setProperty(QLatin1String("dateRange"), m_engine.newFunction(dateRange));
	m_engine.globalObject().setProperty(QLatin1String("timeRange"), m_engine.newFunction(timeRange));
}

QList<QNetworkProxy> NetworkAutomaticProxy::getProxy(const QString &url, const QString &host)
{
	QScriptValueList arguments;
	arguments << m_engine.toScriptValue(url) << m_engine.toScriptValue(host);

	const QScriptValue result = m_findProxy.call(m_engine.globalObject(), arguments);

	if (result.isError())
	{
		return m_proxies[QLatin1String("ERROR")];
	}

	const QString configuration = result.toString().remove(QLatin1Char(' '));

	if (!m_proxies.value(configuration).isEmpty())
	{
		return m_proxies[configuration];
	}

// proxy format: "PROXY host:port; PROXY host:port", "PROXY host:port; SOCKS host:port" etc.
// can be combination of DIRECT, PROXY, SOCKS
	const QStringList proxies = configuration.split(QLatin1Char(';'));
	QList<QNetworkProxy> proxiesForQuery;

	for (int i = 0; i < proxies.count(); ++i)
	{
		const QStringList proxy = proxies.at(i).split(QLatin1Char(':'));
		QString proxyHost = proxy.at(0);

		if (proxy.count() == 2 && proxyHost.indexOf(QLatin1String("PROXY"), Qt::CaseInsensitive) == 0)
		{
			proxiesForQuery << QNetworkProxy(QNetworkProxy::HttpProxy, proxyHost.replace(0, 5, QString()), proxy.at(1).toInt());

			continue;
		}

		if (proxy.count() == 2 && proxyHost.indexOf(QLatin1String("SOCKS"), Qt::CaseInsensitive) == 0)
		{
			proxiesForQuery << QNetworkProxy(QNetworkProxy::Socks5Proxy, proxyHost.replace(0, 5, QString()), proxy.at(1).toInt());

			continue;
		}

		if (proxy.count() == 1 && proxyHost.indexOf(QLatin1String("DIRECT"), Qt::CaseInsensitive) == 0)
		{
			proxiesForQuery << QNetworkProxy(QNetworkProxy::NoProxy);

			continue;
		}

		Console::addMessage(QCoreApplication::translate("main", "Failed to parse entry of proxy auto-config (PAC): %1").arg(proxies.at(i)), NetworkMessageCategory, ErrorMessageLevel);

		return m_proxies[QLatin1String("ERROR")];
	}

	m_proxies.insert(configuration, proxiesForQuery);

	return m_proxies[configuration];
}

QScriptValue NetworkAutomaticProxy::alert(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	Console::addMessage(context->argument(0).toString(), NetworkMessageCategory, WarningMessageLevel);

	return engine->undefinedValue();
}

QScriptValue NetworkAutomaticProxy::dnsDomainIs(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 2)
	{
		return context->throwError(QLatin1String("Function dnsDomainIs takes two arguments!"));
	}

	const QString address = context->argument(0).toString().toLower();
	const QString domain = context->argument(1).toString().toLower();

	return engine->toScriptValue(address.contains(domain));
}

QScriptValue NetworkAutomaticProxy::shExpMatch(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() != 2)
	{
		return context->throwError(QLatin1String("Function shExpMatch takes two arguments!"));
	}

	return engine->toScriptValue(QRegExp(context->argument(1).toString(), Qt::CaseInsensitive, QRegExp::Wildcard).exactMatch(context->argument(0).toString()));
}

QScriptValue NetworkAutomaticProxy::isInNet(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 3)
	{
		return context->throwError(QLatin1String("Function isInNet takes three arguments!"));
	}

	const QHostAddress address(context->argument(0).toString());
	const QHostAddress netaddress(context->argument(1).toString());
	const QHostAddress netmask(context->argument(2).toString());

	return (address.protocol() == QAbstractSocket::IPv4Protocol && (netaddress.toIPv4Address() & netmask.toIPv4Address()) == (address.toIPv4Address() & netmask.toIPv4Address()));
}

QScriptValue NetworkAutomaticProxy::myIpAddress(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(context);

	if (context->argumentCount() != 0)
	{
		return context->throwError(QLatin1String("Function myIpAddress does not take any arguments!"));
	}

	const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

	for (int i = 0; i < addresses.count(); ++i)
	{
		if (!addresses.at(i).isNull() && addresses.at(i) != QHostAddress::LocalHost && addresses.at(i) != QHostAddress::LocalHostIPv6 && addresses.at(i) != QHostAddress::Null && addresses.at(i) != QHostAddress::Broadcast && addresses.at(i) != QHostAddress::Any && addresses.at(i) != QHostAddress::AnyIPv6)
		{
			return addresses.at(i).toString();
		}
	}

	return engine->undefinedValue();
}

QScriptValue NetworkAutomaticProxy::dnsResolve(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function dnsResolve takes only one argument!"));
	}

	const QHostInfo host = QHostInfo::fromName(context->argument(0).toString());

	if (host.error() == QHostInfo::NoError)
	{
		return host.addresses().first().toString();
	}

	return engine->undefinedValue();
}

QScriptValue NetworkAutomaticProxy::isPlainHostName(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function isPlainHostName takes only one argument!"));
	}

	return (!context->argument(0).toString().contains(QLatin1Char('.')));
}

QScriptValue NetworkAutomaticProxy::isResolvable(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function isResolvable takes only one argument!"));
	}

	return (QHostInfo::fromName(context->argument(0).toString()).error() == QHostInfo::NoError);
}

QScriptValue NetworkAutomaticProxy::localHostOrDomainIs(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 2)
	{
		return context->throwError(QLatin1String("Function localHostOrDomainIs takes two arguments!"));
	}

// address "google.com" or "maps.google.com" or "www.google.com", domain ".google.com" - return true
	const QString address = QUrl(context->argument(0).toString()).host().toLower();
	QString domain = context->argument(1).toString().toLower();

	if (!address.contains(QLatin1Char('.')) || address.endsWith(domain))
	{
		return true;
	}

	return (domain.indexOf(QLatin1Char('.')) == 0 && domain.replace(0, 1, QString()) == address);
}

QScriptValue NetworkAutomaticProxy::dnsDomainLevels(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function dnsDomainLevels takes only one argument!"));
	}

	return context->argument(0).toString().remove(QLatin1String("www."), Qt::CaseInsensitive).count(QLatin1Char('.'));
}

QScriptValue NetworkAutomaticProxy::weekdayRange(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() < 1 || context->argumentCount() > 3)
	{
		return context->throwError(QLatin1String("Function weekdayRange takes 1 to 3 arguments!"));
	}

	const QString beginDay = context->argument(0).toString().toLower();
	const QString endDay = context->argument(1).toString().toLower();
	const int currentDay = getDateTime(context).date().dayOfWeek();
	int beginDayNumber = -1;
	int endDayNumber = -1;

	for (int i = 0; i < m_days.count(); ++i)
	{
		if (beginDay == m_days.at(i))
		{
			beginDayNumber = (i + 1);
		}

		if (endDay == m_days.at(i))
		{
			endDayNumber = (i + 1);
		}
	}

	if (endDayNumber == -1)
	{
		endDayNumber = beginDayNumber;
	}

	return ((beginDayNumber != -1 && endDayNumber != -1) && compareRange(beginDayNumber, endDayNumber, currentDay));
}

QScriptValue NetworkAutomaticProxy::dateRange(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() < 1 || context->argumentCount() > 7)
	{
		return context->throwError(QLatin1String("Function dateRange takes 1 to 7 arguments!"));
	}

	QList<int> arguments;
	int amount = 0;
	const QDate currentDay = getDateTime(context, &amount).date();

	for (int i = 0; i < amount; ++i)
	{
		if (context->argument(i).isString())
		{
			const int month = (m_months.indexOf(context->argument(i).toString().toLower()) + 1);

			if (month < 1)
			{
				return false;
			}

			arguments.append(month);
		}
		else
		{
			arguments.append(context->argument(i).toInt32());
		}
	}

	if (amount == 1 && arguments.at(0) > 1500)
	{
		return (currentDay.year() == arguments.at(0));
	}

	if (amount == 1 && context->argument(0).isNumber())
	{
		return (currentDay.day() == arguments.at(0));
	}

	if (amount == 1)
	{
		return (currentDay.month() == arguments.at(0));
	}

	if (amount == 2 && arguments.at(0) > 1500 && arguments.at(1) > 1500)
	{
		return compareRange(arguments.at(0), arguments.at(1), currentDay.year());
	}

	if (amount == 2 && context->argument(0).isNumber() && context->argument(1).isNumber())
	{
		return compareRange(arguments.at(0), arguments.at(1), currentDay.day());
	}

	if (amount == 2)
	{
		return compareRange(arguments.at(0), arguments.at(1), currentDay.month());
	}

	if (amount == 3)
	{
		const QDate dateOne(arguments.at(2), arguments.at(1), arguments.at(0));

		return compareRange(dateOne, dateOne, currentDay);
	}

	if (amount == 4 && arguments.at(1) > 1500 && arguments.at(3) > 1500)
	{
		return compareRange(QDate(arguments.at(1), arguments.at(0), currentDay.day()), QDate(arguments.at(3), arguments.at(2), currentDay.day()), currentDay);
	}

	if (amount == 4)
	{
		return compareRange(QDate(currentDay.year(), arguments.at(1), arguments.at(0)), QDate(currentDay.year(), arguments.at(3), arguments.at(2)), currentDay);
	}

	if (amount == 6)
	{
		return compareRange(QDate(arguments.at(2), arguments.at(1), arguments.at(0)), QDate(arguments.at(5), arguments.at(4), arguments.at(3)), currentDay);
	}

	return false;
}

QScriptValue NetworkAutomaticProxy::timeRange(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() < 1 || context->argumentCount() > 7)
	{
		return context->throwError(QLatin1String("Function timeRange takes 1 to 7 arguments!"));
	}

	QList<int> arguments;
	int amount = 0;
	const QTime currentTime = getDateTime(context, &amount).time();

	for (int i = 0; i < amount; ++i)
	{
		if (!context->argument(i).isNumber())
		{
			return false;
		}

		arguments.append(context->argument(i).toInt32());
	}

	if (amount == 1)
	{
		return compareRange(arguments.at(0), arguments.at(0), currentTime.hour());
	}

	if (amount == 2)
	{
		return compareRange(arguments.at(0), arguments.at(1), currentTime.hour());
	}

	if (amount == 4)
	{
		return compareRange(QTime(arguments.at(0), arguments.at(1)), QTime(arguments.at(2), arguments.at(3)), currentTime);
	}

	if (amount == 6)
	{
		return compareRange(QTime(arguments.at(0), arguments.at(1), arguments.at(2)), QTime(arguments.at(3), arguments.at(4), arguments.at(5)), currentTime);
	}

	return false;
}

QDateTime NetworkAutomaticProxy::getDateTime(QScriptContext *context, int *numberOfArguments)
{
	if (context->argument(context->argumentCount() - 1).toString().toLower() == QLatin1String("gmt"))
	{
		if (numberOfArguments != NULL)
		{
			*numberOfArguments = (context->argumentCount() - 1);
		}

		return QDateTime::currentDateTimeUtc();
	}

	if (numberOfArguments != NULL)
	{
		*numberOfArguments = context->argumentCount();
	}

	return QDateTime::currentDateTime();
}

bool NetworkAutomaticProxy::setup(const QString &script)
{
	if (!m_engine.canEvaluate(script) || m_engine.evaluate(script).isError())
	{
		return false;
	}

	m_findProxy = m_engine.globalObject().property(QLatin1String("FindProxyForURL"));

	return m_findProxy.isFunction();
}

bool NetworkAutomaticProxy::compareRange(const QVariant &valueOne, const QVariant &valueTwo, const QVariant &actualValue)
{
	return (actualValue >= valueOne && actualValue <= valueTwo);
}

}
