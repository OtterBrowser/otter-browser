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

#include "NetworkAutomaticProxy.h"

#include <QtCore/QDate>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include <QtWidgets/QMessageBox>

namespace Otter
{

QStringList NetworkAutomaticProxy::m_months = QStringList() << QLatin1String("jan") << QLatin1String("feb") << QLatin1String("mar") << QLatin1String("apr") << QLatin1String("may")
	<< QLatin1String("jun") << QLatin1String("jul") << QLatin1String("aug") << QLatin1String("sep") << QLatin1String("oct") << QLatin1String("nov") << QLatin1String("dec");

QStringList NetworkAutomaticProxy::m_days = QStringList() << QLatin1String("mon") << QLatin1String("tue") << QLatin1String("wed") << QLatin1String("thu")
	<< QLatin1String("fri") << QLatin1String("sat") << QLatin1String("sun");

NetworkAutomaticProxy::NetworkAutomaticProxy()
{
	m_proxyList.insert(QLatin1String("ERROR"), QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::DefaultProxy));
	m_proxyList.insert(QLatin1String("DIRECT"), QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy));

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

bool NetworkAutomaticProxy::setup(const QString &loadedScript)
{
	if (!m_engine.canEvaluate(loadedScript) || m_engine.evaluate(loadedScript).isError())
	{
		return false;
	}

	m_findProxy = m_engine.globalObject().property(QLatin1String("FindProxyForURL"));

	if (!m_findProxy.isFunction())
	{
		return false;
	}

	return true;
}

QList<QNetworkProxy> NetworkAutomaticProxy::getProxy(const QString &url, const QString &host)
{
	QScriptValueList args;
	args << m_engine.toScriptValue(url) << m_engine.toScriptValue(host);

	const QScriptValue returnedValue = m_findProxy.call(m_engine.globalObject(), args);

	if (returnedValue.isError())
	{
		return m_proxyList[QLatin1String("ERROR")];
	}

	const QString proxyConfiguration = returnedValue.toString().remove(QLatin1Char(' '));

	if (!m_proxyList.value(proxyConfiguration).isEmpty()) // cached proxy list with proxies created in previous requests
	{
		return m_proxyList[proxyConfiguration];
	}
	
	// proxy format: "PROXY host:port; PROXY host:port", "PROXY host:port; SOCKS host:port" etc.
	// can be combination of DIRECT, PROXY, SOCKS
	QList<QNetworkProxy> currentList;
	const QStringList proxies = proxyConfiguration.split(QLatin1Char(';'));

	for (int i = 0; i < proxies.count(); ++i)
	{
		const QStringList singleProxy = proxies.at(i).split(QLatin1Char(':'));
		QString proxyHost = singleProxy.at(0);

		if (singleProxy.count() == 2 && proxyHost.indexOf(QLatin1String("PROXY"), Qt::CaseInsensitive) == 0)
		{
			currentList << QNetworkProxy(QNetworkProxy::HttpProxy, proxyHost.replace(0, 5, QLatin1String("")), singleProxy.at(1).toInt());
			continue;
		}

		if (singleProxy.count() == 2 && proxyHost.indexOf(QLatin1String("SOCKS"), Qt::CaseInsensitive) == 0)
		{
			currentList << QNetworkProxy(QNetworkProxy::Socks5Proxy, proxyHost.replace(0, 5, QLatin1String("")), singleProxy.at(1).toInt());
			continue;
		}

		if (singleProxy.count() == 1 && proxyHost.indexOf(QLatin1String("DIRECT"), Qt::CaseInsensitive) == 0)
		{
			currentList << QNetworkProxy(QNetworkProxy::NoProxy);
			continue;
		}

		//TODO: something is not valid here (show error to debug console)
		return m_proxyList[QLatin1String("ERROR")];
	}

	m_proxyList.insert(proxyConfiguration, currentList);

	return m_proxyList[proxyConfiguration];
}

QScriptValue NetworkAutomaticProxy::alert(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	// will be replaced with debug console in future
	QMessageBox* msgBox = new QMessageBox();
	msgBox->setAttribute(Qt::WA_DeleteOnClose);
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setWindowTitle(QLatin1String("Proxy alert"));
	msgBox->setText(context->argument(0).toString());
	msgBox->open(NULL, SLOT(msgBoxClosed(QAbstractButton*)));

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

	const QRegExp expression(context->argument(1).toString(), Qt::CaseInsensitive, QRegExp::Wildcard);

	return engine->toScriptValue(expression.exactMatch(context->argument(0).toString()));
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

	if (address.protocol() == QAbstractSocket::IPv4Protocol)
	{
		if ((netaddress.toIPv4Address() & netmask.toIPv4Address()) == (address.toIPv4Address() & netmask.toIPv4Address()))
		{
			return true;
		}
	}

	return false;
}

QScriptValue NetworkAutomaticProxy::myIpAddress(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(context);

	if (context->argumentCount() != 0)
	{
		return context->throwError(QLatin1String("Function myIpAddress does not take any arguments!"));
	}

	foreach (QHostAddress address, QNetworkInterface::allAddresses())
	{
		if (!address.isNull() && address != QHostAddress::LocalHost && address != QHostAddress::LocalHostIPv6 && address != QHostAddress::Null
			&& address != QHostAddress::Broadcast && address != QHostAddress::Any && address != QHostAddress::AnyIPv6)
		{
			return address.toString();
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

	const QHostInfo info = QHostInfo::fromName(context->argument(0).toString());

	if (info.error() == QHostInfo::NoError) 
	{
		return info.addresses().first().toString();
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

	if (!context->argument(0).toString().contains(QLatin1Char('.')))
	{
		return true;
	}

	return false;
}

QScriptValue NetworkAutomaticProxy::isResolvable(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function isResolvable takes only one argument!"));
	}

	const QHostInfo info = QHostInfo::fromName(context->argument(0).toString());

	if (info.error() == QHostInfo::NoError) 
	{
		return true;
	}

	return false;
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

	if (domain.indexOf(QLatin1Char('.')) == 0)
	{
		if (domain.replace(0, 1, QLatin1String("")) == address)
		{
			return true;
		}
	}

	return false;
}

QScriptValue NetworkAutomaticProxy::dnsDomainLevels(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() != 1)
	{
		return context->throwError(QLatin1String("Function dnsDomainLevels takes only one argument!"));
	}

	const QString address = context->argument(0).toString().remove(QLatin1String("www."), Qt::CaseInsensitive);

	return address.count(QLatin1Char('.'));
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
	int dayCounter = 1;

	foreach (QString day, m_days)
	{
		if (beginDay.compare(day) == 0)
		{
			beginDayNumber = dayCounter;
		}

		if (endDay.compare(day) == 0)
		{
			endDayNumber = dayCounter;
		}

		dayCounter++;
	}

	if (endDayNumber == -1) // weekdayRange("TUE");
	{
		endDayNumber = beginDayNumber;
	}

	if ((beginDayNumber != -1 && endDayNumber != -1) && compareRange(beginDayNumber, endDayNumber, currentDay))
	{
		return true;
	}

	return false;
}

QScriptValue NetworkAutomaticProxy::dateRange(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(engine);

	if (context->argumentCount() < 1 || context->argumentCount() > 7)
	{
		return context->throwError(QLatin1String("Function dateRange takes 1 to 7 arguments!"));
	}

	int numberOfArguments = 0;
	QList<int> listOfArguments;
	const QDate currentDay = getDateTime(context, &numberOfArguments).date();
	
	for (int i = 0; i < numberOfArguments; ++i)
	{
		if (context->argument(i).isString())
		{
			const int monthIndex = m_months.indexOf(context->argument(i).toString().toLower());

			if (monthIndex == -1)
			{
				return false;
			}

			listOfArguments.append(monthIndex + 1);
		}
		else
		{
			listOfArguments.append(context->argument(i).toInt32());
		}
	}

	if (numberOfArguments == 1 && listOfArguments.at(0) > 1500)
	{
		if (currentDay.year() == listOfArguments.at(0))
		{
			return true;
		}

		return false;
	}

	if (numberOfArguments == 1 && context->argument(0).isNumber())
	{
		if (currentDay.day() == listOfArguments.at(0))
		{
			return true;
		}

		return false;
	}

	if (numberOfArguments == 1) 
	{
		if (currentDay.month() == listOfArguments.at(0))
		{
			return true;
		}

		return false;
	}

	if (numberOfArguments == 2 && listOfArguments.at(0) > 1500 && listOfArguments.at(1) > 1500)
	{
		return compareRange(listOfArguments.at(0), listOfArguments.at(1), currentDay.year());
	}

	if (numberOfArguments == 2 && context->argument(0).isNumber() && context->argument(1).isNumber())
	{
		return compareRange(listOfArguments.at(0), listOfArguments.at(1), currentDay.day());
	}

	if (numberOfArguments == 2)
	{
		return compareRange(listOfArguments.at(0), listOfArguments.at(1), currentDay.month());
	}

	if (numberOfArguments == 3)
	{
		const QDate dateOne (listOfArguments.at(2), listOfArguments.at(1), listOfArguments.at(0));

		return compareRange(dateOne, dateOne, currentDay);
	}

	if (numberOfArguments == 4 && listOfArguments.at(1) > 1500 && listOfArguments.at(3) > 1500)
	{
		const QDate dateOne (listOfArguments.at(1), listOfArguments.at(0), currentDay.day());
		const QDate dateTwo (listOfArguments.at(3), listOfArguments.at(2), currentDay.day());
		
		return compareRange(dateOne, dateTwo, currentDay);
	}

	if (numberOfArguments == 4)
	{
		const QDate dateOne (currentDay.year(), listOfArguments.at(1), listOfArguments.at(0));
		const QDate dateTwo (currentDay.year(), listOfArguments.at(3), listOfArguments.at(2));

		return compareRange(dateOne, dateTwo, currentDay);
	}

	if (numberOfArguments == 6)
	{
		const QDate dateOne (listOfArguments.at(2), listOfArguments.at(1), listOfArguments.at(0));
		const QDate dateTwo (listOfArguments.at(5), listOfArguments.at(4), listOfArguments.at(3));

		return compareRange(dateOne, dateTwo, currentDay);
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

	int numberOfArguments = 0;
	QList<int> listOfArguments;
	const QTime currentTime = getDateTime(context, &numberOfArguments).time();

	for (int i = 0; i < numberOfArguments; ++i)
	{
		if (!context->argument(i).isNumber())
		{
			return false;
		}

		listOfArguments.append(context->argument(i).toInt32());
	}

	if (numberOfArguments == 1)
	{
		return compareRange(listOfArguments.at(0), listOfArguments.at(0), currentTime.hour());
	}

	if (numberOfArguments == 2)
	{
		return compareRange(listOfArguments.at(0), listOfArguments.at(1), currentTime.hour());
	}

	if (numberOfArguments == 4)
	{
		const QTime timeOne (listOfArguments.at(0), listOfArguments.at(1));
		const QTime timeTwo (listOfArguments.at(2), listOfArguments.at(3));

		return compareRange(timeOne, timeTwo, currentTime);
	}

	if (numberOfArguments == 6)
	{
		const QTime timeOne (listOfArguments.at(0), listOfArguments.at(1), listOfArguments.at(2));
		const QTime timeTwo (listOfArguments.at(3), listOfArguments.at(4), listOfArguments.at(5));

		return compareRange(timeOne, timeTwo, currentTime);
	}

	return false;
}

bool NetworkAutomaticProxy::compareRange(QVariant valueOne, QVariant valueTwo, QVariant actualValue)
{
	if (actualValue >= valueOne && actualValue <= valueTwo)
	{
		return true;
	}

	return false;
}

QDateTime NetworkAutomaticProxy::getDateTime(QScriptContext *context, int *numberOfArguments)
{
	if (context->argument(context->argumentCount() - 1).toString().toLower() == QLatin1String("gmt"))
	{
		if (numberOfArguments != NULL)
		{
			*numberOfArguments = context->argumentCount() - 1;
		}

		return QDateTime::currentDateTimeUtc();
	}
	else
	{
		if (numberOfArguments != NULL)
		{
			*numberOfArguments = context->argumentCount();
		}

		return QDateTime::currentDateTime();
	}
}

}
