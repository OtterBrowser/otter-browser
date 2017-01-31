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

#include "UserAgentPropertiesDialog.h"

#include "ui_UserAgentPropertiesDialog.h"

namespace Otter
{

UserAgentPropertiesDialog::UserAgentPropertiesDialog(const UserAgentDefinition &userAgent, bool isDefault, QWidget *parent) : Dialog(parent),
	m_userAgent(userAgent),
	m_ui(new Ui::UserAgentPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(userAgent.getTitle());
	m_ui->valueLineEdit->setText(userAgent.value);
	m_ui->isDefaultUserAgentCheckBox->setChecked(isDefault);
}

UserAgentPropertiesDialog::~UserAgentPropertiesDialog()
{
	delete m_ui;
}

void UserAgentPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

UserAgentDefinition UserAgentPropertiesDialog::getUserAgent() const
{
	UserAgentDefinition userAgent(m_userAgent);
	userAgent.title = m_ui->titleLineEdit->text();
	userAgent.value = m_ui->valueLineEdit->text();

	return userAgent;
}

}
