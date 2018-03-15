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

#include "ClearHistoryDialog.h"
#include "../core/HistoryManager.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/PasswordsManager.h"
#include "../core/SettingsManager.h"
#include "../core/TransfersManager.h"

#include "ui_ClearHistoryDialog.h"

#include <QtWidgets/QPushButton>

namespace Otter
{

ClearHistoryDialog::ClearHistoryDialog(const QStringList &clearSettings, bool isConfiguring, QWidget *parent) : Dialog(parent),
	m_isConfiguring(isConfiguring),
	m_ui(new Ui::ClearHistoryDialog)
{
	m_ui->setupUi(this);

	QStringList settings(clearSettings);
	settings.removeAll({});

	if (settings.isEmpty())
	{
		settings = getDefaultClearSettings();
	}

	if (m_isConfiguring)
	{
		m_ui->periodWidget->hide();
	}
	else
	{
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Clear Now"));
		m_ui->periodSpinBox->setValue(SettingsManager::getOption(SettingsManager::History_ManualClearPeriodOption).toInt());

		connect(this, &ClearHistoryDialog::accepted, this, &ClearHistoryDialog::clearHistory);
	}

	m_ui->clearBrowsingHistoryCheckBox->setChecked(settings.contains(QLatin1String("browsing")));
	m_ui->clearCookiesCheckBox->setChecked(settings.contains(QLatin1String("cookies")));
	m_ui->clearFormsHistoryCheckBox->setChecked(settings.contains(QLatin1String("forms")));
	m_ui->clearDownloadsHistoryCheckBox->setChecked(settings.contains(QLatin1String("downloads")));
	m_ui->clearSearchHistoryCheckBox->setChecked(settings.contains(QLatin1String("search")));
	m_ui->clearCachesCheckBox->setChecked(settings.contains(QLatin1String("caches")));
	m_ui->clearStorageCheckBox->setChecked(settings.contains(QLatin1String("storage")));
	m_ui->clearPasswordsCheckBox->setChecked(settings.contains(QLatin1String("passwords")));
}

ClearHistoryDialog::~ClearHistoryDialog()
{
	delete m_ui;
}

void ClearHistoryDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		if (!m_isConfiguring)
		{
			m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Clear Now"));
		}
	}
}

void ClearHistoryDialog::clearHistory()
{
	if (m_ui->clearBrowsingHistoryCheckBox->isChecked())
	{
		HistoryManager::clearHistory(static_cast<uint>(m_ui->periodSpinBox->value()));
	}

	if (m_ui->clearCookiesCheckBox->isChecked())
	{
		NetworkManagerFactory::clearCookies(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearDownloadsHistoryCheckBox->isChecked())
	{
		TransfersManager::clearTransfers(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearCachesCheckBox->isChecked())
	{
		NetworkManagerFactory::clearCache(m_ui->periodSpinBox->value());
	}

	if (m_ui->clearPasswordsCheckBox->isChecked())
	{
		PasswordsManager::clearPasswords(m_ui->periodSpinBox->value());
	}

	SettingsManager::setOption(SettingsManager::History_ManualClearOptionsOption, getClearSettings());
	SettingsManager::setOption(SettingsManager::History_ManualClearPeriodOption, m_ui->periodSpinBox->value());
}

QStringList ClearHistoryDialog::getClearSettings() const
{
	QStringList clearSettings;

	if (m_ui->clearBrowsingHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("browsing"));
	}

	if (m_ui->clearCookiesCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("cookies"));
	}

	if (m_ui->clearFormsHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("forms"));
	}

	if (m_ui->clearDownloadsHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("downloads"));
	}

	if (m_ui->clearSearchHistoryCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("search"));
	}

	if (m_ui->clearCachesCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("caches"));
	}

	if (m_ui->clearStorageCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("storage"));
	}

	if (m_ui->clearPasswordsCheckBox->isChecked())
	{
		clearSettings.append(QLatin1String("passwords"));
	}

	return clearSettings;
}

QStringList ClearHistoryDialog::getDefaultClearSettings()
{
	return {QLatin1String("browsing"), QLatin1String("cookies"), QLatin1String("forms"), QLatin1String("downloads"), QLatin1String("caches")};
}

}
