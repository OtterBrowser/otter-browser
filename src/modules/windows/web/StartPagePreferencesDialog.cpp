/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StartPagePreferencesDialog.h"
#include "../../../core/SettingsManager.h"

#include "ui_StartPagePreferencesDialog.h"

#include <QtWidgets/QPushButton>

namespace Otter
{

StartPagePreferencesDialog::StartPagePreferencesDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::StartPagePreferencesDialog)
{
	m_ui->setupUi(this);
	m_ui->columnsPerRowSpinBox->setValue(SettingsManager::getValue(QLatin1String("StartPage/TilesPerRow")).toInt());
	m_ui->zoomLevelSpinBox->setValue(SettingsManager::getValue(QLatin1String("StartPage/ZoomLevel")).toInt());
	m_ui->showAddTileCheckBox->setChecked(SettingsManager::getValue(QLatin1String("StartPage/ShowAddTile")).toBool());

	connect(this, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(save()));
}

StartPagePreferencesDialog::~StartPagePreferencesDialog()
{
	delete m_ui;
}

void StartPagePreferencesDialog::changeEvent(QEvent *event)
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

void StartPagePreferencesDialog::save()
{
	SettingsManager::setValue(QLatin1String("StartPage/TilesPerRow"), m_ui->columnsPerRowSpinBox->value());
	SettingsManager::setValue(QLatin1String("StartPage/ZoomLevel"), m_ui->zoomLevelSpinBox->value());
	SettingsManager::setValue(QLatin1String("StartPage/ShowAddTile"), m_ui->showAddTileCheckBox->isChecked());
}

}
