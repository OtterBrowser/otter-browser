/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_COOKIEPROPERTIESDIALOG_H
#define OTTER_COOKIEPROPERTIESDIALOG_H

#include "Dialog.h"

#include <QtNetwork/QNetworkCookie>

namespace Otter
{

namespace Ui
{
	class CookiePropertiesDialog;
}

class CookiePropertiesDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit CookiePropertiesDialog(const QNetworkCookie &cookie, QWidget *parent = nullptr);
	~CookiePropertiesDialog();

	QNetworkCookie getOriginalCookie() const;
	QNetworkCookie getModifiedCookie() const;
	bool isModified() const;

protected:
	void changeEvent(QEvent *event) override;

private:
	QNetworkCookie m_cookie;
	Ui::CookiePropertiesDialog *m_ui;
};

}

#endif
