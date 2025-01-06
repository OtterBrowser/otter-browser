/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitCookieJar.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../ui/AcceptCookieDialog.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/WebWidget.h"

namespace Otter
{

QtWebKitCookieJar::QtWebKitCookieJar(CookieJar *cookieJar, WebWidget *widget) : QNetworkCookieJar(widget),
	m_widget(widget),
	m_cookieJar(cookieJar),
	m_generalCookiesPolicy(CookieJar::AcceptAllCookies),
	m_thirdPartyCookiesPolicy(CookieJar::AcceptAllCookies),
	m_keepMode(CookieJar::KeepUntilExpiresMode)
{
}

void QtWebKitCookieJar::setup(const QStringList &thirdPartyAcceptedHosts, const QStringList &thirdPartyRejectedHosts, CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode)
{
	m_thirdPartyAcceptedHosts = thirdPartyAcceptedHosts;
	m_thirdPartyRejectedHosts = thirdPartyRejectedHosts;
	m_generalCookiesPolicy = generalCookiesPolicy;
	m_thirdPartyCookiesPolicy = thirdPartyCookiesPolicy;
	m_keepMode = keepMode;
}

void QtWebKitCookieJar::showDialog(const QNetworkCookie &cookie, CookieJar::CookieOperation operation)
{
	AcceptCookieDialog *cookieDialog(new AcceptCookieDialog(cookie, operation, m_cookieJar, m_widget));
	ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("dialog-warning")), cookieDialog->windowTitle(), {}, {}, QDialogButtonBox::NoButton, cookieDialog, m_widget));
	dialog->setAttribute(Qt::WA_DeleteOnClose);

	connect(cookieDialog, &AcceptCookieDialog::finished, dialog, &ContentsDialog::close);
	connect(m_widget, &WebWidget::aboutToReload, dialog, &ContentsDialog::close);

	m_widget->showDialog(dialog, false);
}

void QtWebKitCookieJar::setWidget(WebWidget *widget)
{
	m_widget = widget;
}

QtWebKitCookieJar* QtWebKitCookieJar::clone(WebWidget *parent)
{
	return new QtWebKitCookieJar(m_cookieJar, parent);
}

CookieJar* QtWebKitCookieJar::getCookieJar()
{
	return m_cookieJar;
}

QList<QNetworkCookie> QtWebKitCookieJar::cookiesForUrl(const QUrl &url) const
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies)
	{
		return {};
	}

	return m_cookieJar->getCookiesForUrl(url);
}

bool QtWebKitCookieJar::insertCookie(const QNetworkCookie &cookie)
{
	if (!canModifyCookie(cookie))
	{
		return false;
	}

	if (m_keepMode == CookieJar::AskIfKeepMode)
	{
		showDialog(cookie, (m_cookieJar->hasCookie(cookie) ? CookieJar::UpdateCookie : CookieJar::InsertCookie));

		return false;
	}

	if (m_keepMode == CookieJar::KeepUntilExitMode)
	{
		QNetworkCookie mutableCookie(cookie);
		mutableCookie.setExpirationDate({});

		return m_cookieJar->forceInsertCookie(mutableCookie);
	}

	return m_cookieJar->forceInsertCookie(cookie);
}

bool QtWebKitCookieJar::updateCookie(const QNetworkCookie &cookie)
{
	return insertCookie(cookie);
}

bool QtWebKitCookieJar::deleteCookie(const QNetworkCookie &cookie)
{
	if (!canModifyCookie(cookie))
	{
		return false;
	}

	if (m_keepMode == CookieJar::AskIfKeepMode)
	{
		showDialog(cookie, CookieJar::RemoveCookie);

		return false;
	}

	return m_cookieJar->forceDeleteCookie(cookie);
}

bool QtWebKitCookieJar::canModifyCookie(const QNetworkCookie &cookie) const
{
	if (!m_widget || m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	if (m_thirdPartyCookiesPolicy != CookieJar::AcceptAllCookies || !m_thirdPartyRejectedHosts.isEmpty())
	{
		QUrl url;
		url.setScheme(QLatin1String("http"));
		url.setHost(cookie.domain().startsWith(QLatin1Char('.')) ? cookie.domain().mid(1) : cookie.domain());

		if (!Utils::isDomainTheSame(m_widget->getUrl(), url))
		{
			if (m_thirdPartyRejectedHosts.contains(cookie.domain()))
			{
				return false;
			}

			if (m_thirdPartyAcceptedHosts.contains(cookie.domain()))
			{
				return true;
			}

			if (m_thirdPartyCookiesPolicy == CookieJar::IgnoreCookies)
			{
				return false;
			}

			if (m_thirdPartyCookiesPolicy == CookieJar::AcceptExistingCookies && !m_cookieJar->hasCookie(cookie))
			{
				return false;
			}
		}
	}

	return true;
}

bool QtWebKitCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url)
{
	if (m_generalCookiesPolicy == CookieJar::IgnoreCookies || m_generalCookiesPolicy == CookieJar::ReadOnlyCookies)
	{
		return false;
	}

	bool isSuccess(false);

	for (int i = 0; i < cookies.count(); ++i)
	{
		QNetworkCookie cookie(cookies.at(i));
		cookie.normalize(url);

		if (validateCookie(cookie, url) && canModifyCookie(cookie))
		{
			if (m_keepMode == CookieJar::AskIfKeepMode)
			{
				showDialog(cookie, (m_cookieJar->hasCookie(cookie) ? CookieJar::UpdateCookie : CookieJar::InsertCookie));
			}
			else
			{
				if (m_keepMode == CookieJar::KeepUntilExitMode)
				{
					cookie.setExpirationDate({});
				}

				m_cookieJar->forceInsertCookie(cookie);

				isSuccess = true;
			}
		}
	}

	return isSuccess;
}

}
