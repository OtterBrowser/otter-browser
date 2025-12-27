/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

CookieJar::CookieJar(QObject *parent) : QNetworkCookieJar(parent)
{
}

QList<QNetworkCookie> CookieJar::getCookiesForUrl(const QUrl &url) const
{
	return QNetworkCookieJar::cookiesForUrl(url);
}

QVector<QNetworkCookie> CookieJar::getCookies(const QString &domain) const
{
	if (domain.isEmpty())
	{
		return allCookies().toVector();
	}

	const QList<QNetworkCookie> cookies(allCookies());
	QVector<QNetworkCookie> domainCookies;

	for (const QNetworkCookie &cookie: cookies)
	{
		if (cookie.domain() == domain || (cookie.domain().startsWith(QLatin1Char('.')) && domain.endsWith(cookie.domain())))
		{
			domainCookies.append(cookie);
		}
	}

	return domainCookies;
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
		if (cookie.hasSameIdentifier(cookies.at(i)))
		{
			return true;
		}
	}

	return false;
}

DiskCookieJar::DiskCookieJar(const QString &path, QObject *parent) : CookieJar(parent),
	m_path(path),
	m_generalCookiesPolicy(AcceptAllCookies),
	m_thirdPartyCookiesPolicy(AcceptAllCookies),
	m_keepMode(KeepUntilExpiresMode),
	m_saveTimer(0)
{
	if (!path.isEmpty())
	{
		loadCookies(path);
	}

	handleOptionChanged(SettingsManager::Network_CookiesPolicyOption, SettingsManager::getOption(SettingsManager::Network_CookiesPolicyOption));

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &DiskCookieJar::handleOptionChanged);
}

void DiskCookieJar::timerEvent(QTimerEvent *event)
{
	if (event->timerId() != m_saveTimer)
	{
		return;
	}

	killTimer(m_saveTimer);

	m_saveTimer = 0;

	save();
}

void DiskCookieJar::loadCookies(const QString &path)
{
	QFile file(path);

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

		for (const QNetworkCookie &cookie: cookies)
		{
			allCookies.append(cookie);
		}

		if (stream.atEnd())
		{
			break;
		}
	}

	setAllCookies(allCookies);
}

void DiskCookieJar::clearCookies(int period)
{
	Q_UNUSED(period)

	const QList<QNetworkCookie> cookies(allCookies());

	setAllCookies({});

	for (const QNetworkCookie &cookie: cookies)
	{
		emit cookieRemoved(cookie);
	}

	scheduleSave();
}

void DiskCookieJar::scheduleSave()
{
	if (m_path.isEmpty())
	{
		return;
	}

	if (Application::isAboutToQuit())
	{
		if (m_saveTimer != 0)
		{
			killTimer(m_saveTimer);

			m_saveTimer = 0;
		}

		save();
	}
	else if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(500);
	}
}

void DiskCookieJar::handleOptionChanged(int identifier, const QVariant &value)
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

void DiskCookieJar::save()
{
	if (m_path.isEmpty() || SessionsManager::isReadOnly())
	{
		return;
	}

	QSaveFile file(m_path);

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	const QList<QNetworkCookie> cookies(allCookies());
	QDataStream stream(&file);
	stream << static_cast<quint32>(cookies.count());

	for (const QNetworkCookie &cookie: cookies)
	{
		if (!cookie.isSessionCookie())
		{
			stream << cookie.toRawForm();
		}
	}

	file.commit();
}

QString DiskCookieJar::getPath() const
{
	return m_path;
}

QList<QNetworkCookie> DiskCookieJar::cookiesForUrl(const QUrl &url) const
{
	if (m_generalCookiesPolicy == IgnoreCookies)
	{
		return {};
	}

	return QNetworkCookieJar::cookiesForUrl(url);
}

bool DiskCookieJar::insertCookie(const QNetworkCookie &cookie)
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

bool DiskCookieJar::updateCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy == IgnoreCookies || m_generalCookiesPolicy == ReadOnlyCookies)
	{
		return false;
	}

	const bool result(QNetworkCookieJar::updateCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieModified(cookie);
	}

	return result;
}

bool DiskCookieJar::deleteCookie(const QNetworkCookie &cookie)
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

bool DiskCookieJar::forceInsertCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::insertCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieAdded(cookie);
	}

	return result;
}

bool DiskCookieJar::forceUpdateCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::updateCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieModified(cookie);
	}

	return result;
}

bool DiskCookieJar::forceDeleteCookie(const QNetworkCookie &cookie)
{
	const bool result(QNetworkCookieJar::deleteCookie(cookie));

	if (result)
	{
		scheduleSave();

		emit cookieRemoved(cookie);
	}

	return result;
}

}
