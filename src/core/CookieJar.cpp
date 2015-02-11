/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookieJar.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QTimerEvent>

namespace Otter
{

CookieJar::CookieJar(bool isPrivate, QObject *parent) : QNetworkCookieJar(parent),
	m_keepCookiesPolicy(UntilExpireKeepCookies),
	m_thirdPartyCookiesAcceptPolicy(AlwaysAcceptCookies),
	m_saveTimer(0),
	m_enableCookies(true),
	m_isPrivate(isPrivate)
{
	if (isPrivate)
	{
		return;
	}

	QFile file(SessionsManager::getProfilePath() + QLatin1String("/cookies.dat"));

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QList<QNetworkCookie> allCookies;
	QDataStream stream(&file);
	quint32 amount;

	stream >> amount;

	for (quint32 i = 0; i < amount; ++i)
	{
		QByteArray value;

		stream >> value;

		const QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(value);

		for (int j = 0; j < cookies.count(); ++j)
		{
			allCookies.append(cookies.at(j));
		}

		if (stream.atEnd())
		{
			break;
		}
	}

	optionChanged(QLatin1String("Browser/EnableCookies"), SettingsManager::getValue(QLatin1String("Browser/EnableCookies")));
	setAllCookies(allCookies);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void CookieJar::timerEvent(QTimerEvent *event)
{
	if (event->timerId() != m_saveTimer)
	{
		return;
	}

	killTimer(m_saveTimer);

	m_saveTimer = 0;

	save();
}

void CookieJar::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Browser/PrivateMode"))
	{
		m_enableCookies = (!value.toBool() && SettingsManager::getValue(QLatin1String("Browser/EnableCookies")).toBool());
	}
	else if (option == QLatin1String("Browser/EnableCookies"))
	{
		m_enableCookies = (value.toBool() && !SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());
	}
}

void CookieJar::clearCookies(int period)
{
	Q_UNUSED(period)

	setAllCookies(QList<QNetworkCookie>());
	save();
}

void CookieJar::scheduleSave()
{
	if (!m_isPrivate && m_saveTimer == 0)
	{
		m_saveTimer = startTimer(500);
	}
}

void CookieJar::save()
{
	QSaveFile file(SessionsManager::getProfilePath() + QLatin1String("/cookies.dat"));

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	QList<QNetworkCookie> cookies = allCookies();
	QDataStream stream(&file);
	stream << quint32(cookies.size());

	for (int i = 0; i < cookies.size(); ++i)
	{
		stream << cookies.at(i).toRawForm();
	}

	file.commit();
}

CookieJar* CookieJar::clone(QObject *parent)
{
	CookieJar *cookieJar = new CookieJar(m_isPrivate, parent);
	cookieJar->setAllCookies(allCookies());

	return cookieJar;
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl &url) const
{
	if (!m_enableCookies)
	{
		return QList<QNetworkCookie>();
	}

	return QNetworkCookieJar::cookiesForUrl(url);
}

QList<QNetworkCookie> CookieJar::getCookies(const QString &domain) const
{
	if (!domain.isEmpty())
	{
		const QList<QNetworkCookie> cookies = allCookies();
		QList<QNetworkCookie> domainCookies;

		for (int i = 0; i < cookies.length(); ++i)
		{
			if (cookies.at(i).domain() == domain || (cookies.at(i).domain().startsWith(QLatin1Char('.')) && domain.endsWith(cookies.at(i).domain())))
			{
				domainCookies.append(cookies.at(i));
			}
		}

		return domainCookies;
	}

	return allCookies();
}

CookieJar::KeepCookiesPolicy CookieJar::getKeepCookiesPolicy() const
{
	return m_keepCookiesPolicy;
}

CookieJar::ThirdPartyCookiesAcceptPolicy CookieJar::getThirdPartyCookiesAcceptPolicy() const
{
	return m_thirdPartyCookiesAcceptPolicy;
}

bool CookieJar::insertCookie(const QNetworkCookie &cookie)
{
	if (!m_enableCookies)
	{
		return false;
	}

	const bool result = QNetworkCookieJar::insertCookie(cookie);

	if (result)
	{
		scheduleSave();

		emit cookieAdded(cookie);
	}

	return result;
}

bool CookieJar::deleteCookie(const QNetworkCookie &cookie)
{
	const bool result = QNetworkCookieJar::deleteCookie(cookie);

	if (result)
	{
		scheduleSave();

		emit cookieRemoved(cookie);
	}

	return result;
}

bool CookieJar::updateCookie(const QNetworkCookie &cookie)
{
	if (!m_enableCookies)
	{
		return false;
	}

	const bool result = QNetworkCookieJar::updateCookie(cookie);

	if (result)
	{
		scheduleSave();
	}

	return result;
}

}
