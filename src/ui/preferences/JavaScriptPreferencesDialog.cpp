/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "JavaScriptPreferencesDialog.h"
#include "../../core/SettingsManager.h"

#include "ui_JavaScriptPreferencesDialog.h"

namespace Otter
{

JavaScriptPreferencesDialog::JavaScriptPreferencesDialog(const QHash<int, QVariant> &options, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::JavaScriptPreferencesDialog)
{
	m_ui->setupUi(this);
	m_ui->canChangeWindowGeometryCheckBox->setChecked(options.value(SettingsManager::Browser_JavaScriptCanChangeWindowGeometryOption).toBool());
	m_ui->canShowStatusMessagesCheckBox->setChecked(options.value(SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption).toBool());
	m_ui->canAccessClipboardCheckBox->setChecked(options.value(SettingsManager::Browser_JavaScriptCanAccessClipboardOption).toBool());
	m_ui->canDisableContextMenuCheckBox->setChecked(options.value(SettingsManager::Browser_JavaScriptCanDisableContextMenuOption).toBool());
	m_ui->canOpenWindowsCheckBox->setChecked(options.value(SettingsManager::Browser_JavaScriptCanOpenWindowsOption).toBool());
	m_ui->canCloseWindowsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Never"), QLatin1String("disallow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->enableFullScreenComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	const int canCloseWindowsIndex(m_ui->canCloseWindowsComboBox->findData(options.value(SettingsManager::Browser_JavaScriptCanCloseWindowsOption).toString()));

	m_ui->canCloseWindowsComboBox->setCurrentIndex((canCloseWindowsIndex < 0) ? 0 : canCloseWindowsIndex);

	const int enableFullScreenIndex(m_ui->enableFullScreenComboBox->findData(options.value(SettingsManager::Browser_EnableFullScreenOption).toString()));

	m_ui->enableFullScreenComboBox->setCurrentIndex((enableFullScreenIndex < 0) ? 0 : enableFullScreenIndex);

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
	options[SettingsManager::Browser_JavaScriptCanChangeWindowGeometryOption] = m_ui->canChangeWindowGeometryCheckBox->isChecked();
	options[SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption] = m_ui->canShowStatusMessagesCheckBox->isChecked();
	options[SettingsManager::Browser_JavaScriptCanAccessClipboardOption] = m_ui->canAccessClipboardCheckBox->isChecked();
	options[SettingsManager::Browser_JavaScriptCanDisableContextMenuOption] = m_ui->canDisableContextMenuCheckBox->isChecked();
	options[SettingsManager::Browser_JavaScriptCanOpenWindowsOption] = m_ui->canOpenWindowsCheckBox->isChecked();
	options[SettingsManager::Browser_JavaScriptCanCloseWindowsOption] = m_ui->canCloseWindowsComboBox->currentData().toString();
	options[SettingsManager::Browser_EnableFullScreenOption] = m_ui->enableFullScreenComboBox->currentData().toString();

	return options;
}

}
