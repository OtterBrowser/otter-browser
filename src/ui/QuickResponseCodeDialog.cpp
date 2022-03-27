/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QuickResponseCodeDialog.h"

#include "ui_QuickResponseCodeDialog.h"

namespace Otter
{

QuickResponseCodeDialog::QuickResponseCodeDialog(const QString &message, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::QuickResponseCodeDialog)
{
	m_ui->setupUi(this);
	m_ui->messageLineEdit->setText(message);

	connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, &QuickResponseCodeDialog::handleButtonClicked);
}

QuickResponseCodeDialog::~QuickResponseCodeDialog()
{
	delete m_ui;
}

void QuickResponseCodeDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void QuickResponseCodeDialog::handleButtonClicked(QAbstractButton *button)
{
	if (m_ui->buttonBox->standardButton(button) == QDialogButtonBox::Save)
	{
///TODO
	}
}

}
