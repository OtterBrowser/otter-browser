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

#ifndef OTTER_COOKIEJAR_H
#define OTTER_COOKIEJAR_H

#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Otter
{

class CookieJar final : public QNetworkCookieJar
{
	Q_OBJECT

public:
	enum CookiesPolicy
	{
		IgnoreCookies = 0,
		ReadOnlyCookies,
		AcceptAllCookies,
		AcceptExistingCookies
	};

	enum KeepMode
	{
		KeepUntilExpiresMode = 0,
		KeepUntilExitMode,
		AskIfKeepMode
	};

	enum CookieOperation
	{
		InsertCookie = 0,
		UpdateCookie,
		RemoveCookie
	};

	explicit CookieJar(bool isPrivate, QObject *parent = nullptr);

	void clearCookies(int period = 0);
	CookieJar* clone(QObject *parent = nullptr) const;
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
	QList<QNetworkCookie> getCookiesForUrl(const QUrl &url) const;
	QVector<QNetworkCookie> getCookies(const QString &domain = {}) const;
	bool insertCookie(const QNetworkCookie &cookie) override;
	bool updateCookie(const QNetworkCookie &cookie) override;
	bool deleteCookie(const QNetworkCookie &cookie) override;
	bool forceInsertCookie(const QNetworkCookie &cookie);
	bool forceUpdateCookie(const QNetworkCookie &cookie);
	bool forceDeleteCookie(const QNetworkCookie &cookie);
	bool hasCookie(const QNetworkCookie &cookie) const;
	static bool isDomainTheSame(const QUrl &first, const QUrl &second);

protected:
	void timerEvent(QTimerEvent *event) override;
	void scheduleSave();
	void save();

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	CookiesPolicy m_generalCookiesPolicy;
	CookiesPolicy m_thirdPartyCookiesPolicy;
	KeepMode m_keepMode;
	int m_saveTimer;
	bool m_isPrivate;

signals:
	void cookieAdded(QNetworkCookie cookie);
	void cookieRemoved(QNetworkCookie cookie);
};

}

#endif
