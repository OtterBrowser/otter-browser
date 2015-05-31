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

	const QString backgroundModeString = SettingsManager::getValue(QLatin1String("StartPage/BackgroundMode")).toString();

	m_ui->customBackgroundCheckBox->setChecked(backgroundModeString != QLatin1String("standard"));
	m_ui->backgroundFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("StartPage/BackgroundPath")).toString());
	m_ui->backgroundFilePathWidget->setFilter(tr("Images (*.png *.jpg *.bmp *.gif)"));
	m_ui->backgroundModeComboBox->addItem(tr("Best fit"), QLatin1String("bestFit"));
	m_ui->backgroundModeComboBox->addItem(tr("Center"), QLatin1String("center"));
	m_ui->backgroundModeComboBox->addItem(tr("Stretch"), QLatin1String("stretch"));
	m_ui->backgroundModeComboBox->addItem(tr("Tile"), QLatin1String("tile"));

	const int backgroundModeIndex = m_ui->backgroundModeComboBox->findData(backgroundModeString);

	m_ui->backgroundModeComboBox->setCurrentIndex((backgroundModeIndex < 0) ? 0 : backgroundModeIndex);
	m_ui->backgroundWidget->setEnabled(m_ui->customBackgroundCheckBox->isChecked());
	m_ui->columnsPerRowSpinBox->setValue(SettingsManager::getValue(QLatin1String("StartPage/TilesPerRow")).toInt());
	m_ui->zoomLevelSpinBox->setValue(SettingsManager::getValue(QLatin1String("StartPage/ZoomLevel")).toInt());
	m_ui->showAddTileCheckBox->setChecked(SettingsManager::getValue(QLatin1String("StartPage/ShowAddTile")).toBool());

	connect(this, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->customBackgroundCheckBox, SIGNAL(toggled(bool)), m_ui->backgroundWidget, SLOT(setEnabled(bool)));
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
	const QString backgroundModeString = m_ui->backgroundModeComboBox->currentData().toString();

	SettingsManager::setValue(QLatin1String("StartPage/BackgroundPath"), m_ui->backgroundFilePathWidget->getPath());
	SettingsManager::setValue(QLatin1String("StartPage/BackgroundMode"), (m_ui->customBackgroundCheckBox->isChecked() ? backgroundModeString : QLatin1String("system")));
	SettingsManager::setValue(QLatin1String("StartPage/TilesPerRow"), m_ui->columnsPerRowSpinBox->value());
	SettingsManager::setValue(QLatin1String("StartPage/ZoomLevel"), m_ui->zoomLevelSpinBox->value());
	SettingsManager::setValue(QLatin1String("StartPage/ShowAddTile"), m_ui->showAddTileCheckBox->isChecked());
}

}
