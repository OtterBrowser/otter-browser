/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ReloadTimeDialog.h"

#include "ui_ReloadTimeDialog.h"

namespace Otter
{

ReloadTimeDialog::ReloadTimeDialog(int time, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::ReloadTimeDialog)
{
	m_ui->setupUi(this);

	const int minutes(time / 60);

	m_ui->minutesSpinBox->setValue(minutes);
	m_ui->secondsSpinBox->setValue(time - (minutes * 60));
}

ReloadTimeDialog::~ReloadTimeDialog()
{
	delete m_ui;
}

void ReloadTimeDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

int ReloadTimeDialog::getReloadTime() const
{
	return ((m_ui->minutesSpinBox->value() * 60) + m_ui->secondsSpinBox->value());
}

}
