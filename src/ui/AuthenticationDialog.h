/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_AUTHENTICATIONDIALOG_H
#define OTTER_AUTHENTICATIONDIALOG_H

#include "Dialog.h"

#include <QtCore/QUrl>
#include <QtNetwork/QAuthenticator>

namespace Otter
{

namespace Ui
{
	class AuthenticationDialog;
}

class AuthenticationDialog : public Dialog
{
	Q_OBJECT

public:
	enum AuthenticationType
	{
		UnknownAuthentication = 0,
		HttpAuthentication,
		ProxyAuthentication
	};

	explicit AuthenticationDialog(const QUrl &url, QAuthenticator *authenticator, AuthenticationType type, QWidget *parent = nullptr);
	~AuthenticationDialog();

	void setButtonsVisible(bool visible);

public slots:
	void authenticated(QAuthenticator *authenticator, bool wasAccepted);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void setup();

private:
	QAuthenticator *m_authenticator;
	QUrl m_url;
	AuthenticationType m_type;
	Ui::AuthenticationDialog *m_ui;
};

}

#endif
