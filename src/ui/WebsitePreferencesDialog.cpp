/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "WebsitePreferencesDialog.h"
#include "OptionDelegate.h"
#include "preferences/ContentBlockingIntervalDelegate.h"
#include "../core/ContentBlockingManager.h"
#include "../core/ContentBlockingProfile.h"
#include "../core/NetworkManagerFactory.h"
#include "../core/SettingsManager.h"
#include "../core/SessionsManager.h"
#include "../core/Utils.h"

#include "ui_WebsitePreferencesDialog.h"

#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtCore/QTextCodec>

namespace Otter
{

WebsitePreferencesDialog::WebsitePreferencesDialog(const QUrl &url, const QList<QNetworkCookie> &cookies, QWidget *parent) : Dialog(parent),
	m_updateOverride(true),
	m_ui(new Ui::WebsitePreferencesDialog)
{
	const QList<int> textCodecs({106, 1015, 1017, 4, 5, 6, 7, 8, 82, 10, 85, 12, 13, 109, 110, 112, 2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 18, 39, 17, 38, 2026});

	m_ui->setupUi(this);
	m_ui->enableCookiesCheckBox->setChecked(true);

	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->cookiesPolicyOverrideCheckBox, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->cookiesPolicyLabel, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->cookiesPolicyComboBox, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->keepCookiesModeOverrideCheckBox, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->keepCookiesModeLabel, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->keepCookiesModeComboBox, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->thirdPartyCookiesPolicyOverrideCheckBox, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->thirdPartyCookiesPolicyLabel, SLOT(setEnabled(bool)));
	connect(m_ui->enableCookiesCheckBox, SIGNAL(toggled(bool)), m_ui->thirdPartyCookiesPolicyComboBox, SLOT(setEnabled(bool)));

	m_ui->websiteLineEdit->setText(url.isLocalFile() ? QLatin1String("localhost") : url.host());

	m_ui->encodingComboBox->addItem(tr("Auto Detect"));

	for (int i = 0; i < textCodecs.count(); ++i)
	{
		QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

		if (!codec)
		{
			continue;
		}

		m_ui->encodingComboBox->addItem(codec->name(), textCodecs.at(i));
	}

	m_ui->popupsPolicyComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->popupsPolicyComboBox->addItem(tr("Block all"), QLatin1String("blockAll"));
	m_ui->popupsPolicyComboBox->addItem(tr("Open all"), QLatin1String("openAll"));
	m_ui->popupsPolicyComboBox->addItem(tr("Open all in background"), QLatin1String("openAllInBackground"));

	m_ui->enableImagesComboBox->addItem(tr("All images"), QLatin1String("enabled"));
	m_ui->enableImagesComboBox->addItem(tr("Cached images"), QLatin1String("onlyCached"));
	m_ui->enableImagesComboBox->addItem(tr("No images"), QLatin1String("disabled"));

	m_ui->enablePluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->enablePluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->enablePluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	m_ui->canCloseWindowsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->canCloseWindowsComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	m_ui->enableFullScreenComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->enableFullScreenComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I do not want to be tracked"), QLatin1String("doNotAllow"));
	m_ui->doNotTrackComboBox->addItem(tr("Inform websites that I allow tracking"), QLatin1String("allow"));
	m_ui->doNotTrackComboBox->addItem(tr("Do not inform websites about my preference"), QLatin1String("skip"));

	m_ui->cookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Only read existing"), QLatin1String("readOnly"));
	m_ui->cookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	m_ui->keepCookiesModeComboBox->addItem(tr("Expires"), QLatin1String("keepUntilExpires"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Current session is closed"), QLatin1String("keepUntilExit"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Always ask"), QLatin1String("ask"));

	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	QStandardItemModel *cookiesModel(new QStandardItemModel(this));
	cookiesModel->setHorizontalHeaderLabels(QStringList({tr("Domain"), tr("Name"), tr("Path"), tr("Value"), tr("Expiration date")}));

	for (int i = 0; i < cookies.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(cookies.at(i).domain()));
		items.append(new QStandardItem(QString(cookies.at(i).name())));
		items.append(new QStandardItem(cookies.at(i).path()));
		items.append(new QStandardItem(QString(cookies.at(i).value())));
		items.append(new QStandardItem(Utils::formatDateTime(cookies.at(i).expirationDate())));

		cookiesModel->appendRow(items);
	}

	m_ui->cookiesTableWidget->setModel(cookiesModel);

	const QStringList userAgents(NetworkManagerFactory::getUserAgents());

	m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const UserAgentInformation userAgent(NetworkManagerFactory::getUserAgent(userAgents.at(i)));
		const QString title(userAgent.title);

		m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i));
		m_ui->userAgentComboBox->setItemData((i + 1), userAgent.value, (Qt::UserRole + 1));
	}

	m_ui->encodingOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Content/DefaultEncoding")));
	m_ui->popupsPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Content/PopupsPolicy")));
	m_ui->enableImagesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/EnableImages")));
	m_ui->enableJavaOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/EnableJava")));
	m_ui->enablePluginsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/EnablePlugins")));
	m_ui->userStyleSheetOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Content/UserStyleSheet")));
	m_ui->doNotTrackOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/DoNotTrackPolicy")));
	m_ui->rememberBrowsingHistoryOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("History/RememberBrowsing")));
	m_ui->enableCookiesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/CookiesPolicy")));
	m_ui->cookiesPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/CookiesPolicy")));
	m_ui->keepCookiesModeOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/CookiesKeepMode")));
	m_ui->thirdPartyCookiesPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/ThirdPartyCookiesPolicy")));
	m_ui->enableJavaScriptOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/EnableJavaScript")));
	m_ui->canChangeWindowGeometryOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")));
	m_ui->canShowStatusMessagesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanShowStatusMessages")));
	m_ui->canAccessClipboardOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanAccessClipboard")));
	m_ui->canDisableContextMenuOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanDisableContextMenu")));
	m_ui->canOpenWindowsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanOpenWindows")));
	m_ui->canCloseWindowsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/JavaScriptCanCloseWindows")));
	m_ui->enableFullScreenOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Browser/EnableFullScreen")));
	m_ui->sendReferrerOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/EnableReferrer")));
	m_ui->userAgentOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, QLatin1String("Network/UserAgent")));

	updateValues();

	QList<QCheckBox*> checkBoxes(findChildren<QCheckBox*>());

	for (int i = 0; i < checkBoxes.count(); ++i)
	{
		if (checkBoxes.at(i)->text().isEmpty())
		{
			connect(checkBoxes.at(i), SIGNAL(toggled(bool)), this, SLOT(updateValues(bool)));
		}
		else
		{
			connect(checkBoxes.at(i), SIGNAL(toggled(bool)), this, SLOT(valueChanged()));
		}
	}

	QList<QComboBox*> comboBoxes(findChildren<QComboBox*>());

	for (int i = 0; i < comboBoxes.count(); ++i)
	{
		connect(comboBoxes.at(i), SIGNAL(currentIndexChanged(int)), this, SLOT(valueChanged()));
	}

	connect(m_ui->userStyleSheetFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(valueChanged()));
	connect(ContentBlockingManager::getInstance(), SIGNAL(profileModified(QString)), this, SLOT(updateContentBlockingProfile(QString)));
	connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

WebsitePreferencesDialog::~WebsitePreferencesDialog()
{
	delete m_ui;
}

void WebsitePreferencesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void WebsitePreferencesDialog::buttonClicked(QAbstractButton *button)
{
	QStringList contentBlockingProfiles;
	QUrl url;
	url.setHost(m_ui->websiteLineEdit->text());

	switch (m_ui->buttonBox->buttonRole(button))
	{
		case QDialogButtonBox::AcceptRole:
			SettingsManager::setValue(QLatin1String("Content/DefaultEncoding"), (m_ui->encodingOverrideCheckBox->isChecked() ? (m_ui->encodingComboBox->currentIndex() ? m_ui->encodingComboBox->currentText() : QString()) : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Content/PopupsPolicy"), (m_ui->popupsPolicyOverrideCheckBox->isChecked() ? m_ui->popupsPolicyComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableImages"), (m_ui->enableImagesOverrideCheckBox->isChecked() ? m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableJava"), (m_ui->enableJavaOverrideCheckBox->isChecked() ? m_ui->enableJavaCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnablePlugins"), (m_ui->enablePluginsOverrideCheckBox->isChecked() ? m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Content/UserStyleSheet"), (m_ui->userStyleSheetOverrideCheckBox->isChecked() ? m_ui->userStyleSheetFilePathWidget->getPath() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/DoNotTrackPolicy"), (m_ui->doNotTrackOverrideCheckBox->isChecked() ? m_ui->doNotTrackComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("History/RememberBrowsing"), (m_ui->rememberBrowsingHistoryOverrideCheckBox->isChecked() ? m_ui->rememberBrowsingHistoryCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/CookiesPolicy"), (m_ui->enableCookiesOverrideCheckBox->isChecked() ? (m_ui->enableCookiesCheckBox->isChecked() ? m_ui->cookiesPolicyComboBox->currentData().toString() : QLatin1String("ignore")) : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/CookiesKeepMode"), (m_ui->keepCookiesModeOverrideCheckBox->isChecked() ? m_ui->keepCookiesModeComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/ThirdPartyCookiesPolicy"), (m_ui->thirdPartyCookiesPolicyOverrideCheckBox->isChecked() ? m_ui->thirdPartyCookiesPolicyComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableJavaScript"), (m_ui->enableJavaScriptOverrideCheckBox->isChecked() ? m_ui->enableJavaScriptCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"), (m_ui->canChangeWindowGeometryOverrideCheckBox->isChecked() ? m_ui->canChangeWindowGeometryCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), (m_ui->canShowStatusMessagesOverrideCheckBox->isChecked() ? m_ui->canShowStatusMessagesCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanAccessClipboard"), (m_ui->canAccessClipboardOverrideCheckBox->isChecked() ? m_ui->canAccessClipboardCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanDisableContextMenu"), (m_ui->canDisableContextMenuOverrideCheckBox->isChecked() ? m_ui->canDisableContextMenuCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanOpenWindows"), (m_ui->canOpenWindowsOverrideCheckBox->isChecked() ? m_ui->canOpenWindowsCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), (m_ui->canCloseWindowsOverrideCheckBox->isChecked() ? m_ui->canCloseWindowsComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Browser/EnableFullScreen"), (m_ui->enableFullScreenOverrideCheckBox->isChecked() ? m_ui->enableFullScreenComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/EnableReferrer"), (m_ui->sendReferrerOverrideCheckBox->isChecked() ? m_ui->sendReferrerCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(QLatin1String("Network/UserAgent"), (m_ui->userAgentOverrideCheckBox->isChecked() ? ((m_ui->userAgentComboBox->currentIndex() == 0) ? QString() : m_ui->userAgentComboBox->currentData(Qt::UserRole).toString()) : QVariant()), url);

			for (int i = 0; i < m_ui->contentBlockingProfilesViewWidget->getRowCount(); ++i)
			{
				if (m_ui->contentBlockingProfilesViewWidget->getIndex(i, 0).data(Qt::CheckStateRole).toBool())
				{
					contentBlockingProfiles.append(m_ui->contentBlockingProfilesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());
				}
			}

			SettingsManager::setValue(QLatin1String("Content/BlockingProfiles"), contentBlockingProfiles, url);

			accept();

			break;
		case QDialogButtonBox::ResetRole:
			SettingsManager::removeOverride(url);
			accept();

			break;
		case QDialogButtonBox::RejectRole:
			reject();

			break;
		default:
			break;
	}
}

void WebsitePreferencesDialog::updateContentBlockingProfile(const QString &profile)
{
	const ContentBlockingInformation information(ContentBlockingManager::getProfile(profile));

	if (information.name.isEmpty())
	{
		return;
	}

	for (int i = 0; i < m_ui->contentBlockingProfilesViewWidget->model()->rowCount(); ++i)
	{
		const QModelIndex index(m_ui->contentBlockingProfilesViewWidget->model()->index(i, 0));

		if (index.data(Qt::UserRole).toString() == profile)
		{
			const QSettings profilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);

			m_ui->contentBlockingProfilesViewWidget->model()->setData(index, information.title, Qt::DisplayRole);
			m_ui->contentBlockingProfilesViewWidget->model()->setData(index.sibling(i, 2), Utils::formatDateTime(profilesSettings.value(profile + QLatin1String("/lastUpdate")).toDateTime()), Qt::DisplayRole);

			break;
		}
	}
}

void WebsitePreferencesDialog::updateValues(bool checked)
{
	QUrl url;
	url.setHost(m_ui->websiteLineEdit->text());

	if (checked)
	{
		return;
	}

	m_updateOverride = false;

	m_ui->encodingComboBox->setCurrentIndex(qMax(0, m_ui->encodingComboBox->findText(SettingsManager::getValue(QLatin1String("Content/DefaultEncoding"), (m_ui->encodingOverrideCheckBox->isChecked() ? url : QUrl())).toString())));

	const int popupsPolicyIndex(m_ui->popupsPolicyComboBox->findData(SettingsManager::getValue(QLatin1String("Content/PopupsPolicy"), (m_ui->popupsPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->popupsPolicyComboBox->setCurrentIndex((popupsPolicyIndex < 0) ? 0 : popupsPolicyIndex);

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/EnableImages"), (m_ui->enableImagesOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);
	m_ui->enableJavaCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJava"), (m_ui->enableJavaOverrideCheckBox->isChecked() ? url : QUrl())).toBool());

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/EnablePlugins"), (m_ui->enablePluginsOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enablePluginsComboBox->setCurrentIndex((enablePluginsIndex < 0) ? 1 : enablePluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Content/UserStyleSheet"), (m_ui->userStyleSheetOverrideCheckBox->isChecked() ? url : QUrl())).toString());
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJavaScript"), (m_ui->enableJavaScriptOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canChangeWindowGeometryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"), (m_ui->canChangeWindowGeometryOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canShowStatusMessagesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), (m_ui->canShowStatusMessagesOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canAccessClipboardCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanAccessClipboard"), (m_ui->canAccessClipboardOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canDisableContextMenuCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanDisableContextMenu"), (m_ui->canDisableContextMenuOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canOpenWindowsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanOpenWindows"), (m_ui->canOpenWindowsCheckBox->isChecked() ? url : QUrl())).toBool());

	const int canCloseWindowsIndex(m_ui->canCloseWindowsComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), (m_ui->canCloseWindowsOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->canCloseWindowsComboBox->setCurrentIndex((canCloseWindowsIndex < 0) ? 0 : canCloseWindowsIndex);

	const int enableFullScreenIndex(m_ui->enableFullScreenComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/EnableFullScreen"), (m_ui->enableFullScreenOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enableFullScreenComboBox->setCurrentIndex((enableFullScreenIndex < 0) ? 0 : enableFullScreenIndex);

	const int doNotTrackPolicyIndex(m_ui->doNotTrackComboBox->findData(SettingsManager::getValue(QLatin1String("Network/DoNotTrackPolicy"), (m_ui->doNotTrackOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->doNotTrackComboBox->setCurrentIndex((doNotTrackPolicyIndex < 0) ? 2 : doNotTrackPolicyIndex);
	m_ui->rememberBrowsingHistoryCheckBox->setChecked(SettingsManager::getValue(QLatin1String("History/RememberBrowsing"), (m_ui->rememberBrowsingHistoryOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->enableCookiesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Network/CookiesPolicy"), (m_ui->enableCookiesOverrideCheckBox->isChecked() ? url : QUrl())).toString() != QLatin1String("ignore"));

	const int cookiesPolicyIndex(m_ui->cookiesPolicyComboBox->findData(SettingsManager::getValue(QLatin1String("Network/CookiesPolicy"), (m_ui->cookiesPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->cookiesPolicyComboBox->setCurrentIndex((cookiesPolicyIndex < 0) ? 0 : cookiesPolicyIndex);

	const int cookiesKeepModeIndex(m_ui->keepCookiesModeComboBox->findData(SettingsManager::getValue(QLatin1String("Network/CookiesKeepMode"), (m_ui->keepCookiesModeOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->keepCookiesModeComboBox->setCurrentIndex((cookiesKeepModeIndex < 0) ? 0 : cookiesKeepModeIndex);

	const int thirdPartyCookiesPolicyIndex(m_ui->thirdPartyCookiesPolicyComboBox->findData(SettingsManager::getValue(QLatin1String("Network/ThirdPartyCookiesPolicy"), (m_ui->thirdPartyCookiesPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->thirdPartyCookiesPolicyComboBox->setCurrentIndex((thirdPartyCookiesPolicyIndex < 0) ? 0 : thirdPartyCookiesPolicyIndex);
	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Network/EnableReferrer"), (m_ui->sendReferrerOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(SettingsManager::getValue(QLatin1String("Network/UserAgent"), (m_ui->userAgentOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	const QSettings contentBlockingProfilesSettings(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking.ini")), QSettings::IniFormat);
	const QStringList contentBlockingGlobalProfiles(SettingsManager::getValue(QLatin1String("Content/BlockingProfiles"), url).toStringList());
	const QVector<ContentBlockingInformation> contentBlockingProfiles(ContentBlockingManager::getProfiles());
	QStandardItemModel *contentBlockingProfilesModel(new QStandardItemModel(this));
	contentBlockingProfilesModel->setHorizontalHeaderLabels(QStringList({tr("Title"), tr("Update Interval"), tr("Last Update")}));

	for (int i = 0; i < contentBlockingProfiles.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(contentBlockingProfiles.at(i).title));
		items[0]->setFlags(Qt::ItemIsEnabled);
		items[0]->setData(contentBlockingProfiles.at(i).name, Qt::UserRole);
		items[0]->setCheckable(true);
		items[0]->setCheckState(contentBlockingGlobalProfiles.contains(contentBlockingProfiles.at(i).name) ? Qt::Checked : Qt::Unchecked);

		items.append(new QStandardItem(contentBlockingProfilesSettings.value(contentBlockingProfiles.at(i).name + QLatin1String("/updateInterval")).toString()));
		items[1]->setFlags(Qt::ItemIsEnabled);

		items.append(new QStandardItem(Utils::formatDateTime(contentBlockingProfilesSettings.value(contentBlockingProfiles.at(i).name + QLatin1String("/lastUpdate")).toDateTime())));
		items[2]->setFlags(Qt::ItemIsEnabled);

		contentBlockingProfilesModel->appendRow(items);
	}

	m_ui->contentBlockingProfilesViewWidget->setModel(contentBlockingProfilesModel);
	m_ui->contentBlockingProfilesViewWidget->setItemDelegate(new OptionDelegate(true, this));
	m_ui->contentBlockingProfilesViewWidget->setItemDelegateForColumn(1, new ContentBlockingIntervalDelegate(this));

	m_updateOverride = true;
}

void WebsitePreferencesDialog::valueChanged()
{
	QWidget *widget = qobject_cast<QWidget*>(sender());

	if (!widget || !m_updateOverride)
	{
		return;
	}

	QWidget *tab = qobject_cast<QWidget*>(widget->parent());

	if (!tab)
	{
		return;
	}

	QGridLayout *layout = qobject_cast<QGridLayout*>(tab->layout());

	if (!layout)
	{
		return;
	}

	const int index = layout->indexOf(widget);

	if (index < 0)
	{
		return;
	}

	int row = 0;
	int dummy = 0;

	layout->getItemPosition(index, &row, &dummy, &dummy, &dummy);

	QLayoutItem *item = layout->itemAtPosition(row, 0);

	if (!item)
	{
		return;
	}

	QCheckBox *overrideCheckBox = qobject_cast<QCheckBox*>(item->widget());

	if (!overrideCheckBox)
	{
		return;
	}

	overrideCheckBox->setChecked(true);
}

}
