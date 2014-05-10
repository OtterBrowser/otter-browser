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

#ifndef OTTER_COOKIEJAR_H
#define OTTER_COOKIEJAR_H

#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Otter
{

class CookieJar : public QNetworkCookieJar
{
	Q_OBJECT
	Q_ENUMS(KeepCookiesPolicy ThirdPartyCookiesAcceptPolicy)

public:
	explicit CookieJar(QObject *parent = NULL);

	enum KeepCookiesPolicy
	{
		UntilExpireKeepCookies = 0,
		UntilExitKeepCookies = 1,
		AskIfKeepCookies = 2
	};

	enum ThirdPartyCookiesAcceptPolicy
	{
		NeverAcceptCookies = 0,
		AlwaysAcceptCookies = 1,
		AcceptExistingCookies = 2
	};

	void clearCookies(int period = 0);
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
	QList<QNetworkCookie> getCookies() const;

protected:
	void timerEvent(QTimerEvent *event);
	void scheduleSave();
	bool insertCookie(const QNetworkCookie &cookie);
	bool deleteCookie(const QNetworkCookie &cookie);
	bool updateCookie(const QNetworkCookie &cookie);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);

private:
	int m_saveTimer;
	bool m_enableCookies;

signals:
	void cookieAdded(QNetworkCookie cookie);
	void cookieRemoved(QNetworkCookie cookie);
};

}

#endif
