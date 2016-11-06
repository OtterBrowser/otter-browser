/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITCOOKIEJAR_H
#define OTTER_QTWEBKITCOOKIEJAR_H

#include "../../../../core/CookieJar.h"

#include <QtCore/QQueue>
#include <QtCore/QStringList>

namespace Otter
{

class WebWidget;

class QtWebKitCookieJar : public QNetworkCookieJar
{
	Q_OBJECT

public:
	explicit QtWebKitCookieJar(CookieJar *cookieJar, WebWidget *widget);

	void setup(const QStringList &thirdPartyAcceptedHosts, const QStringList &thirdPartyRejectedHosts, CookieJar::CookiesPolicy generalCookiesPolicy, CookieJar::CookiesPolicy thirdPartyCookiesPolicy, CookieJar::KeepMode keepMode);
	void setWidget(WebWidget *widget);
	QtWebKitCookieJar* clone(WebWidget *parent = nullptr);
	CookieJar* getCookieJar();
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
	bool insertCookie(const QNetworkCookie &cookie);
	bool updateCookie(const QNetworkCookie &cookie);
	bool deleteCookie(const QNetworkCookie &cookie);
	bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);

protected:
	bool canModifyCookie(const QNetworkCookie &cookie) const;

protected slots:
	void dialogClosed();
	void showDialog();

private:
	WebWidget *m_widget;
	CookieJar *m_cookieJar;
	QStringList m_thirdPartyAcceptedHosts;
	QStringList m_thirdPartyRejectedHosts;
	QQueue<QPair<CookieJar::CookieOperation, QNetworkCookie> > m_operations;
	CookieJar::CookiesPolicy m_generalCookiesPolicy;
	CookieJar::CookiesPolicy m_thirdPartyCookiesPolicy;
	CookieJar::KeepMode m_keepMode;
	bool m_isDialogVisible;
};

}

#endif
