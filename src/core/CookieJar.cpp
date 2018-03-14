/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookieJar.h"
#include "Application.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QTimerEvent>

namespace Otter
{

CookieJar::CookieJar(bool isPrivate, QObject *parent) : QNetworkCookieJar(parent),
	m_generalCookiesPolicy(AcceptAllCookies),
	m_thirdPartyCookiesPolicy(AcceptAllCookies),
	m_keepMode(KeepUntilExpiresMode),
	m_saveTimer(0),
	m_isPrivate(isPrivate)
{
	if (isPrivate)
	{
		return;
	}

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("cookies.dat")));

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QDataStream stream(&file);
	quint32 amount;

	stream >> amount;

	QList<QNetworkCookie> allCookies;
	allCookies.reserve(static_cast<int>(amount));

	for (quint32 i = 0; i < amount; ++i)
	{
		QByteArray value;

		stream >> value;

		const QList<QNetworkCookie> cookies(QNetworkCookie::parseCookies(value));

		for (int j = 0; j < cookies.count(); ++j)
		{
			allCookies.append(cookies.at(j));
		}

		if (stream.atEnd())
		{
			break;
		}
	}

	handleOptionChanged(SettingsManager::Network_CookiesPolicyOption, SettingsManager::getOption(SettingsManager::Network_CookiesPolicyOption));
	setAllCookies(allCookies);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &CookieJar::handleOptionChanged);
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

void CookieJar::clearCookies(int period)
{
	Q_UNUSED(period)

	const QList<QNetworkCookie> cookies(allCookies());

	setAllCookies({});

	for (int i = 0; i < cookies.count(); ++i)
	{
		emit cookieRemoved(cookies.at(i));
	}

	scheduleSave();
}

void CookieJar::scheduleSave()
{
	if (!m_isPrivate)
	{
		if (Application::isAboutToQuit())
		{
			save();
		}
		else if (m_saveTimer == 0)
		{
			m_saveTimer = startTimer(500);
		}
	}
}

void CookieJar::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Browser_PrivateModeOption:
			if (value.toBool())
			{
				m_generalCookiesPolicy = IgnoreCookies;
			}

			break;
		case SettingsManager::Network_CookiesPolicyOption:
			if (SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool() || value.toString() == QLatin1String("ignore"))
			{
				m_generalCookiesPolicy = IgnoreCookies;
			}
			else if (value.toString() == QLatin1String("readOnly"))
			{
				m_generalCookiesPolicy = ReadOnlyCookies;
			}
			else if (value.toString() == QLatin1String("acceptExisting"))
			{
				m_generalCookiesPolicy = AcceptExistingCookies;
			}
			else
			{
				m_generalCookiesPolicy = AcceptAllCookies;
			}

			break;
		default:
			break;
	}
}

void CookieJar::save()
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	QSaveFile file(SessionsManager::getWritableDataPath(QLatin1String("cookies.dat")));

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	const QList<QNetworkCookie> cookies(allCookies());
	QDataStream stream(&file);
	stream << static_cast<quint32>(cookies.count());

	for (int i = 0; i < cookies.count(); ++i)
	{
		if (!cookies.at(i).isSessionCookie())
		{
			stream << cookies.at(i).toRawForm();
		}
	}

	file.commit();
}

CookieJar* CookieJar::clone(QObject *parent) const
{
	CookieJar *cookieJar(new CookieJar(m_isPrivate, parent));
	cookieJar->setAllCookies(allCookies());

	return cookieJar;
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl &url) const
{
	if (m_generalCookiesPolicy == IgnoreCookies)
	{
		return {};
	}

	return QNetworkCookieJar::cookiesForUrl(url);
}

QList<QNetworkCookie> CookieJar::getCookiesForUrl(const QUrl &url) const
{
	return QNetworkCookieJar::cookiesForUrl(url);
}

QVector<QNetworkCookie> CookieJar::getCookies(const QString &domain) const
{
	if (!domain.isEmpty())
	{
		const QList<QNetworkCookie> cookies(allCookies());
		QVector<QNetworkCookie> domainCookies;

		for (int i = 0; i < cookies.count(); ++i)
		{
			if (cookies.at(i).domain() == domain || (cookies.at(i).domain().startsWith(QLatin1Char('.')) && domain.endsWith(cookies.at(i).domain())))
			{
				domainCookies.append(cookies.at(i));
			}
		}

		return domainCookies;
	}

	return allCookies().toVector();
}

bool CookieJar::insertCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy != AcceptAllCookies)
	{
		return false;
	}

	const bool result(QNetworkCookieJar::insertCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieAdded(cookie);
	}

	return result;
}

bool CookieJar::updateCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy == IgnoreCookies || m_generalCookiesPolicy == ReadOnlyCookies)
	{
		return false;
	}

	const bool result(QNetworkCookieJar::updateCookie(cookie));

	if (result)
	{
		scheduleSave();
	}

	return result;
}

bool CookieJar::deleteCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy == IgnoreCookies || m_generalCookiesPolicy == ReadOnlyCookies)
	{
		return false;
	}

	const bool result(QNetworkCookieJar::deleteCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieRemoved(cookie);
	}

	return result;
}

bool CookieJar::forceInsertCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::insertCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieAdded(cookie);
	}

	return result;
}

bool CookieJar::forceUpdateCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::updateCookie(cookie));

	if (result)
	{
		scheduleSave();
	}

	return result;
}

bool CookieJar::forceDeleteCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::deleteCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieRemoved(cookie);
	}

	return result;
}

bool CookieJar::hasCookie(const QNetworkCookie &cookie) const
{
	QUrl url;
	url.setScheme(cookie.isSecure() ? QLatin1String("https") : QLatin1String("http"));
	url.setHost(cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain());
	url.setPath(cookie.path());

	const QList<QNetworkCookie> cookies(getCookiesForUrl(url));

	for (int i = 0; i < cookies.count(); ++i)
	{
		if (cookies.at(i).domain() == cookie.domain() && cookies.at(i).name() == cookie.name())
		{
			return true;
		}
	}

	return false;
}

bool CookieJar::isDomainTheSame(const QUrl &first, const QUrl &second)
{
	const QString firstTld(first.topLevelDomain());
	const QString secondTld(second.topLevelDomain());

	if (firstTld != secondTld)
	{
		return false;
	}

	QString firstDomain(QLatin1Char('.') + first.host().toLower());
	firstDomain.remove((firstDomain.length() - firstTld.length()), firstTld.length());

	QString secondDomain(QLatin1Char('.') + second.host().toLower());
	secondDomain.remove((secondDomain.length() - secondTld.length()), secondTld.length());

	if (firstDomain.section(QLatin1Char('.'), -1) == secondDomain.section(QLatin1Char('.'), -1))
	{
		return true;
	}

	return false;
}

}
