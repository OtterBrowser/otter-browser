/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PrivacyPreferencesPage.h"
#include "../../../core/Application.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/ClearHistoryDialog.h"
#include "../../../ui/preferences/CookiesExceptionsDialog.h"

#include "ui_PrivacyPreferencesPage.h"

namespace Otter
{

PrivacyPreferencesPage::PrivacyPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_thirdPartyCookiesAcceptedHosts(SettingsManager::getOption(SettingsManager::Network_ThirdPartyCookiesAcceptedHostsOption).toStringList()),
	m_thirdPartyCookiesRejectedHosts(SettingsManager::getOption(SettingsManager::Network_ThirdPartyCookiesRejectedHostsOption).toStringList()),
	m_clearHistorySettings(SettingsManager::getOption(SettingsManager::History_ClearOnCloseOption).toStringList()),
	m_ui(nullptr)
{
	m_clearHistorySettings.removeAll({});
}

PrivacyPreferencesPage::~PrivacyPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void PrivacyPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() != QEvent::LanguageChange || !wasLoaded())
	{
		return;
	}

	m_ui->retranslateUi(this);
	m_ui->doNotTrackComboBox->setItemText(0, tr("Inform websites that I do not want to be tracked"));
	m_ui->doNotTrackComboBox->setItemText(1, tr("Inform websites that I allow tracking"));
	m_ui->doNotTrackComboBox->setItemText(2, tr("Do not inform websites about my preference"));

	m_ui->cookiesPolicyComboBox->setItemText(0, tr("Always"));
	m_ui->cookiesPolicyComboBox->setItemText(1, tr("Only existing"));
	m_ui->cookiesPolicyComboBox->setItemText(2, tr("Only read existing"));

	m_ui->keepCookiesModeComboBox->setItemText(0, tr("Expires"));
	m_ui->keepCookiesModeComboBox->setItemText(1, tr("Current session is closed"));
	m_ui->keepCookiesModeComboBox->setItemText(2, tr("Always ask"));

	m_ui->thirdPartyCookiesPolicyComboBox->setItemText(0, tr("Always"));
	m_ui->thirdPartyCookiesPolicyComboBox->setItemText(1, tr("Only existing"));
	m_ui->thirdPartyCookiesPolicyComboBox->setItemText(2, tr("Never"));
}

void PrivacyPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::PrivacyPreferencesPage();
	m_ui->setupUi(this);
	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I do not want to be tracked"), QLatin1String("doNotAllow"));
	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I allow tracking"), QLatin1String("allow"));
	m_ui->doNotTrackComboBox->addItem(tr("Do not inform websites about my preference"), QLatin1String("skip"));

	const int doNotTrackPolicyIndex(m_ui->doNotTrackComboBox->findData(SettingsManager::getOption(SettingsManager::Network_DoNotTrackPolicyOption).toString()));

	m_ui->doNotTrackComboBox->setCurrentIndex((doNotTrackPolicyIndex < 0) ? 2 : doNotTrackPolicyIndex);
	m_ui->privateModeCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_PrivateModeOption).toBool());
	m_ui->historyWidget->setDisabled(m_ui->privateModeCheckBox->isChecked());
	m_ui->rememberBrowsingHistoryCheckBox->setChecked(SettingsManager::getOption(SettingsManager::History_RememberBrowsingOption).toBool());
	m_ui->rememberDownloadsHistoryCheckBox->setChecked(SettingsManager::getOption(SettingsManager::History_RememberDownloadsOption).toBool());
	m_ui->enableCookiesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Network_CookiesPolicyOption).toString() != QLatin1String("ignore"));
	m_ui->cookiesWidget->setEnabled(m_ui->enableCookiesCheckBox->isChecked());
	m_ui->cookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only read existing"), QLatin1String("readOnly"));

	const int cookiesPolicyIndex(m_ui->cookiesPolicyComboBox->findData(SettingsManager::getOption(SettingsManager::Network_CookiesPolicyOption).toString()));

	m_ui->cookiesPolicyComboBox->setCurrentIndex((cookiesPolicyIndex < 0) ? 0 : cookiesPolicyIndex);
	m_ui->keepCookiesModeComboBox->addItem(tr("Expires"), QLatin1String("keepUntilExpires"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Current session is closed"), QLatin1String("keepUntilExit"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Always ask"), QLatin1String("ask"));

	const int cookiesKeepModeIndex(m_ui->keepCookiesModeComboBox->findData(SettingsManager::getOption(SettingsManager::Network_CookiesKeepModeOption).toString()));

	m_ui->keepCookiesModeComboBox->setCurrentIndex((cookiesKeepModeIndex < 0) ? 0 : cookiesKeepModeIndex);
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	const int thirdPartyCookiesIndex(m_ui->thirdPartyCookiesPolicyComboBox->findData(SettingsManager::getOption(SettingsManager::Network_ThirdPartyCookiesPolicyOption).toString()));

	m_ui->thirdPartyCookiesPolicyComboBox->setCurrentIndex((thirdPartyCookiesIndex < 0) ? 0 : thirdPartyCookiesIndex);
	m_ui->clearHistoryCheckBox->setChecked(!m_clearHistorySettings.isEmpty());
	m_ui->clearHistoryButton->setEnabled(!m_clearHistorySettings.isEmpty());

	m_ui->rememberPasswordsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_RememberPasswordsOption).toBool());

	connect(m_ui->privateModeCheckBox, &QCheckBox::toggled, m_ui->historyWidget, &QWidget::setDisabled);
	connect(m_ui->enableCookiesCheckBox, &QCheckBox::toggled, m_ui->cookiesWidget, &QWidget::setEnabled);
	connect(m_ui->thirdPartyCookiesExceptionsButton, &QPushButton::clicked, this, [&]()
	{
		CookiesExceptionsDialog dialog(m_thirdPartyCookiesAcceptedHosts, m_thirdPartyCookiesRejectedHosts, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			m_thirdPartyCookiesAcceptedHosts = dialog.getAcceptedHosts();
			m_thirdPartyCookiesRejectedHosts = dialog.getRejectedHosts();

			emit settingsModified();
		}
	});
	connect(m_ui->clearHistoryCheckBox, &QCheckBox::toggled, m_ui->clearHistoryButton, &QPushButton::setEnabled);
	connect(m_ui->clearHistoryCheckBox, &QCheckBox::toggled, this, [&](bool isChecked)
	{
		if (isChecked && m_clearHistorySettings.isEmpty())
		{
			m_clearHistorySettings = SettingsManager::getOptionDefinition(SettingsManager::History_ManualClearOptionsOption).defaultValue.toStringList();

			emit settingsModified();
		}
	});
	connect(m_ui->clearHistoryButton, &QPushButton::clicked, this, [&]()
	{
		ClearHistoryDialog dialog(m_clearHistorySettings, true, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			m_clearHistorySettings = dialog.getClearSettings();

			emit settingsModified();
		}

		m_ui->clearHistoryCheckBox->setChecked(!m_clearHistorySettings.isEmpty());
		m_ui->clearHistoryButton->setEnabled(!m_clearHistorySettings.isEmpty());
	});
	connect(m_ui->managePasswordsButton, &QPushButton::clicked, this, [&]()
	{
		Application::triggerAction(ActionsManager::PasswordsAction, {}, this);
	});

	markAsLoaded();
}

void PrivacyPreferencesPage::save()
{
	SettingsManager::setOption(SettingsManager::Network_DoNotTrackPolicyOption, m_ui->doNotTrackComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Browser_PrivateModeOption, m_ui->privateModeCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::History_RememberBrowsingOption, m_ui->rememberBrowsingHistoryCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::History_RememberDownloadsOption, m_ui->rememberDownloadsHistoryCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Network_CookiesPolicyOption, (m_ui->enableCookiesCheckBox->isChecked() ? m_ui->cookiesPolicyComboBox->currentData().toString() : QLatin1String("ignore")));
	SettingsManager::setOption(SettingsManager::Network_CookiesKeepModeOption, m_ui->keepCookiesModeComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Network_ThirdPartyCookiesPolicyOption, m_ui->thirdPartyCookiesPolicyComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Network_ThirdPartyCookiesAcceptedHostsOption, m_thirdPartyCookiesAcceptedHosts);
	SettingsManager::setOption(SettingsManager::Network_ThirdPartyCookiesRejectedHostsOption, m_thirdPartyCookiesRejectedHosts);
	SettingsManager::setOption(SettingsManager::History_ClearOnCloseOption, (m_ui->clearHistoryCheckBox->isChecked() ? m_clearHistorySettings : QStringList()));
	SettingsManager::setOption(SettingsManager::Browser_RememberPasswordsOption, m_ui->rememberPasswordsCheckBox->isChecked());
}

QString PrivacyPreferencesPage::getTitle() const
{
	return tr("Privacy");
}

}
