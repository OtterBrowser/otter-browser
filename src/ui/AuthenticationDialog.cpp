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

#include "AuthenticationDialog.h"

#include "ui_AuthenticationDialog.h"

namespace Otter
{

AuthenticationDialog::AuthenticationDialog(const QUrl &url, QAuthenticator *authenticator, QWidget *parent) : QDialog(parent),
	m_authenticator(authenticator),
	m_ui(new Ui::AuthenticationDialog)
{
	m_ui->setupUi(this);
	m_ui->serverValueLabel->setText(url.host());
	m_ui->messageValueLabel->setText(authenticator->realm().toHtmlEscaped());
	m_ui->userLineEdit->setText(authenticator->user());
	m_ui->passwordLineEdit->setText(authenticator->password());

	connect(this, SIGNAL(accepted()), this, SLOT(setup()));
}

AuthenticationDialog::~AuthenticationDialog()
{
	delete m_ui;
}

void AuthenticationDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void AuthenticationDialog::setup()
{
	m_authenticator->setUser(m_ui->userLineEdit->text());
	m_authenticator->setPassword(m_ui->passwordLineEdit->text());
}

}
