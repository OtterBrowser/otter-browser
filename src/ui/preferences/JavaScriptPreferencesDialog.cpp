/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "JavaScriptPreferencesDialog.h"
#include "../../core/SettingsManager.h"

#include "ui_JavaScriptPreferencesDialog.h"

namespace Otter
{

JavaScriptPreferencesDialog::JavaScriptPreferencesDialog(const QHash<int, QVariant> &options, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::JavaScriptPreferencesDialog)
{
	m_ui->setupUi(this);
	m_ui->canChangeWindowGeometryCheckBox->setChecked(options.value(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption).toBool());
	m_ui->canShowStatusMessagesCheckBox->setChecked(options.value(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption).toBool());
	m_ui->canAccessClipboardCheckBox->setChecked(options.value(SettingsManager::Permissions_ScriptsCanAccessClipboardOption).toBool());
	m_ui->canReceiveRightClicksCheckBox->setChecked(options.value(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption).toBool());
	m_ui->canCloseWindowsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Never"), QLatin1String("disallow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->enableFullScreenComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	const int canCloseWindowsIndex(m_ui->canCloseWindowsComboBox->findData(options.value(SettingsManager::Permissions_ScriptsCanCloseWindowsOption).toString()));

	m_ui->canCloseWindowsComboBox->setCurrentIndex((canCloseWindowsIndex < 0) ? 0 : canCloseWindowsIndex);

	const int enableFullScreenIndex(m_ui->enableFullScreenComboBox->findData(options.value(SettingsManager::Permissions_EnableFullScreenOption).toString()));

	m_ui->enableFullScreenComboBox->setCurrentIndex((enableFullScreenIndex < 0) ? 0 : enableFullScreenIndex);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &JavaScriptPreferencesDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &JavaScriptPreferencesDialog::reject);
}

JavaScriptPreferencesDialog::~JavaScriptPreferencesDialog()
{
	delete m_ui;
}

void JavaScriptPreferencesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

QHash<int, QVariant> JavaScriptPreferencesDialog::getOptions() const
{
	QHash<int, QVariant> options;
	options[SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption] = m_ui->canChangeWindowGeometryCheckBox->isChecked();
	options[SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption] = m_ui->canShowStatusMessagesCheckBox->isChecked();
	options[SettingsManager::Permissions_ScriptsCanAccessClipboardOption] = m_ui->canAccessClipboardCheckBox->isChecked();
	options[SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption] = m_ui->canReceiveRightClicksCheckBox->isChecked();
	options[SettingsManager::Permissions_ScriptsCanCloseWindowsOption] = m_ui->canCloseWindowsComboBox->currentData().toString();
	options[SettingsManager::Permissions_EnableFullScreenOption] = m_ui->enableFullScreenComboBox->currentData().toString();

	return options;
}

}
