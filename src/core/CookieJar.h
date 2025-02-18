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

#ifndef OTTER_COOKIEJAR_H
#define OTTER_COOKIEJAR_H

#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>

namespace Otter
{

class CookieJar : public QNetworkCookieJar
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

	explicit CookieJar(QObject *parent = nullptr);

	virtual void clearCookies(int period = 0) = 0;
	QList<QNetworkCookie> getCookiesForUrl(const QUrl &url) const;
	QVector<QNetworkCookie> getCookies(const QString &domain = {}) const;
	virtual bool forceInsertCookie(const QNetworkCookie &cookie) = 0;
	virtual bool forceUpdateCookie(const QNetworkCookie &cookie) = 0;
	virtual bool forceDeleteCookie(const QNetworkCookie &cookie) = 0;
	bool hasCookie(const QNetworkCookie &cookie) const;

signals:
	void cookieAdded(const QNetworkCookie &cookie);
	void cookieModified(const QNetworkCookie &cookie);
	void cookieRemoved(const QNetworkCookie &cookie);
};

class DiskCookieJar final : public CookieJar
{
	Q_OBJECT

public:
	explicit DiskCookieJar(const QString &path, QObject *parent = nullptr);

	void clearCookies(int period = 0) override;
	QString getPath() const;
	QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
	bool insertCookie(const QNetworkCookie &cookie) override;
	bool updateCookie(const QNetworkCookie &cookie) override;
	bool deleteCookie(const QNetworkCookie &cookie) override;
	bool forceInsertCookie(const QNetworkCookie &cookie) override;
	bool forceUpdateCookie(const QNetworkCookie &cookie) override;
	bool forceDeleteCookie(const QNetworkCookie &cookie) override;

protected:
	void timerEvent(QTimerEvent *event) override;
	void loadCookies(const QString &path);
	void scheduleSave();
	void save();

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	QString m_path;
	CookiesPolicy m_generalCookiesPolicy;
	CookiesPolicy m_thirdPartyCookiesPolicy;
	KeepMode m_keepMode;
	int m_saveTimer;
};

}

#endif
