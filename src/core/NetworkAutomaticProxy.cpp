/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "NetworkAutomaticProxy.h"
#include "Console.h"
#include "Job.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

namespace Otter
{

QStringList PacUtils::m_months = {QLatin1String("jan"), QLatin1String("feb"), QLatin1String("mar"), QLatin1String("apr"), QLatin1String("may"), QLatin1String("jun"), QLatin1String("jul"), QLatin1String("aug"), QLatin1String("sep"), QLatin1String("oct"), QLatin1String("nov"), QLatin1String("dec")};
QStringList PacUtils::m_days = {QLatin1String("mon"), QLatin1String("tue"), QLatin1String("wed"), QLatin1String("thu"), QLatin1String("fri"), QLatin1String("sat"), QLatin1String("sun")};

PacUtils::PacUtils(QObject *parent) : QObject(parent)
{
}

void PacUtils::alert(const QString &message) const
{
	Console::addMessage(message, Console::NetworkCategory, Console::DebugLevel);
}

QString PacUtils::dnsResolve(const QString &host) const
{
	const QHostInfo hostInformation(QHostInfo::fromName(host));
	const QList<QHostAddress> addresses(hostInformation.addresses());

	if (hostInformation.error() == QHostInfo::NoError && !addresses.isEmpty())
	{
		return addresses.first().toString();
	}

	return {};
}

QString PacUtils::myIpAddress() const
{
	const QList<QHostAddress> addresses(QNetworkInterface::allAddresses());

	for (int i = 0; i < addresses.count(); ++i)
	{
		const QHostAddress address(addresses.at(i));

		if (!address.isNull() && address != QHostAddress::LocalHost && address != QHostAddress::LocalHostIPv6 && address != QHostAddress::Null && address != QHostAddress::Broadcast && address != QHostAddress::Any && address != QHostAddress::AnyIPv6)
		{
			return address.toString();
		}
	}

	return {};
}

int PacUtils::dnsDomainLevels(const QString &host) const
{
	if (host.startsWith(QLatin1String("www."), Qt::CaseInsensitive))
	{
		return host.mid(4).count(QLatin1Char('.'));
	}

	return host.count(QLatin1Char('.'));
}

bool PacUtils::isInNet(const QString &host, const QString &pattern, const QString &mask) const
{
	const QHostAddress address(host);
	const QHostAddress netaddress(pattern);
	const QHostAddress netmask(mask);

	return (address.protocol() == QAbstractSocket::IPv4Protocol && (netaddress.toIPv4Address() & netmask.toIPv4Address()) == (address.toIPv4Address() & netmask.toIPv4Address()));
}

bool PacUtils::isPlainHostName(const QString &host) const
{
	return !host.contains(QLatin1Char('.'));
}

bool PacUtils::isResolvable(const QString &host) const
{
	return (QHostInfo::fromName(host).error() == QHostInfo::NoError);
}

bool PacUtils::localHostOrDomainIs(const QString &host, QString domain) const
{
// address "google.com" or "maps.google.com" or "www.google.com", domain ".google.com" - return true
	const QString address(QUrl(host).host().toLower());

	domain = domain.toLower();

	if (!address.contains(QLatin1Char('.')) || address.endsWith(domain))
	{
		return true;
	}

	return (domain.indexOf(QLatin1Char('.')) == 0 && domain.replace(0, 1, QString()) == address);
}

bool PacUtils::dnsDomainIs(const QString &host, const QString &domain) const
{
	return host.contains(domain);
}

bool PacUtils::shExpMatch(const QString &string, const QString &expression) const
{
	return QRegularExpression(QRegularExpression::wildcardToRegularExpression(expression)).match(string).hasMatch();
}

bool PacUtils::weekdayRange(QString fromDay, QString toDay, const QString &gmt) const
{
	fromDay = fromDay.toLower();
	toDay = toDay.toLower();

	const int currentDay(((gmt.toLower() == QLatin1String("gmt")) ? QDateTime::currentDateTimeUtc() : QDateTime::currentDateTime()).date().dayOfWeek());
	int fromDayNumber(-1);
	int toDayNumber(-1);

	for (int i = 0; i < m_days.count(); ++i)
	{
		const QString day(m_days.at(i));

		if (fromDay == day)
		{
			fromDayNumber = (i + 1);
		}

		if (toDay == day)
		{
			toDayNumber = (i + 1);
		}
	}

	if (toDayNumber == -1)
	{
		toDayNumber = fromDayNumber;
	}

	return ((fromDayNumber != -1 && toDayNumber != -1) && isNumberInRange(fromDayNumber, toDayNumber, currentDay));
}

bool PacUtils::dateRange(const QVariant &arg1, const QVariant &arg2, const QVariant &arg3, const QVariant &arg4, const QVariant &arg5, const QVariant &arg6, const QString &gmt) const
{
	const QVariantList rawArguments({arg1, arg2, arg3, arg4, arg5, arg6});
	const QDate currentDay(((gmt.toLower() == QLatin1String("gmt")) ? QDateTime::currentDateTimeUtc().date() : QDate::currentDate()));
	QVector<int> arguments;
	arguments.reserve(6);

	for (int i = 0; i < rawArguments.count(); ++i)
	{
		const QVariant rawArgument(rawArguments.at(i));

		if (rawArgument.isNull())
		{
			break;
		}

		if (rawArgument.type() == QVariant::String)
		{
			const int month(m_months.indexOf(rawArgument.toString().toLower()) + 1);

			if (month < 1)
			{
				return false;
			}

			arguments.append(month);
		}
		else
		{
			arguments.append(rawArgument.toInt());
		}
	}

	if (arguments.count() == 1 && arguments.at(0) > 1500)
	{
		return (currentDay.year() == arguments.at(0));
	}

	if (arguments.count() == 1 && rawArguments.at(0).type() == QVariant::Int)
	{
		return (currentDay.day() == arguments.at(0));
	}

	if (arguments.count() == 1)
	{
		return (currentDay.month() == arguments.at(0));
	}

	if (arguments.count() == 2 && arguments.at(0) > 1500 && arguments.at(1) > 1500)
	{
		return isNumberInRange(arguments.at(0), arguments.at(1), currentDay.year());
	}

	if (arguments.count() == 2 && rawArguments.at(0).type() == QVariant::Int && rawArguments.at(1).type() == QVariant::Int)
	{
		return isNumberInRange(arguments.at(0), arguments.at(1), currentDay.day());
	}

	if (arguments.count() == 2)
	{
		return isNumberInRange(arguments.at(0), arguments.at(1), currentDay.month());
	}

	if (arguments.count() == 3)
	{
		const QDate dateOne(arguments.at(2), arguments.at(1), arguments.at(0));

		return isDateInRange(dateOne, dateOne, currentDay);
	}

	if (arguments.count() == 4 && arguments.at(1) > 1500 && arguments.at(3) > 1500)
	{
		return isDateInRange({arguments.at(1), arguments.at(0), currentDay.day()}, {arguments.at(3), arguments.at(2), currentDay.day()}, currentDay);
	}

	if (arguments.count() == 4)
	{
		return isDateInRange({currentDay.year(), arguments.at(1), arguments.at(0)}, {currentDay.year(), arguments.at(3), arguments.at(2)}, currentDay);
	}

	if (arguments.count() == 6)
	{
		return isDateInRange({arguments.at(2), arguments.at(1), arguments.at(0)}, {arguments.at(5), arguments.at(4), arguments.at(3)}, currentDay);
	}

	return false;
}

bool PacUtils::timeRange(const QVariant &arg1, const QVariant &arg2, const QVariant &arg3, const QVariant &arg4, const QVariant &arg5, const QVariant &arg6, const QString &gmt) const
{
	const QVariantList rawArguments({arg1, arg2, arg3, arg4, arg5, arg6});
	const QTime currentTime(((gmt.toLower() == QLatin1String("gmt")) ? QDateTime::currentDateTimeUtc() : QDateTime::currentDateTime()).time());
	QVector<int> arguments;
	arguments.reserve(6);

	for (int i = 0; i < rawArguments.count(); ++i)
	{
		const QVariant rawArgument(rawArguments.at(i));

		if (rawArgument.isNull())
		{
			break;
		}

		if (rawArgument.type() != QVariant::Int)
		{
			return false;
		}

		arguments.append(rawArgument.toInt());
	}

	arguments.squeeze();

	if (arguments.count() == 1)
	{
		return isNumberInRange(arguments.at(0), arguments.at(0), currentTime.hour());
	}

	if (arguments.count() == 2)
	{
		return isNumberInRange(arguments.at(0), arguments.at(1), currentTime.hour());
	}

	if (arguments.count() == 4)
	{
		return isTimeInRange({arguments.at(0), arguments.at(1)}, {arguments.at(2), arguments.at(3)}, currentTime);
	}

	if (arguments.count() == 6)
	{
		return isTimeInRange({arguments.at(0), arguments.at(1), arguments.at(2)}, {arguments.at(3), arguments.at(4), arguments.at(5)}, currentTime);
	}

	return false;
}

bool PacUtils::isDateInRange(const QDate &from, const QDate &to, const QDate &value) const
{
	return (value >= from && value <= to);
}

bool PacUtils::isTimeInRange(const QTime &from, const QTime &to, const QTime &value) const
{
	return (value >= from && value <= to);
}

bool PacUtils::isNumberInRange(int from, int to, int value) const
{
	return (value >= from && value <= to);
}

NetworkAutomaticProxy::NetworkAutomaticProxy(const QString &path, QObject *parent) : QObject(parent),
	m_path(path),
	m_isValid(false)
{
	m_engine.globalObject().setProperty(QLatin1String("PacUtils"), m_engine.newQObject(new PacUtils(this)));

	const QStringList functions({QLatin1String("alert"), QLatin1String("dnsResolve"), QLatin1String("myIpAddress"), QLatin1String("dnsDomainLevels"), QLatin1String("isInNet"), QLatin1String("isPlainHostName"), QLatin1String("isResolvable"), QLatin1String("localHostOrDomainIs"), QLatin1String("dnsDomainIs"), QLatin1String("shExpMatch"), QLatin1String("weekdayRange"), QLatin1String("dateRange"), QLatin1String("timeRange")});

	for (int i = 0; i < functions.count(); ++i)
	{
		m_engine.evaluate(QStringLiteral("function %1() { return PacUtils.%1.apply(null, arguments); }").arg(functions.at(i))).isError();
	}

	m_proxies.insert(QLatin1String("ERROR"), QVector<QNetworkProxy>({QNetworkProxy(QNetworkProxy::DefaultProxy)}));
	m_proxies.insert(QLatin1String("DIRECT"), QVector<QNetworkProxy>({QNetworkProxy(QNetworkProxy::NoProxy)}));

	setPath(path);
}

void NetworkAutomaticProxy::setPath(const QString &path)
{
	if (QFile::exists(path))
	{
		QFile file(path);

		if (file.open(QIODevice::ReadOnly | QIODevice::Text) && setup(QString::fromLatin1(file.readAll())))
		{
			m_isValid = true;

			file.close();
		}
		else
		{
			Console::addMessage(tr("Failed to load proxy auto-config (PAC): %1").arg(file.errorString()), Console::NetworkCategory, Console::ErrorLevel, path);
		}

		return;
	}

	const QUrl url(path);

	if (url.isValid())
	{
		DataFetchJob *job(new DataFetchJob(url, this));

		connect(job, &Job::jobFinished, this, [=](bool isSuccess)
		{
			QIODevice *device(job->getData());

			if (isSuccess && device && setup(QString::fromLatin1(device->readAll())))
			{
				m_isValid = true;
			}
			else
			{
				Console::addMessage(tr("Failed to load proxy auto-config (PAC): %1").arg(device ? device->errorString() : tr("Download failure")), Console::NetworkCategory, Console::ErrorLevel, url.url());
			}
		});

		job->start();
	}
	else
	{
		Console::addMessage(tr("Failed to load proxy auto-config (PAC). Invalid URL: %1").arg(url.url()), Console::NetworkCategory, Console::ErrorLevel);
	}
}

QString NetworkAutomaticProxy::getPath() const
{
	return m_path;
}

QVector<QNetworkProxy> NetworkAutomaticProxy::getProxy(const QString &url, const QString &host)
{
	const QJSValue result(m_findProxyFunction.call(QJSValueList({m_engine.toScriptValue(url), m_engine.toScriptValue(host)})));

	if (result.isError())
	{
		return m_proxies[QLatin1String("ERROR")];
	}

	const QString configuration(result.toString().remove(QLatin1Char(' ')));

	if (!m_proxies.value(configuration).isEmpty())
	{
		return m_proxies[configuration];
	}

// proxy format: "PROXY host:port; PROXY host:port", "PROXY host:port; SOCKS host:port" etc.
// can be combination of DIRECT, PROXY, SOCKS
	const QStringList proxies(configuration.split(QLatin1Char(';')));
	QVector<QNetworkProxy> proxiesForQuery;

	for (int i = 0; i < proxies.count(); ++i)
	{
		const QStringList proxy(proxies.at(i).split(QLatin1Char(':')));
		QString proxyHost(proxy.at(0));
		const int proxyCount(proxy.count());

		if (proxyCount == 2)
		{
			const ushort proxyPort(proxy.at(1).toUShort());

			if (proxyHost.indexOf(QLatin1String("PROXY"), Qt::CaseInsensitive) == 0)
			{
				proxiesForQuery.append(QNetworkProxy(QNetworkProxy::HttpProxy, proxyHost.remove(0, 5), proxyPort));

				continue;
			}

			if (proxyHost.indexOf(QLatin1String("SOCKS"), Qt::CaseInsensitive) == 0)
			{
				proxiesForQuery.append(QNetworkProxy(QNetworkProxy::Socks5Proxy, proxyHost.remove(0, 5), proxyPort));

				continue;
			}
		}

		if (proxyCount == 1 && proxyHost.indexOf(QLatin1String("DIRECT"), Qt::CaseInsensitive) == 0)
		{
			proxiesForQuery.append(QNetworkProxy(QNetworkProxy::NoProxy));

			continue;
		}

		Console::addMessage(QCoreApplication::translate("main", "Failed to parse entry of proxy auto-config (PAC): %1").arg(proxies.at(i)), Console::NetworkCategory, Console::ErrorLevel);

		return m_proxies[QLatin1String("ERROR")];
	}

	m_proxies.insert(configuration, proxiesForQuery);

	return m_proxies[configuration];
}

bool NetworkAutomaticProxy::isValid() const
{
	return m_isValid;
}

bool NetworkAutomaticProxy::setup(const QString &script)
{
	if (m_engine.evaluate(script).isError())
	{
		return false;
	}

	m_findProxyFunction = m_engine.globalObject().property(QLatin1String("FindProxyForURL"));

	return m_findProxyFunction.isCallable();
}

}
