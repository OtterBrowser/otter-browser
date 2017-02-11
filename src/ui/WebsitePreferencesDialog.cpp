/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "WebsitePreferencesDialog.h"
#include "CookiePropertiesDialog.h"
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

	m_ui->encodingComboBox->addItem(tr("Auto Detect"), QLatin1String("auto"));

	for (int i = 0; i < textCodecs.count(); ++i)
	{
		QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

		if (!codec)
		{
			continue;
		}

		m_ui->encodingComboBox->addItem(codec->name(), codec->name());
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

	m_ui->keepCookiesModeComboBox->addItem(tr("Expires"), QLatin1String("keepUntilExpires"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Current session is closed"), QLatin1String("keepUntilExit"));
	m_ui->keepCookiesModeComboBox->addItem(tr("Always ask"), QLatin1String("ask"));

	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Always"), QLatin1String("acceptAll"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Only existing"), QLatin1String("acceptExisting"));
	m_ui->thirdPartyCookiesPolicyComboBox->addItem(tr("Never"), QLatin1String("ignore"));

	QStandardItemModel *cookiesModel(new QStandardItemModel(this));
	cookiesModel->setHorizontalHeaderLabels(QStringList({tr("Domain"), tr("Name"), tr("Path"), tr("Value"), tr("Expiration date")}));

	m_ui->cookiesViewWidget->setModel(cookiesModel);

	for (int i = 0; i < cookies.count(); ++i)
	{
		addCookie(cookies.at(i));
	}

	m_ui->userAgentComboBox->setModel(new UserAgentsModel(QString(), false, this));
	m_ui->proxyComboBox->setModel(new ProxiesModel(QString(), false, this));

	m_ui->encodingOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Content_DefaultCharacterEncodingOption));
	m_ui->popupsPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Content_PopupsPolicyOption));
	m_ui->enableImagesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_EnableImagesOption));
	m_ui->enablePluginsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_EnablePluginsOption));
	m_ui->userStyleSheetOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Content_UserStyleSheetOption));
	m_ui->doNotTrackOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_DoNotTrackPolicyOption));
	m_ui->rememberBrowsingHistoryOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::History_RememberBrowsingOption));
	m_ui->enableCookiesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_CookiesPolicyOption));
	m_ui->cookiesPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_CookiesPolicyOption));
	m_ui->keepCookiesModeOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_CookiesKeepModeOption));
	m_ui->thirdPartyCookiesPolicyOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_ThirdPartyCookiesPolicyOption));
	m_ui->enableJavaScriptOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_EnableJavaScriptOption));
	m_ui->canChangeWindowGeometryOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption));
	m_ui->canShowStatusMessagesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption));
	m_ui->canAccessClipboardOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanAccessClipboardOption));
	m_ui->canReceiveRightClicksOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption));
	m_ui->canOpenWindowsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanOpenWindowsOption));
	m_ui->canCloseWindowsOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_ScriptsCanCloseWindowsOption));
	m_ui->enableFullScreenOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Permissions_EnableFullScreenOption));
	m_ui->sendReferrerOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_EnableReferrerOption));
	m_ui->userAgentOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::Network_UserAgentOption));
	m_ui->contentBlockingProfilesOverrideCheckBox->setChecked(SettingsManager::hasOverride(url, SettingsManager::ContentBlocking_ProfilesOption));

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

	connect(ContentBlockingManager::getInstance(), SIGNAL(profileModified(QString)), this, SLOT(updateContentBlockingProfile(QString)));
	connect(m_ui->userStyleSheetFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(valueChanged()));
	connect(m_ui->cookiesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateCookiesActions()));
	connect(m_ui->cookiesAddButton, SIGNAL(clicked(bool)), this, SLOT(addCookie()));
	connect(m_ui->cookiesDeleteButton, SIGNAL(clicked(bool)), this, SLOT(removeCookie()));
	connect(m_ui->cookiesPropertiesButton, SIGNAL(clicked(bool)), this, SLOT(cookieProperties()));
	connect(m_ui->contentBlockingProfilesViewWidget, SIGNAL(modified()), this, SLOT(valueChanged()));
	connect(m_ui->enableCustomRulesCheckBox, SIGNAL(toggled(bool)), this, SLOT(valueChanged()));
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
			SettingsManager::setValue(SettingsManager::Content_DefaultCharacterEncodingOption, (m_ui->encodingOverrideCheckBox->isChecked() ? m_ui->encodingComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Content_PopupsPolicyOption, (m_ui->popupsPolicyOverrideCheckBox->isChecked() ? m_ui->popupsPolicyComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_EnableImagesOption, (m_ui->enableImagesOverrideCheckBox->isChecked() ? m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_EnablePluginsOption, (m_ui->enablePluginsOverrideCheckBox->isChecked() ? m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Content_UserStyleSheetOption, (m_ui->userStyleSheetOverrideCheckBox->isChecked() ? m_ui->userStyleSheetFilePathWidget->getPath() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_DoNotTrackPolicyOption, (m_ui->doNotTrackOverrideCheckBox->isChecked() ? m_ui->doNotTrackComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::History_RememberBrowsingOption, (m_ui->rememberBrowsingHistoryOverrideCheckBox->isChecked() ? m_ui->rememberBrowsingHistoryCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_CookiesPolicyOption, (m_ui->enableCookiesOverrideCheckBox->isChecked() ? (m_ui->enableCookiesCheckBox->isChecked() ? m_ui->cookiesPolicyComboBox->currentData().toString() : QLatin1String("ignore")) : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_CookiesKeepModeOption, (m_ui->keepCookiesModeOverrideCheckBox->isChecked() ? m_ui->keepCookiesModeComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_ThirdPartyCookiesPolicyOption, (m_ui->thirdPartyCookiesPolicyOverrideCheckBox->isChecked() ? m_ui->thirdPartyCookiesPolicyComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_EnableJavaScriptOption, (m_ui->enableJavaScriptOverrideCheckBox->isChecked() ? m_ui->enableJavaScriptCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, (m_ui->canChangeWindowGeometryOverrideCheckBox->isChecked() ? m_ui->canChangeWindowGeometryCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, (m_ui->canShowStatusMessagesOverrideCheckBox->isChecked() ? m_ui->canShowStatusMessagesCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, (m_ui->canAccessClipboardOverrideCheckBox->isChecked() ? m_ui->canAccessClipboardCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption, (m_ui->canReceiveRightClicksOverrideCheckBox->isChecked() ? m_ui->canReceiveRightClicksCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, (m_ui->canOpenWindowsOverrideCheckBox->isChecked() ? m_ui->canOpenWindowsCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, (m_ui->canCloseWindowsOverrideCheckBox->isChecked() ? m_ui->canCloseWindowsComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Permissions_EnableFullScreenOption, (m_ui->enableFullScreenOverrideCheckBox->isChecked() ? m_ui->enableFullScreenComboBox->currentData().toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_EnableReferrerOption, (m_ui->sendReferrerOverrideCheckBox->isChecked() ? m_ui->sendReferrerCheckBox->isChecked() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_UserAgentOption, (m_ui->userAgentOverrideCheckBox->isChecked() ? m_ui->userAgentComboBox->currentData(UserAgentsModel::IdentifierRole).toString() : QVariant()), url);
			SettingsManager::setValue(SettingsManager::Network_ProxyOption, (m_ui->proxyOverrideCheckBox->isChecked() ? m_ui->proxyComboBox->currentData(ProxiesModel::IdentifierRole).toString() : QVariant()), url);

			if (m_ui->contentBlockingProfilesOverrideCheckBox->isChecked())
			{
				for (int i = 0; i < m_ui->contentBlockingProfilesViewWidget->getRowCount(); ++i)
				{
					const QModelIndex categoryIndex(m_ui->contentBlockingProfilesViewWidget->getIndex(i));

					for (int j = 0; j < m_ui->contentBlockingProfilesViewWidget->getRowCount(categoryIndex); ++j)
					{
						const QModelIndex entryIndex(m_ui->contentBlockingProfilesViewWidget->getIndex(j, 0, categoryIndex));

						if (entryIndex.data(Qt::CheckStateRole).toBool())
						{
							contentBlockingProfiles.append(entryIndex.data(Qt::UserRole).toString());
						}
					}
				}

				if (m_ui->enableCustomRulesCheckBox->isChecked())
				{
					contentBlockingProfiles.append(QLatin1String("custom"));
				}
			}

			SettingsManager::setValue(SettingsManager::ContentBlocking_ProfilesOption, (contentBlockingProfiles.isEmpty() ? QVariant() : contentBlockingProfiles), url);

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

void WebsitePreferencesDialog::addCookie()
{
	const QString domain(m_ui->websiteLineEdit->text());
	QNetworkCookie cookie;
	cookie.setDomain(domain.startsWith(QLatin1String("*.")) ? domain.mid(1) : domain);

	CookiePropertiesDialog dialog(cookie, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_cookiesToInsert.append(dialog.getModifiedCookie());

		addCookie(dialog.getModifiedCookie());
	}
}

void WebsitePreferencesDialog::addCookie(const QNetworkCookie &cookie)
{
	QList<QStandardItem*> items({new QStandardItem(cookie.domain()), new QStandardItem(QString(cookie.name())), new QStandardItem(cookie.path()), new QStandardItem(QString(cookie.value())), new QStandardItem(cookie.isSessionCookie() ? tr("this session only") : Utils::formatDateTime(cookie.expirationDate()))});
	items[0]->setData(cookie.toRawForm(), Qt::UserRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[3]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[4]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	m_ui->cookiesViewWidget->getSourceModel()->appendRow(items);
}

void WebsitePreferencesDialog::removeCookie()
{
	m_cookiesToDelete.append(QNetworkCookie::parseCookies(m_ui->cookiesViewWidget->getIndex(m_ui->cookiesViewWidget->getCurrentRow()).data(Qt::UserRole).toByteArray()));

	m_ui->cookiesViewWidget->removeRow();
}

void WebsitePreferencesDialog::cookieProperties()
{
	const QList<QNetworkCookie> cookies(QNetworkCookie::parseCookies(m_ui->cookiesViewWidget->getIndex(m_ui->cookiesViewWidget->getCurrentRow()).data(Qt::UserRole).toByteArray()));

	if (cookies.isEmpty())
	{
		return;
	}

	CookiePropertiesDialog dialog(cookies.first(), this);

	if (dialog.exec() == QDialog::Accepted && dialog.isModified())
	{
		removeCookie();
		addCookie(dialog.getModifiedCookie());

		m_cookiesToInsert.append(dialog.getModifiedCookie());
	}
}

void WebsitePreferencesDialog::updateCookiesActions()
{
	const QModelIndex index(m_ui->cookiesViewWidget->getIndex(m_ui->cookiesViewWidget->getCurrentRow()));

	m_ui->cookiesPropertiesButton->setEnabled(index.isValid());
	m_ui->cookiesDeleteButton->setEnabled(index.isValid());
}

void WebsitePreferencesDialog::updateContentBlockingProfile(const QString &name)
{
	ContentBlockingProfile *profile(ContentBlockingManager::getProfile(name));

	if (!profile)
	{
		return;
	}

	for (int i = 0; i < m_ui->contentBlockingProfilesViewWidget->getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(m_ui->contentBlockingProfilesViewWidget->getIndex(i));

		for (int j = 0; j < m_ui->contentBlockingProfilesViewWidget->getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(m_ui->contentBlockingProfilesViewWidget->getIndex(j, 0, categoryIndex));

			if (entryIndex.data(Qt::UserRole).toString() == name)
			{
				ContentBlockingProfile *profile(ContentBlockingManager::getProfile(name));

				m_ui->contentBlockingProfilesViewWidget->setData(entryIndex, profile->getTitle(), Qt::DisplayRole);
				m_ui->contentBlockingProfilesViewWidget->setData(entryIndex.sibling(j, 2), Utils::formatDateTime(profile->getLastUpdate()), Qt::DisplayRole);

				return;
			}
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

	m_ui->encodingComboBox->setCurrentIndex(m_ui->encodingComboBox->findData(SettingsManager::getValue(SettingsManager::Content_DefaultCharacterEncodingOption, (m_ui->encodingOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	const int popupsPolicyIndex(m_ui->popupsPolicyComboBox->findData(SettingsManager::getValue(SettingsManager::Content_PopupsPolicyOption, (m_ui->popupsPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->popupsPolicyComboBox->setCurrentIndex((popupsPolicyIndex < 0) ? 0 : popupsPolicyIndex);

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_EnableImagesOption, (m_ui->enableImagesOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_EnablePluginsOption, (m_ui->enablePluginsOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enablePluginsComboBox->setCurrentIndex((enablePluginsIndex < 0) ? 1 : enablePluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getValue(SettingsManager::Content_UserStyleSheetOption, (m_ui->userStyleSheetOverrideCheckBox->isChecked() ? url : QUrl())).toString());
	m_ui->userStyleSheetFilePathWidget->setFilters(QStringList(tr("Style sheets (*.css)")));
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_EnableJavaScriptOption, (m_ui->enableJavaScriptOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canChangeWindowGeometryCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, (m_ui->canChangeWindowGeometryOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canShowStatusMessagesCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, (m_ui->canShowStatusMessagesOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canAccessClipboardCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, (m_ui->canAccessClipboardOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canReceiveRightClicksCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption, (m_ui->canReceiveRightClicksOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->canOpenWindowsCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, (m_ui->canOpenWindowsCheckBox->isChecked() ? url : QUrl())).toBool());

	const int canCloseWindowsIndex(m_ui->canCloseWindowsComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, (m_ui->canCloseWindowsOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->canCloseWindowsComboBox->setCurrentIndex((canCloseWindowsIndex < 0) ? 0 : canCloseWindowsIndex);

	const int enableFullScreenIndex(m_ui->enableFullScreenComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_EnableFullScreenOption, (m_ui->enableFullScreenOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->enableFullScreenComboBox->setCurrentIndex((enableFullScreenIndex < 0) ? 0 : enableFullScreenIndex);

	const int doNotTrackPolicyIndex(m_ui->doNotTrackComboBox->findData(SettingsManager::getValue(SettingsManager::Network_DoNotTrackPolicyOption, (m_ui->doNotTrackOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->doNotTrackComboBox->setCurrentIndex((doNotTrackPolicyIndex < 0) ? 2 : doNotTrackPolicyIndex);
	m_ui->rememberBrowsingHistoryCheckBox->setChecked(SettingsManager::getValue(SettingsManager::History_RememberBrowsingOption, (m_ui->rememberBrowsingHistoryOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->enableCookiesCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Network_CookiesPolicyOption, (m_ui->enableCookiesOverrideCheckBox->isChecked() ? url : QUrl())).toString() != QLatin1String("ignore"));

	const int cookiesPolicyIndex(m_ui->cookiesPolicyComboBox->findData(SettingsManager::getValue(SettingsManager::Network_CookiesPolicyOption, (m_ui->cookiesPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->cookiesPolicyComboBox->setCurrentIndex((cookiesPolicyIndex < 0) ? 0 : cookiesPolicyIndex);

	const int cookiesKeepModeIndex(m_ui->keepCookiesModeComboBox->findData(SettingsManager::getValue(SettingsManager::Network_CookiesKeepModeOption, (m_ui->keepCookiesModeOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->keepCookiesModeComboBox->setCurrentIndex((cookiesKeepModeIndex < 0) ? 0 : cookiesKeepModeIndex);

	const int thirdPartyCookiesPolicyIndex(m_ui->thirdPartyCookiesPolicyComboBox->findData(SettingsManager::getValue(SettingsManager::Network_ThirdPartyCookiesPolicyOption, (m_ui->thirdPartyCookiesPolicyOverrideCheckBox->isChecked() ? url : QUrl())).toString()));

	m_ui->thirdPartyCookiesPolicyComboBox->setCurrentIndex((thirdPartyCookiesPolicyIndex < 0) ? 0 : thirdPartyCookiesPolicyIndex);
	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Network_EnableReferrerOption, (m_ui->sendReferrerOverrideCheckBox->isChecked() ? url : QUrl())).toBool());
	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->model()->match(m_ui->userAgentComboBox->model()->index(0, 0), UserAgentsModel::IdentifierRole, SettingsManager::getValue(SettingsManager::Network_UserAgentOption, (m_ui->userAgentOverrideCheckBox->isChecked() ? url : QUrl())).toString(), 1, Qt::MatchRecursive).value(0));
	m_ui->proxyComboBox->setCurrentIndex(m_ui->proxyComboBox->model()->match(m_ui->proxyComboBox->model()->index(0, 0), ProxiesModel::IdentifierRole, SettingsManager::getValue(SettingsManager::Network_ProxyOption, (m_ui->proxyOverrideCheckBox->isChecked() ? url : QUrl())).toString(), 1, Qt::MatchRecursive).value(0));

	const QStringList contentBlockingProfiles(SettingsManager::getValue(SettingsManager::ContentBlocking_ProfilesOption, url).toStringList());

	m_ui->contentBlockingProfilesViewWidget->setModel(ContentBlockingManager::createModel(this, contentBlockingProfiles));
	m_ui->contentBlockingProfilesViewWidget->setItemDelegateForColumn(1, new ContentBlockingIntervalDelegate(this));
	m_ui->contentBlockingProfilesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->contentBlockingProfilesViewWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->contentBlockingProfilesViewWidget->expandAll();

	m_ui->enableCustomRulesCheckBox->setChecked(contentBlockingProfiles.contains("custom"));

	m_updateOverride = true;
}

void WebsitePreferencesDialog::valueChanged()
{
	QWidget *widget(qobject_cast<QWidget*>(sender()));

	if (!widget || !m_updateOverride)
	{
		return;
	}

	if (widget == m_ui->contentBlockingProfilesViewWidget || widget == m_ui->enableCustomRulesCheckBox)
	{
		m_ui->contentBlockingProfilesOverrideCheckBox->setChecked(true);

		return;
	}

	QWidget *tab(qobject_cast<QWidget*>(widget->parent()));

	if (!tab)
	{
		return;
	}

	QGridLayout *layout(qobject_cast<QGridLayout*>(tab->layout()));

	if (!layout)
	{
		return;
	}

	const int index(layout->indexOf(widget));

	if (index < 0)
	{
		return;
	}

	int row(0);
	int dummy(0);

	layout->getItemPosition(index, &row, &dummy, &dummy, &dummy);

	QLayoutItem *item(layout->itemAtPosition(row, 0));

	if (!item)
	{
		return;
	}

	QCheckBox *overrideCheckBox(qobject_cast<QCheckBox*>(item->widget()));

	if (!overrideCheckBox)
	{
		return;
	}

	overrideCheckBox->setChecked(true);
}

QList<QNetworkCookie> WebsitePreferencesDialog::getCookiesToDelete() const
{
	return m_cookiesToDelete;
}

QList<QNetworkCookie> WebsitePreferencesDialog::getCookiesToInsert() const
{
	return m_cookiesToInsert;
}

}
