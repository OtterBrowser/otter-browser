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

#ifndef OTTER_COOKIEJARPROXY_H
#define OTTER_COOKIEJARPROXY_H

#include "CookieJar.h"

namespace Otter
{

class WebWidget;

class CookieJarProxy : public QNetworkCookieJar
{
	Q_OBJECT

public:
	explicit CookieJarProxy(CookieJar *cookieJar, WebWidget *widget);

	void setup(CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode);
	void setWidget(WebWidget *widget);
	CookieJarProxy* clone(WebWidget *parent = NULL);
	CookieJar* getCookieJar();
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
	bool insertCookie(const QNetworkCookie &cookie);
	bool deleteCookie(const QNetworkCookie &cookie);
	bool updateCookie(const QNetworkCookie &cookie);

private:
	WebWidget *m_widget;
	CookieJar *m_cookieJar;
	CookieJar::CookiesPolicy m_generalCookiesPolicy;
	CookieJar::CookiesPolicy m_thirdPartyCookiesPolicy;
	CookieJar::KeepMode m_keepMode;
};

}

#endif
