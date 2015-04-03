/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "CookieJarProxy.h"
#include "Utils.h"
#include "../ui/AcceptCookieDialog.h"
#include "../ui/ContentsDialog.h"
#include "../ui/WebWidget.h"

namespace Otter
{

CookieJarProxy::CookieJarProxy(CookieJar *cookieJar, WebWidget *widget) : QNetworkCookieJar(widget),
	m_widget(widget),
	m_cookieJar(cookieJar),
	m_generalCookiesPolicy(CookieJar::AcceptAllCookies),
	m_thirdPartyCookiesPolicy(CookieJar::AcceptAllCookies),
	m_keepMode(CookieJar::KeepUntilExpiresMode)
{
}

void CookieJarProxy::setup(CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode)
{
	m_generalCookiesPolicy = generalCookiesPolicy;
	m_thirdPartyCookiesPolicy = thirdPartyCookiesPolicy;
	m_keepMode = keepMode;
}

void CookieJarProxy::setWidget(WebWidget *widget)
{
	m_widget = widget;
}

CookieJarProxy* CookieJarProxy::clone(WebWidget *parent)
{
	return new CookieJarProxy(m_cookieJar, parent);
}

CookieJar* CookieJarProxy::getCookieJar()
{
	return m_cookieJar;
}

QList<QNetworkCookie> CookieJarProxy::cookiesForUrl(const QUrl &url) const
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies)
	{
		return QList<QNetworkCookie>();
	}

	return m_cookieJar->getCookiesForUrl(url);
}

bool CookieJarProxy::insertCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy != CookieJar::AcceptAllCookies)
	{
		return false;
	}

	if (m_keepMode == CookieJar::AskIfKeepMode)
	{
		AcceptCookieDialog *cookieDialog = new AcceptCookieDialog(cookie, AcceptCookieDialog::InsertCookie, m_widget);
		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), cookieDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, cookieDialog, m_widget);

		connect(cookieDialog, SIGNAL(finished(int)), cookieDialog, SLOT(close()));
		connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

		m_widget->showDialog(&dialog);

		if (cookieDialog->getResult() == AcceptCookieDialog::AcceptAsSessionCookie)
		{
			QNetworkCookie mutableCookie(cookie);
			mutableCookie.setExpirationDate(QDateTime());

			return m_cookieJar->forceInsertCookie(mutableCookie);
		}

		if (cookieDialog->getResult() == AcceptCookieDialog::IgnoreCookie)
		{
			return false;
		}
	}
	else if (m_keepMode == CookieJar::KeepUntilExitMode)
	{
		QNetworkCookie mutableCookie(cookie);
		mutableCookie.setExpirationDate(QDateTime());

		return m_cookieJar->forceInsertCookie(mutableCookie);
	}

	return m_cookieJar->forceInsertCookie(cookie);
}

bool CookieJarProxy::deleteCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	AcceptCookieDialog *cookieDialog = new AcceptCookieDialog(cookie, AcceptCookieDialog::InsertCookie, m_widget);
	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), cookieDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, cookieDialog, m_widget);

	connect(cookieDialog, SIGNAL(finished(int)), cookieDialog, SLOT(close()));
	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (cookieDialog->getResult() == AcceptCookieDialog::IgnoreCookie)
	{
		return false;
	}

	return m_cookieJar->forceDeleteCookie(cookie);
}

bool CookieJarProxy::updateCookie(const QNetworkCookie &cookie)
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	AcceptCookieDialog *cookieDialog = new AcceptCookieDialog(cookie, AcceptCookieDialog::UpdateCookie, m_widget);
	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), cookieDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, cookieDialog, m_widget);

	connect(cookieDialog, SIGNAL(finished(int)), cookieDialog, SLOT(close()));
	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (cookieDialog->getResult() == AcceptCookieDialog::AcceptAsSessionCookie)
	{
		QNetworkCookie mutableCookie(cookie);
		mutableCookie.setExpirationDate(QDateTime());

		return m_cookieJar->forceUpdateCookie(mutableCookie);
	}

	if (cookieDialog->getResult() == AcceptCookieDialog::IgnoreCookie)
	{
		return false;
	}

	return m_cookieJar->forceUpdateCookie(cookie);
}

}
