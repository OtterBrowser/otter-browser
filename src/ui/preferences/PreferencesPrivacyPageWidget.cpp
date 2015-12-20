/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesPrivacyPageWidget.h"
#include "CookiesExceptionsDialog.h"
#include "../ClearHistoryDialog.h"
#include "../../core/SettingsManager.h"

#include "ui_PreferencesPrivacyPageWidget.h"

namespace Otter
{

PreferencesPrivacyPageWidget::PreferencesPrivacyPageWidget(QWidget *parent) : QWidget(parent),
	m_thirdPartyCookiesAcceptedHosts(SettingsManager::getValue(QLatin1String("Network/ThirdPartyCookiesAcceptedHosts")).toStringList()),
	m_thirdPartyCookiesRejectedHosts(SettingsManager::getValue(QLatin1String("Network/ThirdPartyCookiesRejectedHosts")).toStringList()),
	m_clearHisorySettings(SettingsManager::getValue(QLatin1String("History/ClearOnClose")).toStringList()),
	m_ui(new Ui::PreferencesPrivacyPageWidget)
{
	m_clearHisorySettings.removeAll(QString());

	m_ui->setupUi(this);
	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I do not want to be tracked"), QLatin1String("doNotAllow"));
	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I allow tracking"), QLatin1String("allow"));
	m_ui->doNotTrackComboBox->addItem(tr("Do not inform websites about my preference"), QLatin1String("skip"));

	const int doNotTrackPolicyIndex = m_ui->doNotTrackComboBox->findData(SettingsManager::getValue(QLatin1String("Network/DoNotTrackPolicy")).toString());

	m_ui->doNotTrackComboBox->setCurrentIndex((doNotTrackPolicyIndex < 0) ? 2 : doNotTrackPolicyIndex);
	m_ui->privateModeCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());
	m_ui->historyWidget->setDisabled(m_ui->privateModeCheckBox->isChecked());
	m_ui->rememberBrowsingHistoryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("History/RememberBrowsing")).toBool());
	m_ui->rememberDownloadsHistoryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("History/RememberDownloads")).toBool());
	m_ui->enableCookiesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Network/CookiesPolicy")).toString() != QLatin1String("ignore"));
	m_ui->cookiesWidget->setEnabled(m_ui->enableCookiesCheckBox->isChecked());
	m_ui->cookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only read existing"), QLatin1String("readOnly"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	const int cookiesPolicyIndex = m_ui->cookiesPolicyComboBox->findData(SettingsManager::getValue(QLatin1String("Network/CookiesPolicy")).toString());

	m_ui->cookiesPolicyComboBox->setCurrentIndex((cookiesPolicyIndex < 0) ? 0 : cookiesPolicyIndex);
	m_ui->keepCookiesModeComboBox->addItem(tr("Expires"), QLatin1String("keepUntilExpires"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Current session is closed"), QLatin1String("keepUntilExit"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Always ask"), QLatin1String("ask"));

	const int cookiesKeepModeIndex = m_ui->keepCookiesModeComboBox->findData(SettingsManager::getValue(QLatin1String("Network/CookiesKeepMode")).toString());

	m_ui->keepCookiesModeComboBox->setCurrentIndex((cookiesKeepModeIndex < 0) ? 0 : cookiesKeepModeIndex);
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	const int thirdPartyCookiesIndex = m_ui->thirdPartyCookiesPolicyComboBox->findData(SettingsManager::getValue(QLatin1String("Network/ThirdPartyCookiesPolicy")).toString());

	m_ui->thirdPartyCookiesPolicyComboBox->setCurrentIndex((thirdPartyCookiesIndex < 0) ? 0 : thirdPartyCookiesIndex);
	m_ui->clearHistoryCheckBox->setChecked(!m_clearHisorySettings.isEmpty());
	m_ui->clearHistoryButton->setEnabled(!m_clearHisorySettings.isEmpty());

	connect(m_ui->privateModeCheckBox, SIGNAL(toggled(bool)), m_ui->historyWidget, SLOT(setDisabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->cookiesWidget, SLOT(setEnabled(bool)));
	connect(m_ui->thirdPartyCookiesExceptionsButton, SIGNAL(clicked(bool)), this, SLOT(setupThirdPartyCookiesExceptions()));
	connect(m_ui->clearHistoryCheckBox, SIGNAL(toggled(bool)), m_ui->clearHistoryButton, SLOT(setEnabled(bool)));
	connect(m_ui->clearHistoryButton, SIGNAL(clicked()), this, SLOT(setupClearHistory()));
}

PreferencesPrivacyPageWidget::~PreferencesPrivacyPageWidget()
{
	delete m_ui;
}

void PreferencesPrivacyPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesPrivacyPageWidget::setupThirdPartyCookiesExceptions()
{
	CookiesExceptionsDialog dialog(m_thirdPartyCookiesAcceptedHosts, m_thirdPartyCookiesRejectedHosts, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_thirdPartyCookiesAcceptedHosts = dialog.getAcceptedHosts();
		m_thirdPartyCookiesRejectedHosts = dialog.getRejectedHosts();

		emit settingsModified();
	}
}

void PreferencesPrivacyPageWidget::setupClearHistory()
{
	ClearHistoryDialog dialog(m_clearHisorySettings, true, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_clearHisorySettings = dialog.getClearSettings();

		emit settingsModified();
	}

	m_ui->clearHistoryCheckBox->setChecked(!m_clearHisorySettings.isEmpty());
	m_ui->clearHistoryButton->setEnabled(!m_clearHisorySettings.isEmpty());
}

void PreferencesPrivacyPageWidget::save()
{
	SettingsManager::setValue(QLatin1String("Network/DoNotTrackPolicy"), m_ui->doNotTrackComboBox->currentData().toString());
	SettingsManager::setValue(QLatin1String("Browser/PrivateMode"), m_ui->privateModeCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("History/RememberBrowsing"), m_ui->rememberBrowsingHistoryCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("History/RememberDownloads"), m_ui->rememberDownloadsHistoryCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Network/CookiesPolicy"), (m_ui->enableCookiesCheckBox->isChecked() ? m_ui->cookiesPolicyComboBox->currentData().toString() : QLatin1String("ignore")));
	SettingsManager::setValue(QLatin1String("Network/CookiesKeepMode"), m_ui->keepCookiesModeComboBox->currentData().toString());
	SettingsManager::setValue(QLatin1String("Network/ThirdPartyCookiesPolicy"), m_ui->thirdPartyCookiesPolicyComboBox->currentData().toString());
	SettingsManager::setValue(QLatin1String("Network/ThirdPartyCookiesAcceptedHosts"), m_thirdPartyCookiesAcceptedHosts);
	SettingsManager::setValue(QLatin1String("Network/ThirdPartyCookiesRejectedHosts"), m_thirdPartyCookiesRejectedHosts);
	SettingsManager::setValue(QLatin1String("History/ClearOnClose"), (m_ui->clearHistoryCheckBox->isChecked() ? m_clearHisorySettings : QStringList()));
}

}
