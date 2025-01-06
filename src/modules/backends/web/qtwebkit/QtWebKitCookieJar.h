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

#ifndef OTTER_QTWEBKITCOOKIEJAR_H
#define OTTER_QTWEBKITCOOKIEJAR_H

#include "../../../../core/CookieJar.h"

#include <QtCore/QStringList>

namespace Otter
{

class WebWidget;

class QtWebKitCookieJar final : public QNetworkCookieJar
{
	Q_OBJECT

public:
	explicit QtWebKitCookieJar(CookieJar *cookieJar, WebWidget *widget);

	void setup(const QStringList &thirdPartyAcceptedHosts, const QStringList &thirdPartyRejectedHosts, CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode);
	void setWidget(WebWidget *widget);
	QtWebKitCookieJar* clone(WebWidget *parent = nullptr);
	CookieJar* getCookieJar();
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
	bool insertCookie(const QNetworkCookie &cookie) override;
	bool updateCookie(const QNetworkCookie &cookie) override;
	bool deleteCookie(const QNetworkCookie &cookie) override;
	bool setCookiesFromUrl(const QList<QNetworkCookie> &cookies, const QUrl &url) override;

protected:
	void showDialog(const QNetworkCookie &cookie, CookieJar::CookieOperation operation);
	bool canModifyCookie(const QNetworkCookie &cookie) const;

private:
	WebWidget *m_widget;
	CookieJar *m_cookieJar;
	QStringList m_thirdPartyAcceptedHosts;
	QStringList m_thirdPartyRejectedHosts;
	CookieJar::CookiesPolicy m_generalCookiesPolicy;
	CookieJar::CookiesPolicy m_thirdPartyCookiesPolicy;
	CookieJar::KeepMode m_keepMode;
};

}

#endif
