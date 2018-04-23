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

#include "AuthenticationDialog.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/PasswordsManager.h"
#include "../core/SettingsManager.h"

#include "ui_AuthenticationDialog.h"

namespace Otter
{

AuthenticationDialog::AuthenticationDialog(const QUrl &url, QAuthenticator *authenticator, AuthenticationType type, QWidget *parent) : Dialog(parent),
	m_authenticator(authenticator),
	m_url(url),
	m_type(type),
	m_ui(new Ui::AuthenticationDialog)
{
	m_ui->setupUi(this);
	m_ui->serverValueLabel->setText(url.host());
	m_ui->messageValueLabel->setText(authenticator->realm());
	m_ui->userComboBox->setCurrentText(authenticator->user());
	m_ui->passwordLineEditWidget->setText(authenticator->password());

	if (type == HttpAuthentication)
	{
		m_ui->rememberPasswordCheckBox->setEnabled(SettingsManager::getOption(SettingsManager::Browser_RememberPasswordsOption).toBool());
	}
	else
	{
		m_ui->rememberPasswordCheckBox->hide();
	}

	connect(NetworkManagerFactory::getInstance(), &NetworkManagerFactory::authenticated, this, &AuthenticationDialog::handleAuthenticated);
	connect(this, &AuthenticationDialog::accepted, this, &AuthenticationDialog::setup);
}

AuthenticationDialog::~AuthenticationDialog()
{
	delete m_ui;
}

void AuthenticationDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void AuthenticationDialog::setup()
{
	m_authenticator->setUser(m_ui->userComboBox->currentText());
	m_authenticator->setPassword(m_ui->passwordLineEditWidget->text());

	if (m_ui->rememberPasswordCheckBox->isChecked() && !m_authenticator->user().isEmpty() && !m_authenticator->password().isEmpty())
	{
		PasswordsManager::PasswordInformation::Field userField;
		userField.value = m_authenticator->user();
		userField.type = PasswordsManager::TextField;

		PasswordsManager::PasswordInformation::Field passwordField;
		passwordField.value = m_authenticator->password();
		passwordField.type = PasswordsManager::PasswordField;

		PasswordsManager::PasswordInformation password;
		password.url = m_url.toString();
		password.timeAdded = QDateTime::currentDateTimeUtc();
		password.type = PasswordsManager::AuthPassword;
		password.fields = {userField, passwordField};

		PasswordsManager::addPassword(password);
	}
}

void AuthenticationDialog::handleAuthenticated(QAuthenticator *authenticator, bool wasAccepted)
{
	if (*authenticator == *m_authenticator)
	{
		disconnect(NetworkManagerFactory::getInstance(), &NetworkManagerFactory::authenticated, this, &AuthenticationDialog::handleAuthenticated);
		disconnect(this, &AuthenticationDialog::accepted, this, &AuthenticationDialog::setup);

		if (wasAccepted)
		{
			accept();
		}
		else
		{
			reject();
		}
	}
}

void AuthenticationDialog::setButtonsVisible(bool visible)
{
	m_ui->buttonBox->setVisible(visible);
}

}
