/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACCEPTCOOKIEDIALOG_H
#define OTTER_ACCEPTCOOKIEDIALOG_H

#include "../core/CookieJar.h"
#include "../ui/Dialog.h"

#include <QtWidgets/QAbstractButton>

namespace Otter
{

namespace Ui
{
	class AcceptCookieDialog;
}

class AcceptCookieDialog final : public Dialog
{
	Q_OBJECT

public:
	enum AcceptCookieResult
	{
		AcceptCookie,
		AcceptAsSessionCookie,
		IgnoreCookie
	};

	explicit AcceptCookieDialog(const QNetworkCookie &cookie, CookieJar::CookieOperation operation, CookieJar *cookieJar, QWidget *parent = nullptr);
	~AcceptCookieDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void handleButtonClicked(QAbstractButton *button);

private:
	CookieJar *m_cookieJar;
	QNetworkCookie m_cookie;
	CookieJar::CookieOperation m_operation;
	Ui::AcceptCookieDialog *m_ui;
};

}

#endif
