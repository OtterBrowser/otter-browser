/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "PreferencesAdvancedPageWidget.h"
#include "JavaScriptPreferencesDialog.h"
#include "KeyboardProfileDialog.h"
#include "MouseProfileDialog.h"
#include "../Style.h"
#include "../UserAgentsManagerDialog.h"
#include "../../core/ActionsManager.h"
#include "../../core/Application.h"
#include "../../core/GesturesManager.h"
#include "../../core/JsonSettings.h"
#include "../../core/NetworkManagerFactory.h"
#include "../../core/NotificationsManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/Settings.h"
#include "../../core/SettingsManager.h"
#include "../../core/ThemesManager.h"
#include "../../core/Utils.h"

#include "ui_PreferencesAdvancedPageWidget.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeDatabase>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtMultimedia/QSoundEffect>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslCipher>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>

namespace Otter
{

PreferencesAdvancedPageWidget::PreferencesAdvancedPageWidget(QWidget *parent) : QWidget(parent),
	m_userAgentsModified(false),
	m_ui(new Ui::PreferencesAdvancedPageWidget)
{
	m_ui->setupUi(this);

	QStandardItemModel *navigationModel(new QStandardItemModel(this));
	const QStringList navigationTitles({tr("Browsing"), tr("Notifications"), tr("Appearance"), tr("Content"), QString(), tr("Downloads"), tr("Programs"), QString(), tr("History"), tr("Network"), tr("Security"), tr("Updates"), QString(), tr("Keyboard"), tr("Mouse")});
	int navigationIndex(0);

	for (int i = 0; i < navigationTitles.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(navigationTitles.at(i)));

		if (navigationTitles.at(i).isEmpty())
		{
			item->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
			item->setEnabled(false);
		}
		else
		{
			item->setData(navigationIndex, Qt::UserRole);

			if (i == 6 || i == 8)
			{
				item->setEnabled(false);
			}

			++navigationIndex;
		}

		navigationModel->appendRow(item);
	}

	m_ui->advancedViewWidget->setModel(navigationModel);
	m_ui->advancedViewWidget->selectionModel()->select(navigationModel->index(0, 0), QItemSelectionModel::Select);

	updatePageSwitcher();

	m_ui->browsingSuggestBookmarksCheckBox->setChecked(SettingsManager::getValue(SettingsManager::AddressField_SuggestBookmarksOption).toBool());
	m_ui->browsingSuggestHistoryCheckBox->setChecked(SettingsManager::getValue(SettingsManager::AddressField_SuggestHistoryOption).toBool());
	m_ui->browsingSuggestLocalPathsCheckBox->setChecked(SettingsManager::getValue(SettingsManager::AddressField_SuggestLocalPathsOption).toBool());
	m_ui->browsingCategoriesCheckBox->setChecked(SettingsManager::getValue(SettingsManager::AddressField_ShowCompletionCategoriesOption).toBool());

	m_ui->browsingDisplayModeComboBox->addItem(tr("Compact"), QLatin1String("compact"));
	m_ui->browsingDisplayModeComboBox->addItem(tr("Columns"), QLatin1String("columns"));

	const int displayModeIndex(m_ui->browsingDisplayModeComboBox->findData(SettingsManager::getValue(SettingsManager::AddressField_CompletionDisplayModeOption).toString()));

	m_ui->browsingDisplayModeComboBox->setCurrentIndex((displayModeIndex < 0) ? 1 : displayModeIndex);

	m_ui->notificationsPlaySoundButton->setIcon(ThemesManager::getIcon(QLatin1String("media-playback-start")));
	m_ui->notificationsPlaySoundFilePathWidget->setFilters(QStringList(tr("WAV files (*.wav)")));

	QStandardItemModel *notificationsModel(new QStandardItemModel(this));
	notificationsModel->setHorizontalHeaderLabels(QStringList({tr("Name"), tr("Description")}));

	const QVector<EventDefinition> events(NotificationsManager::getEventDefinitions());

	for (int i = 0; i < events.count(); ++i)
	{
		QList<QStandardItem*> items({new QStandardItem(QCoreApplication::translate("notifications", events.at(i).title.toUtf8())), new QStandardItem(QCoreApplication::translate("notifications", events.at(i).description.toUtf8()))});
		items[0]->setData(events.at(i).identifier, Qt::UserRole);
		items[0]->setData(events.at(i).playSound, (Qt::UserRole + 1));
		items[0]->setData(events.at(i).showAlert, (Qt::UserRole + 2));
		items[0]->setData(events.at(i).showNotification, (Qt::UserRole + 3));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		notificationsModel->appendRow(items);
	}

	m_ui->notificationsItemView->setModel(notificationsModel);
	m_ui->preferNativeNotificationsCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Interface_UseNativeNotificationsOption).toBool());

	const QStringList widgetStyles(QStyleFactory::keys());

	m_ui->appearranceWidgetStyleComboBox->addItem(tr("System Style"));

	for (int i = 0; i < widgetStyles.count(); ++i)
	{
		m_ui->appearranceWidgetStyleComboBox->addItem(widgetStyles.at(i));
	}

	m_ui->appearranceWidgetStyleComboBox->setCurrentIndex(qMax(0, m_ui->appearranceWidgetStyleComboBox->findData(SettingsManager::getValue(SettingsManager::Interface_WidgetStyleOption).toString(), Qt::DisplayRole)));
	m_ui->appearranceStyleSheetFilePathWidget->setPath(SettingsManager::getValue(SettingsManager::Interface_StyleSheetOption).toString());
	m_ui->appearranceStyleSheetFilePathWidget->setFilters(QStringList(tr("Style sheets (*.css)")));
	m_ui->enableTrayIconCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Browser_EnableTrayIconOption).toBool());

	m_ui->enableImagesComboBox->addItem(tr("All images"), QLatin1String("enabled"));
	m_ui->enableImagesComboBox->addItem(tr("Cached images"), QLatin1String("onlyCached"));
	m_ui->enableImagesComboBox->addItem(tr("No images"), QLatin1String("disabled"));

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getValue(SettingsManager::Browser_EnableImagesOption).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Browser_EnableJavaScriptOption).toBool());
	m_ui->javaScriptOptionsButton->setEnabled(m_ui->enableJavaScriptCheckBox->isChecked());
	m_ui->enableJavaCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Browser_EnableJavaOption).toBool());
	m_ui->enablePluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->enablePluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->enablePluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getValue(SettingsManager::Browser_EnablePluginsOption).toString()));

	m_ui->enablePluginsComboBox->setCurrentIndex((enablePluginsIndex < 0) ? 1 : enablePluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getValue(SettingsManager::Content_UserStyleSheetOption).toString());
	m_ui->userStyleSheetFilePathWidget->setFilters(QStringList(tr("Style sheets (*.css)")));

	QStandardItemModel *downloadsModel(new QStandardItemModel(this));
	downloadsModel->setHorizontalHeaderLabels(QStringList(tr("Name")));

	Settings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QStringList handlers(handlersSettings.getGroups());

	for (int i = 0; i < handlers.count(); ++i)
	{
		if (handlers.at(i) != QLatin1String("*") && !handlers.at(i).contains(QLatin1Char('/')))
		{
			continue;
		}

		handlersSettings.beginGroup(handlers.at(i));

		QStandardItem *item(new QStandardItem(handlers.at(i)));
		item->setData(handlersSettings.getValue(QLatin1String("transferMode")), Qt::UserRole);
		item->setData(handlersSettings.getValue(QLatin1String("downloadsPath")), (Qt::UserRole + 1));
		item->setData(handlersSettings.getValue(QLatin1String("openCommand")), (Qt::UserRole + 2));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		handlersSettings.endGroup();

		downloadsModel->appendRow(item);
	}

	m_ui->downloadsItemView->setModel(downloadsModel);
	m_ui->downloadsItemView->sortByColumn(0, Qt::AscendingOrder);
	m_ui->downloadsFilePathWidget->setSelectFile(false);
	m_ui->downloadsApplicationComboBoxWidget->setAlwaysShowDefaultApplication(true);

	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Network_EnableReferrerOption).toBool());

	const QStringList userAgents(NetworkManagerFactory::getUserAgents());

	m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		if (userAgents.at(i).isEmpty())
		{
			m_ui->userAgentComboBox->insertSeparator(i);
		}
		else
		{
			const UserAgentDefinition userAgent(NetworkManagerFactory::getUserAgent(userAgents.at(i)));

			m_ui->userAgentComboBox->addItem(userAgent.getTitle(), userAgents.at(i));
			m_ui->userAgentComboBox->setItemData((i + 1), userAgent.value, (Qt::UserRole + 1));
		}
	}

	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(SettingsManager::getValue(SettingsManager::Network_UserAgentOption).toString()));

	QStandardItemModel *proxyExceptionsModel(new QStandardItemModel(this));
	const QStringList currentProxyExceptions(SettingsManager::getValue(SettingsManager::Proxy_ExceptionsOption).toStringList());

	for (int i = 0; i < currentProxyExceptions.count(); ++i)
	{
		proxyExceptionsModel->appendRow(new QStandardItem(currentProxyExceptions.at(i)));
	}

	m_ui->proxyExceptionsItemView->setModel(proxyExceptionsModel);

	m_ui->proxyModeComboBox->addItem(tr("No proxy"), QLatin1String("noproxy"));
	m_ui->proxyModeComboBox->addItem(tr("System configuration"), QLatin1String("system"));
	m_ui->proxyModeComboBox->addItem(tr("Manual configuration"), QLatin1String("manual"));
	m_ui->proxyModeComboBox->addItem(tr("Automatic configuration (PAC)"), QLatin1String("automatic"));

	const int proxyIndex(m_ui->proxyModeComboBox->findData(SettingsManager::getValue(SettingsManager::Network_ProxyModeOption).toString()));

	m_ui->proxyModeComboBox->setCurrentIndex((proxyIndex < 0) ? 1 : proxyIndex);

	if (proxyIndex == 2 || proxyIndex == 3)
	{
		proxyModeChanged(proxyIndex);
	}

	m_ui->allProxyCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseCommonOption).toBool());
	m_ui->httpProxyCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseHttpOption).toBool());
	m_ui->httpsProxyCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseHttpsOption).toBool());
	m_ui->ftpProxyCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseFtpOption).toBool());
	m_ui->socksProxyCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseSocksOption).toBool());
	m_ui->allProxyServersLineEdit->setText(SettingsManager::getValue(SettingsManager::Proxy_CommonServersOption).toString());
	m_ui->httpProxyServersLineEdit->setText(SettingsManager::getValue(SettingsManager::Proxy_HttpServersOption).toString());
	m_ui->httpsProxyServersLineEdit->setText(SettingsManager::getValue(SettingsManager::Proxy_HttpsServersOption).toString());
	m_ui->ftpProxyServersLineEdit->setText(SettingsManager::getValue(SettingsManager::Proxy_FtpServersOption).toString());
	m_ui->socksProxyServersLineEdit->setText(SettingsManager::getValue(SettingsManager::Proxy_SocksServersOption).toString());
	m_ui->allProxyPortSpinBox->setValue(SettingsManager::getValue(SettingsManager::Proxy_CommonPortOption).toInt());
	m_ui->httpProxyPortSpinBox->setValue(SettingsManager::getValue(SettingsManager::Proxy_HttpPortOption).toInt());
	m_ui->httpsProxyPortSpinBox->setValue(SettingsManager::getValue(SettingsManager::Proxy_HttpsPortOption).toInt());
	m_ui->ftpProxyPortSpinBox->setValue(SettingsManager::getValue(SettingsManager::Proxy_FtpPortOption).toInt());
	m_ui->socksProxyPortSpinBox->setValue(SettingsManager::getValue(SettingsManager::Proxy_SocksPortOption).toInt());
	m_ui->automaticProxyConfigurationFilePathWidget->setPath(SettingsManager::getValue(SettingsManager::Proxy_AutomaticConfigurationPathOption).toString());
	m_ui->automaticProxyConfigurationFilePathWidget->setFilters(QStringList(tr("Proxy configuration files (*.pac)")));
	m_ui->proxySystemAuthentication->setChecked(SettingsManager::getValue(SettingsManager::Proxy_UseSystemAuthenticationOption).toBool());

	m_ui->ciphersAddButton->setMenu(new QMenu(m_ui->ciphersAddButton));

	if (QSslSocket::supportsSsl())
	{
		QStandardItemModel *ciphersModel(new QStandardItemModel(this));
		const bool useDefaultCiphers(SettingsManager::getValue(SettingsManager::Security_CiphersOption).toString() == QLatin1String("default"));
		const QStringList selectedCiphers(useDefaultCiphers ? QStringList() : SettingsManager::getValue(SettingsManager::Security_CiphersOption).toStringList());

		for (int i = 0; i < selectedCiphers.count(); ++i)
		{
			const QSslCipher cipher(selectedCiphers.at(i));

			if (!cipher.isNull())
			{
				QStandardItem *item(new QStandardItem(cipher.name()));
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

				ciphersModel->appendRow(item);
			}
		}

		const QList<QSslCipher> defaultCiphers(NetworkManagerFactory::getDefaultCiphers());
		const QList<QSslCipher> supportedCiphers(QSslSocket::supportedCiphers());

		for (int i = 0; i < supportedCiphers.count(); ++i)
		{
			if (useDefaultCiphers && defaultCiphers.contains(supportedCiphers.at(i)))
			{
				QStandardItem *item(new QStandardItem(supportedCiphers.at(i).name()));
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

				ciphersModel->appendRow(item);
			}
			else if (!selectedCiphers.contains(supportedCiphers.at(i).name()))
			{
				m_ui->ciphersAddButton->menu()->addAction(supportedCiphers.at(i).name());
			}
		}

		m_ui->ciphersViewWidget->setModel(ciphersModel);
		m_ui->ciphersAddButton->setEnabled(m_ui->ciphersAddButton->menu()->actions().count() > 0);
	}
	else
	{
		m_ui->ciphersViewWidget->setEnabled(false);
	}

	m_ui->ciphersMoveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-down")));
	m_ui->ciphersMoveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *updateChannelsModel(new QStandardItemModel(this));
	const QStringList availableUpdateChannels(SettingsManager::getValue(SettingsManager::Updates_ActiveChannelsOption).toStringList());

	QMap<QString, QString> defaultChannels;
	defaultChannels[QLatin1String("release")] = tr("Stable version");
	defaultChannels[QLatin1String("beta")] = tr("Beta version");
	defaultChannels[QLatin1String("weekly")] = tr("Weekly development version");

	QMap<QString, QString>::iterator iterator;

	for (iterator = defaultChannels.begin(); iterator != defaultChannels.end(); ++iterator)
	{
		QStandardItem *item(new QStandardItem(iterator.value()));
		item->setData(iterator.key(), Qt::UserRole);
		item->setCheckable(true);

		if (availableUpdateChannels.contains(iterator.key()))
		{
			item->setCheckState(Qt::Checked);
		}

		updateChannelsModel->appendRow(item);
	}

	m_ui->updateChannelsItemView->setModel(updateChannelsModel);

	m_ui->autoInstallCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Updates_AutomaticInstallOption).toBool());
	m_ui->intervalSpinBox->setValue(SettingsManager::getValue(SettingsManager::Updates_CheckIntervalOption).toInt());

	updateUpdateChannelsActions();

	m_ui->keyboardMoveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-down")));
	m_ui->keyboardMoveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *keyboardProfilesModel(new QStandardItemModel(this));
	const QStringList keyboardProfiles(SettingsManager::getValue(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList());

	for (int i = 0; i < keyboardProfiles.count(); ++i)
	{
		const KeyboardProfile profile(loadKeyboardProfile(keyboardProfiles.at(i), true));

		if (profile.identifier.isEmpty())
		{
			continue;
		}

		m_keyboardProfiles[keyboardProfiles.at(i)] = profile;

		QStandardItem *item(new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title));
		item->setToolTip(profile.description);
		item->setData(profile.identifier, Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		keyboardProfilesModel->appendRow(item);
	}

	m_ui->keyboardViewWidget->setModel(keyboardProfilesModel);

	QMenu *addKeyboardProfileMenu(new QMenu(m_ui->keyboardAddButton));
	addKeyboardProfileMenu->addAction(tr("New…"));
	addKeyboardProfileMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->keyboardAddButton));

	m_ui->keyboardAddButton->setMenu(addKeyboardProfileMenu);
	m_ui->keyboardEnableSingleKeyShortcutsCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool());

	updateReaddKeyboardProfileMenu();

	m_ui->mouseMoveDownButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-down")));
	m_ui->mouseMoveUpButton->setIcon(ThemesManager::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *mouseProfilesModel(new QStandardItemModel(this));
	const QStringList mouseProfiles(SettingsManager::getValue(SettingsManager::Browser_MouseProfilesOrderOption).toStringList());

	for (int i = 0; i < mouseProfiles.count(); ++i)
	{
		const MouseProfile profile(loadMouseProfile(mouseProfiles.at(i), true));

		if (profile.identifier.isEmpty())
		{
			continue;
		}

		m_mouseProfiles[mouseProfiles.at(i)] = profile;

		QStandardItem *item(new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title));
		item->setToolTip(profile.description);
		item->setData(profile.identifier, Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		mouseProfilesModel->appendRow(item);
	}

	m_ui->mouseViewWidget->setModel(mouseProfilesModel);

	QMenu *addMouseProfileMenu(new QMenu(m_ui->mouseAddButton));
	addMouseProfileMenu->addAction(tr("New…"));
	addMouseProfileMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->mouseAddButton));

	m_ui->mouseAddButton->setMenu(addMouseProfileMenu);
	m_ui->mouseEnableGesturesCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Browser_EnableMouseGesturesOption).toBool());

	updateReaddMouseProfileMenu();

	connect(m_ui->advancedViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(changeCurrentPage()));
	connect(m_ui->notificationsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateNotificationsActions()));
	connect(m_ui->notificationsPlaySoundButton, SIGNAL(clicked()), this, SLOT(playNotificationSound()));
	connect(m_ui->enableJavaScriptCheckBox, SIGNAL(toggled(bool)), m_ui->javaScriptOptionsButton, SLOT(setEnabled(bool)));
	connect(m_ui->javaScriptOptionsButton, SIGNAL(clicked()), this, SLOT(updateJavaScriptOptions()));
	connect(m_ui->downloadsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateDownloadsActions()));
	connect(m_ui->downloadsAddMimeTypeButton, SIGNAL(clicked(bool)), this, SLOT(addDownloadsMimeType()));
	connect(m_ui->downloadsRemoveMimeTypeButton, SIGNAL(clicked(bool)), this, SLOT(removeDownloadsMimeType()));
	connect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsMode()));
	connect(m_ui->userAgentButton, SIGNAL(clicked()), this, SLOT(manageUserAgents()));
	connect(m_ui->proxyModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyModeChanged(int)));
	connect(m_ui->proxyExceptionsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateProxyExceptionsActions()));
	connect(m_ui->addProxyExceptionButton, SIGNAL(clicked()), this, SLOT(addProxyException()));
	connect(m_ui->editProxyExceptionButton, SIGNAL(clicked()), this, SLOT(editProxyException()));
	connect(m_ui->removeProxyExceptionButton, SIGNAL(clicked()), this, SLOT(removeProxyException()));
	connect(m_ui->ciphersViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->ciphersMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->ciphersViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->ciphersMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->ciphersViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateCiphersActions()));
	connect(m_ui->ciphersViewWidget, SIGNAL(modified()), this, SIGNAL(settingsModified()));
	connect(m_ui->ciphersAddButton->menu(), SIGNAL(triggered(QAction*)), this, SLOT(addCipher(QAction*)));
	connect(m_ui->ciphersRemoveButton, SIGNAL(clicked()), this, SLOT(removeCipher()));
	connect(m_ui->ciphersMoveDownButton, SIGNAL(clicked()), m_ui->ciphersViewWidget, SLOT(moveDownRow()));
	connect(m_ui->ciphersMoveUpButton, SIGNAL(clicked()), m_ui->ciphersViewWidget, SLOT(moveUpRow()));
	connect(m_ui->updateChannelsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateUpdateChannelsActions()));
	connect(m_ui->keyboardViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->keyboardMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->keyboardViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->keyboardMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->keyboardViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateKeyboardProfileActions()));
	connect(m_ui->keyboardViewWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editKeyboardProfile()));
	connect(m_ui->keyboardViewWidget, SIGNAL(modified()), this, SIGNAL(settingsModified()));
	connect(m_ui->keyboardAddButton->menu()->actions().at(0), SIGNAL(triggered()), this, SLOT(addKeyboardProfile()));
	connect(m_ui->keyboardAddButton->menu()->actions().at(1)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(readdKeyboardProfile(QAction*)));
	connect(m_ui->keyboardEditButton, SIGNAL(clicked()), this, SLOT(editKeyboardProfile()));
	connect(m_ui->keyboardCloneButton, SIGNAL(clicked()), this, SLOT(cloneKeyboardProfile()));
	connect(m_ui->keyboardRemoveButton, SIGNAL(clicked()), this, SLOT(removeKeyboardProfile()));
	connect(m_ui->keyboardMoveDownButton, SIGNAL(clicked()), m_ui->keyboardViewWidget, SLOT(moveDownRow()));
	connect(m_ui->keyboardMoveUpButton, SIGNAL(clicked()), m_ui->keyboardViewWidget, SLOT(moveUpRow()));
	connect(m_ui->mouseViewWidget, SIGNAL(canMoveDownChanged(bool)), m_ui->mouseMoveDownButton, SLOT(setEnabled(bool)));
	connect(m_ui->mouseViewWidget, SIGNAL(canMoveUpChanged(bool)), m_ui->mouseMoveUpButton, SLOT(setEnabled(bool)));
	connect(m_ui->mouseViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateMouseProfileActions()));
	connect(m_ui->mouseViewWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editMouseProfile()));
	connect(m_ui->mouseViewWidget, SIGNAL(modified()), this, SIGNAL(settingsModified()));
	connect(m_ui->mouseAddButton->menu()->actions().at(0), SIGNAL(triggered()), this, SLOT(addMouseProfile()));
	connect(m_ui->mouseAddButton->menu()->actions().at(1)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(readdMouseProfile(QAction*)));
	connect(m_ui->mouseEditButton, SIGNAL(clicked()), this, SLOT(editMouseProfile()));
	connect(m_ui->mouseCloneButton, SIGNAL(clicked()), this, SLOT(cloneMouseProfile()));
	connect(m_ui->mouseRemoveButton, SIGNAL(clicked()), this, SLOT(removeMouseProfile()));
	connect(m_ui->mouseMoveDownButton, SIGNAL(clicked()), m_ui->mouseViewWidget, SLOT(moveDownRow()));
	connect(m_ui->mouseMoveUpButton, SIGNAL(clicked()), m_ui->mouseViewWidget, SLOT(moveUpRow()));
}

PreferencesAdvancedPageWidget::~PreferencesAdvancedPageWidget()
{
	delete m_ui;
}

void PreferencesAdvancedPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		updatePageSwitcher();
	}
}

void PreferencesAdvancedPageWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	updatePageSwitcher();
}

void PreferencesAdvancedPageWidget::playNotificationSound()
{
	QSoundEffect *effect(new QSoundEffect(this));
	effect->setSource(QUrl::fromLocalFile(m_ui->notificationsPlaySoundFilePathWidget->getPath()));
	effect->setLoopCount(1);
	effect->setVolume(0.5);

	if (effect->status() == QSoundEffect::Error)
	{
		effect->deleteLater();
	}
	else
	{
		QTimer::singleShot(10000, effect, SLOT(deleteLater()));

		effect->play();
	}
}

void PreferencesAdvancedPageWidget::updateNotificationsActions()
{
	disconnect(m_ui->notificationsPlaySoundFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(updateNotificationsOptions()));
	disconnect(m_ui->notificationsShowAlertCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
	disconnect(m_ui->notificationsShowNotificationCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));

	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));
	const QString path(index.data(Qt::UserRole + 1).toString());

	m_ui->notificationOptionsWidget->setEnabled(index.isValid());
	m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
	m_ui->notificationsPlaySoundFilePathWidget->setPath(path);
	m_ui->notificationsShowAlertCheckBox->setChecked(index.data(Qt::UserRole + 2).toBool());
	m_ui->notificationsShowNotificationCheckBox->setChecked(index.data(Qt::UserRole + 3).toBool());

	connect(m_ui->notificationsPlaySoundFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(updateNotificationsOptions()));
	connect(m_ui->notificationsShowAlertCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
	connect(m_ui->notificationsShowNotificationCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
}

void PreferencesAdvancedPageWidget::updateNotificationsOptions()
{
	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));

	if (index.isValid())
	{
		disconnect(m_ui->notificationsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateNotificationsActions()));

		const QString path(m_ui->notificationsPlaySoundFilePathWidget->getPath());

		m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
		m_ui->notificationsItemView->setData(index, path, (Qt::UserRole + 1));
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowAlertCheckBox->isChecked(), (Qt::UserRole + 2));
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowNotificationCheckBox->isChecked(), (Qt::UserRole + 3));

		emit settingsModified();

		connect(m_ui->notificationsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateNotificationsActions()));
	}
}

void PreferencesAdvancedPageWidget::addDownloadsMimeType()
{
	const QString mimeType(QInputDialog::getText(this, tr("MIME Type Name"), tr("Select name of MIME Type:")));

	if (!mimeType.isEmpty())
	{
		if (QRegularExpression(QLatin1String("^[a-zA-Z\\-]+/[a-zA-Z0-9\\.\\+\\-_]+$")).match(mimeType).hasMatch())
		{
			QStandardItem *item(new QStandardItem(mimeType));
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

			m_ui->downloadsItemView->insertRow(item);
			m_ui->downloadsItemView->sortByColumn(0, Qt::AscendingOrder);
		}
		else
		{
			QMessageBox::critical(this, tr("Error"), tr("Invalid MIME Type name."));
		}
	}
}

void PreferencesAdvancedPageWidget::removeDownloadsMimeType()
{
	const QModelIndex index(m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow()));

	if (index.isValid() && index.data(Qt::DisplayRole).toString() != QLatin1String("*"))
	{
		m_ui->downloadsItemView->removeRow();
	}
}

void PreferencesAdvancedPageWidget::updateDownloadsActions()
{
	disconnect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsSaveDirectlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsPassUrlCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsApplicationComboBoxWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDownloadsOptions()));

	const QModelIndex index(m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow()));
	const QString mode(index.data(Qt::UserRole).toString());

	if (mode == QLatin1String("save") || mode == QLatin1String("saveAs"))
	{
		m_ui->downloadsSaveButton->setChecked(true);
	}
	else if (mode == QLatin1String("open"))
	{
		m_ui->downloadsOpenButton->setChecked(true);
	}
	else
	{
		m_ui->downloadsAskButton->setChecked(true);
	}

	m_ui->downloadsRemoveMimeTypeButton->setEnabled(index.isValid() && index.data(Qt::DisplayRole).toString() != QLatin1String("*"));
	m_ui->downloadsOptionsWidget->setEnabled(index.isValid());
	m_ui->downloadsSaveDirectlyCheckBox->setChecked(mode == QLatin1String("save"));
	m_ui->downloadsFilePathWidget->setPath(index.data(Qt::UserRole + 1).toString());
	m_ui->downloadsApplicationComboBoxWidget->setMimeType(QMimeDatabase().mimeTypeForName(index.data(Qt::DisplayRole).toString()));
	m_ui->downloadsApplicationComboBoxWidget->setCurrentCommand(index.data(Qt::UserRole + 2).toString());

	connect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsSaveDirectlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsFilePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsPassUrlCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsApplicationComboBoxWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDownloadsOptions()));
}

void PreferencesAdvancedPageWidget::updateDownloadsOptions()
{
	const QModelIndex index(m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow()));

	disconnect(m_ui->downloadsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateDownloadsActions()));
	disconnect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));

	if (index.isValid())
	{
		QString mode;

		if (m_ui->downloadsSaveButton->isChecked())
		{
			mode = (m_ui->downloadsSaveDirectlyCheckBox->isChecked() ? QLatin1String("save") : QLatin1String("saveAs"));
		}
		else if (m_ui->downloadsOpenButton->isChecked())
		{
			mode = QLatin1String("open");
		}
		else
		{
			mode = QLatin1String("ask");
		}

		m_ui->downloadsItemView->setData(index, mode, Qt::UserRole);
		m_ui->downloadsItemView->setData(index, ((mode == QLatin1String("save") || mode == QLatin1String("saveAs")) ? m_ui->downloadsFilePathWidget->getPath() : QString()), (Qt::UserRole + 1));
		m_ui->downloadsItemView->setData(index, ((mode == QLatin1String("open")) ? m_ui->downloadsApplicationComboBoxWidget->getCommand() : QString()), (Qt::UserRole + 2));
	}

	emit settingsModified();

	connect(m_ui->downloadsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateDownloadsActions()));
	connect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
}

void PreferencesAdvancedPageWidget::updateDownloadsMode()
{
	m_ui->downloadsOpenOptionsWidget->setEnabled(m_ui->downloadsOpenButton->isChecked());
	m_ui->downloadsSaveOptionsWidget->setEnabled(m_ui->downloadsSaveButton->isChecked());
}

void PreferencesAdvancedPageWidget::manageUserAgents()
{
	QList<UserAgentDefinition> userAgents;

	for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString();
		userAgent.title = m_ui->userAgentComboBox->itemText(i);
		userAgent.value = m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString();

		userAgents.append(userAgent);
	}

	const QString selectedUserAgent(m_ui->userAgentComboBox->currentData().toString());
	UserAgentsManagerDialog dialog(userAgents, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_userAgentsModified = true;

		m_ui->userAgentComboBox->clear();

		userAgents = dialog.getUserAgents();

		m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

		for (int i = 0; i < userAgents.count(); ++i)
		{
			m_ui->userAgentComboBox->addItem(userAgents.at(i).getTitle(), userAgents.at(i).identifier);
			m_ui->userAgentComboBox->setItemData((i + 1), userAgents.at(i).value, (Qt::UserRole + 1));
		}

		m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(selectedUserAgent));
	}
}

void PreferencesAdvancedPageWidget::proxyModeChanged(int index)
{
	if (index == 2)
	{
		m_ui->manualProxyConfigurationWidget->setEnabled(true);
		m_ui->allProxyCheckBox->setEnabled(true);
		m_ui->allProxyServersLineEdit->setEnabled(true);
		m_ui->allProxyPortSpinBox->setEnabled(true);
		m_ui->proxyExceptionsWidget->setEnabled(true);

		if (!m_ui->allProxyCheckBox->isChecked())
		{
			m_ui->httpProxyCheckBox->setEnabled(true);
			m_ui->httpsProxyCheckBox->setEnabled(true);
			m_ui->ftpProxyCheckBox->setEnabled(true);
			m_ui->socksProxyCheckBox->setEnabled(true);
			m_ui->httpProxyServersLineEdit->setEnabled(true);
			m_ui->httpProxyPortSpinBox->setEnabled(true);
			m_ui->httpsProxyServersLineEdit->setEnabled(true);
			m_ui->httpsProxyPortSpinBox->setEnabled(true);
			m_ui->ftpProxyServersLineEdit->setEnabled(true);
			m_ui->ftpProxyPortSpinBox->setEnabled(true);
			m_ui->socksProxyServersLineEdit->setEnabled(true);
			m_ui->socksProxyPortSpinBox->setEnabled(true);
		}

		updateProxyExceptionsActions();
	}
	else
	{
		m_ui->manualProxyConfigurationWidget->setDisabled(true);
		m_ui->allProxyCheckBox->setChecked(false);
		m_ui->httpProxyCheckBox->setChecked(false);
		m_ui->httpsProxyCheckBox->setChecked(false);
		m_ui->ftpProxyCheckBox->setChecked(false);
		m_ui->socksProxyCheckBox->setChecked(false);
		m_ui->proxyExceptionsWidget->setEnabled(false);
	}

	if (index == 3)
	{
		m_ui->automaticProxyConfigurationWidget->setEnabled(true);
		m_ui->automaticProxyConfigurationFilePathWidget->setEnabled(true);
	}
	else
	{
		m_ui->automaticProxyConfigurationWidget->setEnabled(false);
		m_ui->automaticProxyConfigurationFilePathWidget->setEnabled(false);
	}

	if (index == 0)
	{
		m_ui->proxySystemAuthentication->setEnabled(false);
		m_ui->proxySystemAuthentication->setChecked(false);
	}
	else
	{
		m_ui->proxySystemAuthentication->setEnabled(true);
	}
}

void PreferencesAdvancedPageWidget::addProxyException()
{
	m_ui->proxyExceptionsItemView->insertRow();

	editProxyException();
}

void PreferencesAdvancedPageWidget::editProxyException()
{
	m_ui->proxyExceptionsItemView->edit(m_ui->proxyExceptionsItemView->getIndex(m_ui->proxyExceptionsItemView->getCurrentRow()));

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::removeProxyException()
{
	m_ui->proxyExceptionsItemView->removeRow();
	m_ui->proxyExceptionsItemView->setFocus();

	updateProxyExceptionsActions();

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::updateProxyExceptionsActions()
{
	const bool isEditable(m_ui->proxyExceptionsItemView->getCurrentRow() >= 0);

	m_ui->editProxyExceptionButton->setEnabled(isEditable);
	m_ui->removeProxyExceptionButton->setEnabled(isEditable);
}

void PreferencesAdvancedPageWidget::addCipher(QAction *action)
{
	if (!action)
	{
		return;
	}

	QStandardItem *item(new QStandardItem(action->text()));
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->ciphersViewWidget->insertRow(item);
	m_ui->ciphersAddButton->menu()->removeAction(action);
	m_ui->ciphersAddButton->setEnabled(m_ui->ciphersAddButton->menu()->actions().count() > 0);
}

void PreferencesAdvancedPageWidget::removeCipher()
{
	const int currentRow(m_ui->ciphersViewWidget->getCurrentRow());

	if (currentRow >= 0)
	{
		m_ui->ciphersAddButton->menu()->addAction(m_ui->ciphersViewWidget->getIndex(currentRow, 0).data(Qt::DisplayRole).toString());
		m_ui->ciphersAddButton->setEnabled(true);

		m_ui->ciphersViewWidget->removeRow();
	}
}

void PreferencesAdvancedPageWidget::updateCiphersActions()
{
	const int currentRow(m_ui->ciphersViewWidget->getCurrentRow());

	m_ui->ciphersRemoveButton->setEnabled(currentRow >= 0 && currentRow < m_ui->ciphersViewWidget->getRowCount());
}

void PreferencesAdvancedPageWidget::updateUpdateChannelsActions()
{
	m_ui->intervalSpinBox->setEnabled(!getSelectedUpdateChannels().isEmpty());
	m_ui->autoInstallCheckBox->setEnabled(!getSelectedUpdateChannels().isEmpty());

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::addKeyboardProfile()
{
	const QString identifier(createProfileIdentifier(m_ui->keyboardViewWidget));

	if (identifier.isEmpty())
	{
		return;
	}

	KeyboardProfile profile;
	profile.title = tr("(Untitled)");

	m_keyboardProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(tr("(Untitled)")));
	item->setData(identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->keyboardViewWidget->insertRow(item);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::readdKeyboardProfile(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier(action->data().toString());
	const KeyboardProfile profile(loadKeyboardProfile(identifier, true));

	if (profile.identifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title));
	item->setToolTip(profile.description);
	item->setData(profile.identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->keyboardViewWidget->insertRow(item);

	updateReaddKeyboardProfileMenu();

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::editKeyboardProfile()
{
	const QModelIndex index(m_ui->keyboardViewWidget->currentIndex());
	const QString identifier(index.data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		if (!index.isValid())
		{
			addKeyboardProfile();
		}

		return;
	}

	KeyboardProfileDialog dialog(identifier, m_keyboardProfiles, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	m_keyboardProfiles[identifier] = dialog.getProfile();

	m_ui->keyboardViewWidget->setData(index, (m_keyboardProfiles[identifier].title.isEmpty() ? tr("(Untitled)") : m_keyboardProfiles[identifier].title), Qt::DisplayRole);
	m_ui->keyboardViewWidget->setData(index, m_keyboardProfiles[identifier].description, Qt::ToolTipRole);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::cloneKeyboardProfile()
{
	const QString identifier(m_ui->keyboardViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier(createProfileIdentifier(m_ui->keyboardViewWidget, identifier));

	if (newIdentifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[newIdentifier] = m_keyboardProfiles[identifier];
	m_keyboardProfiles[newIdentifier].identifier = newIdentifier;
	m_keyboardProfiles[newIdentifier].isModified = true;

	QStandardItem *item(new QStandardItem(m_keyboardProfiles[newIdentifier].title.isEmpty() ? tr("(Untitled)") : m_keyboardProfiles[newIdentifier].title));
	item->setToolTip(m_keyboardProfiles[newIdentifier].description);
	item->setData(newIdentifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->keyboardViewWidget->insertRow(item);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::removeKeyboardProfile()
{
	const QString identifier(m_ui->keyboardViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this profile?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".ini")));

	if (QFile::exists(path))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete profile permanently")));
	}

	if (messageBox.exec() == QMessageBox::Yes)
	{
		if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
		{
			m_filesToRemove.append(path);
		}

		m_keyboardProfiles.remove(identifier);

		m_ui->keyboardViewWidget->removeRow();

		updateReaddKeyboardProfileMenu();

		emit settingsModified();
	}
}

void PreferencesAdvancedPageWidget::updateKeyboardProfileActions()
{
	const int currentRow(m_ui->keyboardViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->keyboardViewWidget->getRowCount());

	m_ui->keyboardEditButton->setEnabled(isSelected);
	m_ui->keyboardCloneButton->setEnabled(isSelected);
	m_ui->keyboardRemoveButton->setEnabled(isSelected);
}

void PreferencesAdvancedPageWidget::updateReaddKeyboardProfileMenu()
{
	if (!m_ui->keyboardAddButton->menu())
	{
		return;
	}

	QStringList availableIdentifiers;
	QList<KeyboardProfile> availableShortcutsProfiles;
	QList<QFileInfo> allShortcutsProfiles(QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"))).entryInfoList(QDir::Files));
	allShortcutsProfiles.append(QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allShortcutsProfiles.count(); ++i)
	{
		const QString identifier(allShortcutsProfiles.at(i).baseName());

		if (!m_keyboardProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const KeyboardProfile profile(loadKeyboardProfile(identifier, false));

			if (!profile.identifier.isEmpty())
			{
				availableIdentifiers.append(identifier);

				availableShortcutsProfiles.append(profile);
			}
		}
	}

	m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->clear();
	m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->setEnabled(!availableShortcutsProfiles.isEmpty());

	for (int i = 0; i < availableShortcutsProfiles.count(); ++i)
	{
		m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->addAction((availableShortcutsProfiles.at(i).title.isEmpty() ? tr("(Untitled)") : availableShortcutsProfiles.at(i).title))->setData(availableShortcutsProfiles.at(i).identifier);
	}
}

void PreferencesAdvancedPageWidget::addMouseProfile()
{
	const QString identifier(createProfileIdentifier(m_ui->mouseViewWidget));

	if (identifier.isEmpty())
	{
		return;
	}

	MouseProfile profile;
	profile.title = tr("(Untitled)");

	m_mouseProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(tr("(Untitled)")));
	item->setData(identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->mouseViewWidget->insertRow(item);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::readdMouseProfile(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier(action->data().toString());
	const MouseProfile profile(loadMouseProfile(identifier, true));

	if (profile.identifier.isEmpty())
	{
		return;
	}

	m_mouseProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title));
	item->setToolTip(profile.description);
	item->setData(profile.identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->mouseViewWidget->insertRow(item);

	updateReaddMouseProfileMenu();

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::editMouseProfile()
{
	const QModelIndex index(m_ui->mouseViewWidget->currentIndex());
	const QString identifier(index.data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_mouseProfiles.contains(identifier))
	{
		if (!index.isValid())
		{
			addMouseProfile();
		}

		return;
	}

	MouseProfileDialog dialog(identifier, m_mouseProfiles, this);

	if (dialog.exec() == QDialog::Rejected)
	{
		return;
	}

	m_mouseProfiles[identifier] = dialog.getProfile();

	m_ui->mouseViewWidget->setData(index, (m_mouseProfiles[identifier].title.isEmpty() ? tr("(Untitled)") : m_mouseProfiles[identifier].title), Qt::DisplayRole);
	m_ui->mouseViewWidget->setData(index, m_mouseProfiles[identifier].description, Qt::ToolTipRole);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::cloneMouseProfile()
{
	const QString identifier(m_ui->mouseViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_mouseProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier(createProfileIdentifier(m_ui->mouseViewWidget, identifier));

	if (newIdentifier.isEmpty())
	{
		return;
	}

	m_mouseProfiles[newIdentifier] = m_mouseProfiles[identifier];
	m_mouseProfiles[newIdentifier].identifier = newIdentifier;
	m_mouseProfiles[newIdentifier].isModified = true;

	QStandardItem *item(new QStandardItem(m_mouseProfiles[newIdentifier].title.isEmpty() ? tr("(Untitled)") : m_mouseProfiles[newIdentifier].title));
	item->setToolTip(m_mouseProfiles[newIdentifier].description);
	item->setData(newIdentifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->mouseViewWidget->insertRow(item);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::removeMouseProfile()
{
	const QString identifier(m_ui->mouseViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_mouseProfiles.contains(identifier))
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this profile?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("mouse/") + identifier + QLatin1String(".ini")));

	if (QFile::exists(path))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete profile permanently")));
	}

	if (messageBox.exec() == QMessageBox::Yes)
	{
		if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
		{
			m_filesToRemove.append(path);
		}

		m_mouseProfiles.remove(identifier);

		m_ui->mouseViewWidget->removeRow();

		updateReaddMouseProfileMenu();

		emit settingsModified();
	}
}

void PreferencesAdvancedPageWidget::updateMouseProfileActions()
{
	const int currentRow(m_ui->mouseViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->mouseViewWidget->getRowCount());

	m_ui->mouseEditButton->setEnabled(isSelected);
	m_ui->mouseCloneButton->setEnabled(isSelected);
	m_ui->mouseRemoveButton->setEnabled(isSelected);
}

void PreferencesAdvancedPageWidget::updateReaddMouseProfileMenu()
{
	if (!m_ui->mouseAddButton->menu())
	{
		return;
	}

	QStringList availableIdentifiers;
	QList<MouseProfile> availableMouseProfiles;
	QList<QFileInfo> allMouseProfiles(QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"))).entryInfoList(QDir::Files));
	allMouseProfiles.append(QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allMouseProfiles.count(); ++i)
	{
		const QString identifier(allMouseProfiles.at(i).baseName());

		if (!m_mouseProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const MouseProfile profile(loadMouseProfile(identifier, false));

			if (!profile.identifier.isEmpty())
			{
				availableIdentifiers.append(identifier);

				availableMouseProfiles.append(profile);
			}
		}
	}

	m_ui->mouseAddButton->menu()->actions().at(1)->menu()->clear();
	m_ui->mouseAddButton->menu()->actions().at(1)->menu()->setEnabled(!availableMouseProfiles.isEmpty());

	for (int i = 0; i < availableMouseProfiles.count(); ++i)
	{
		m_ui->mouseAddButton->menu()->actions().at(1)->menu()->addAction((availableMouseProfiles.at(i).title.isEmpty() ? tr("(Untitled)") : availableMouseProfiles.at(i).title))->setData(availableMouseProfiles.at(i).identifier);
	}
}

void PreferencesAdvancedPageWidget::updateJavaScriptOptions()
{
	const bool isSet(!m_javaScriptOptions.isEmpty());

	if (!isSet)
	{
		const QList<int> javaScriptOptions({SettingsManager::Browser_EnableFullScreenOption, SettingsManager::Browser_JavaScriptCanAccessClipboardOption, SettingsManager::Browser_JavaScriptCanChangeWindowGeometryOption, SettingsManager::Browser_JavaScriptCanCloseWindowsOption, SettingsManager::Browser_JavaScriptCanDisableContextMenuOption, SettingsManager::Browser_JavaScriptCanOpenWindowsOption, SettingsManager::Browser_JavaScriptCanShowStatusMessagesOption});

		for (int i = 0; i < javaScriptOptions.count(); ++i)
		{
			m_javaScriptOptions[javaScriptOptions.at(i)] = SettingsManager::getValue(javaScriptOptions.at(i));
		}
	}

	JavaScriptPreferencesDialog dialog(m_javaScriptOptions, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_javaScriptOptions = dialog.getOptions();

		emit settingsModified();
	}
	else if (!isSet)
	{
		m_javaScriptOptions.clear();
	}
}

void PreferencesAdvancedPageWidget::changeCurrentPage()
{
	const QModelIndex index(m_ui->advancedViewWidget->currentIndex());

	if (index.isValid() && index.data(Qt::UserRole).type() == QVariant::Int)
	{
		m_ui->advancedStackedWidget->setCurrentIndex(index.data(Qt::UserRole).toInt());
	}
}

void PreferencesAdvancedPageWidget::save()
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	for (int i = 0; i < m_filesToRemove.count(); ++i)
	{
		QFile::remove(m_filesToRemove.at(i));
	}

	m_filesToRemove.clear();

	SettingsManager::setValue(SettingsManager::AddressField_SuggestBookmarksOption, m_ui->browsingSuggestBookmarksCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::AddressField_SuggestHistoryOption, m_ui->browsingSuggestHistoryCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::AddressField_SuggestLocalPathsOption, m_ui->browsingSuggestLocalPathsCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::AddressField_ShowCompletionCategoriesOption, m_ui->browsingCategoriesCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::AddressField_CompletionDisplayModeOption, m_ui->browsingDisplayModeComboBox->currentData(Qt::UserRole).toString());

	QSettings notificationsSettings(SessionsManager::getWritableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);
	notificationsSettings.setIniCodec("UTF-8");
	notificationsSettings.clear();

	for (int i = 0; i < m_ui->notificationsItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->notificationsItemView->getIndex(i, 0));
		const QString eventName(NotificationsManager::getEventName(index.data(Qt::UserRole).toInt()));

		if (eventName.isEmpty())
		{
			continue;
		}

		notificationsSettings.beginGroup(eventName);
		notificationsSettings.setValue(QLatin1String("playSound"), index.data(Qt::UserRole + 1).toString());
		notificationsSettings.setValue(QLatin1String("showAlert"), index.data(Qt::UserRole + 2).toBool());
		notificationsSettings.setValue(QLatin1String("showNotification"), index.data(Qt::UserRole + 3).toBool());
		notificationsSettings.endGroup();
	}

	SettingsManager::setValue(SettingsManager::Interface_UseNativeNotificationsOption, m_ui->preferNativeNotificationsCheckBox->isChecked());

	const QString widgetStyle((m_ui->appearranceWidgetStyleComboBox->currentIndex() == 0) ? QString() : m_ui->appearranceWidgetStyleComboBox->currentText());

	Application::setStyle(ThemesManager::createStyle(widgetStyle));

	SettingsManager::setValue(SettingsManager::Interface_WidgetStyleOption, widgetStyle);
	SettingsManager::setValue(SettingsManager::Interface_StyleSheetOption, m_ui->appearranceStyleSheetFilePathWidget->getPath());
	SettingsManager::setValue(SettingsManager::Browser_EnableTrayIconOption, m_ui->enableTrayIconCheckBox->isChecked());

	if (m_ui->appearranceStyleSheetFilePathWidget->getPath().isEmpty())
	{
		Application::getInstance()->setStyleSheet(QString());
	}
	else
	{
		QFile file(m_ui->appearranceStyleSheetFilePathWidget->getPath());

		if (file.open(QIODevice::ReadOnly))
		{
			Application::getInstance()->setStyleSheet(file.readAll());

			file.close();
		}
		else
		{
			Application::getInstance()->setStyleSheet(QString());
		}
	}

	SettingsManager::setValue(SettingsManager::Browser_EnableImagesOption, m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setValue(SettingsManager::Browser_EnableJavaScriptOption, m_ui->enableJavaScriptCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::Browser_EnableJavaOption, m_ui->enableJavaCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::Browser_EnablePluginsOption, m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setValue(SettingsManager::Content_UserStyleSheetOption, m_ui->userStyleSheetFilePathWidget->getPath());

	Settings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QStringList handlers(handlersSettings.getGroups());

	for (int i = 0; i < handlers.count(); ++i)
	{
		if (handlers.at(i).contains(QLatin1Char('/')))
		{
			handlersSettings.removeGroup(handlers.at(i));
		}
	}

	for (int i = 0; i < m_ui->downloadsItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->downloadsItemView->getIndex(i, 0));

		if (index.data(Qt::DisplayRole).isNull())
		{
			continue;
		}

		handlersSettings.beginGroup(index.data(Qt::DisplayRole).toString());
		handlersSettings.setValue(QLatin1String("transferMode"), index.data(Qt::UserRole).toString());
		handlersSettings.setValue(QLatin1String("downloadsPath"), index.data(Qt::UserRole + 1).toString());
		handlersSettings.setValue(QLatin1String("openCommand"), index.data(Qt::UserRole + 2).toString());
		handlersSettings.endGroup();
	}

	handlersSettings.save(SessionsManager::getWritableDataPath(QLatin1String("handlers.ini")));

	SettingsManager::setValue(SettingsManager::Network_EnableReferrerOption, m_ui->sendReferrerCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::Network_UserAgentOption, m_ui->userAgentComboBox->currentData().toString());
	SettingsManager::setValue(SettingsManager::Network_ProxyModeOption, m_ui->proxyModeComboBox->currentData(Qt::UserRole).toString());

	if (!m_ui->allProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(SettingsManager::Proxy_UseCommonOption, m_ui->allProxyCheckBox->isChecked());
	}

	if (!m_ui->httpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(SettingsManager::Proxy_UseHttpOption, m_ui->httpProxyCheckBox->isChecked());
	}

	if (!m_ui->httpsProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(SettingsManager::Proxy_UseHttpsOption, m_ui->httpsProxyCheckBox->isChecked());
	}

	if (!m_ui->ftpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(SettingsManager::Proxy_UseFtpOption, m_ui->ftpProxyCheckBox->isChecked());
	}

	if (!m_ui->socksProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(SettingsManager::Proxy_UseSocksOption, m_ui->socksProxyCheckBox->isChecked());
	}

	SettingsManager::setValue(SettingsManager::Proxy_CommonServersOption, m_ui->allProxyServersLineEdit->text());
	SettingsManager::setValue(SettingsManager::Proxy_HttpServersOption, m_ui->httpProxyServersLineEdit->text());
	SettingsManager::setValue(SettingsManager::Proxy_HttpsServersOption, m_ui->httpsProxyServersLineEdit->text());
	SettingsManager::setValue(SettingsManager::Proxy_FtpServersOption, m_ui->ftpProxyServersLineEdit->text());
	SettingsManager::setValue(SettingsManager::Proxy_SocksServersOption, m_ui->socksProxyServersLineEdit->text());
	SettingsManager::setValue(SettingsManager::Proxy_CommonPortOption, m_ui->allProxyPortSpinBox->value());
	SettingsManager::setValue(SettingsManager::Proxy_HttpPortOption, m_ui->httpProxyPortSpinBox->value());
	SettingsManager::setValue(SettingsManager::Proxy_HttpsPortOption, m_ui->httpsProxyPortSpinBox->value());
	SettingsManager::setValue(SettingsManager::Proxy_FtpPortOption, m_ui->ftpProxyPortSpinBox->value());
	SettingsManager::setValue(SettingsManager::Proxy_SocksPortOption, m_ui->socksProxyPortSpinBox->value());
	SettingsManager::setValue(SettingsManager::Proxy_AutomaticConfigurationPathOption, m_ui->automaticProxyConfigurationFilePathWidget->getPath());
	SettingsManager::setValue(SettingsManager::Proxy_UseSystemAuthenticationOption, m_ui->proxySystemAuthentication->isChecked());

	QStandardItemModel *proxyListModel(m_ui->proxyExceptionsItemView->getSourceModel());
	QStringList proxyExceptions;

	for (int i = 0; i < proxyListModel->rowCount(); ++i)
	{
		const QString value(proxyListModel->item(i)->text());

		if (!value.isEmpty())
		{
			proxyExceptions.append(value);
		}
	}

	SettingsManager::setValue(SettingsManager::Proxy_ExceptionsOption, proxyExceptions);

	if (m_userAgentsModified)
	{
		QJsonArray userAgentsArray;

		for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
		{
			QJsonObject userAgentObject;
			userAgentObject.insert(QLatin1String("identifier"), m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString());
			userAgentObject.insert(QLatin1String("title"), m_ui->userAgentComboBox->itemText(i));
			userAgentObject.insert(QLatin1String("value"), m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString());

			userAgentsArray.append(userAgentObject);
		}

		JsonSettings settings;
		settings.setArray(userAgentsArray);
		settings.save(SessionsManager::getWritableDataPath(QLatin1String("userAgents.json")));

		NetworkManagerFactory::loadUserAgents();
	}

	if (m_ui->ciphersViewWidget->isModified())
	{
		QStringList ciphers;

		for (int i = 0; i < m_ui->ciphersViewWidget->getRowCount(); ++i)
		{
			ciphers.append(m_ui->ciphersViewWidget->getIndex(i, 0).data(Qt::DisplayRole).toString());
		}

		SettingsManager::setValue(SettingsManager::Security_CiphersOption, ciphers);
	}

	SettingsManager::setValue(SettingsManager::Updates_ActiveChannelsOption, getSelectedUpdateChannels());
	SettingsManager::setValue(SettingsManager::Updates_AutomaticInstallOption, m_ui->autoInstallCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::Updates_CheckIntervalOption, m_ui->intervalSpinBox->value());

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("keyboard")));

	bool needsKeyboardProfilesReload(false);
	QHash<QString, KeyboardProfile>::iterator keyboardProfilesIterator;

	for (keyboardProfilesIterator = m_keyboardProfiles.begin(); keyboardProfilesIterator != m_keyboardProfiles.end(); ++keyboardProfilesIterator)
	{
		if (!keyboardProfilesIterator.value().isModified)
		{
			continue;
		}

		Settings settings(SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + keyboardProfilesIterator.key() + QLatin1String(".ini")));
		settings.clear();

		QString comment;
		QTextStream stream(&comment);
		stream.setCodec("UTF-8");
		stream << QLatin1String("Title: ") << (keyboardProfilesIterator.value().title.isEmpty() ? tr("(Untitled)") : keyboardProfilesIterator.value().title) << QLatin1Char('\n');
		stream << QLatin1String("Description: ") << keyboardProfilesIterator.value().description << QLatin1Char('\n');
		stream << QLatin1String("Type: keyboard-profile\n");
		stream << QLatin1String("Author: ") << keyboardProfilesIterator.value().author << QLatin1Char('\n');
		stream << QLatin1String("Version: ") << keyboardProfilesIterator.value().version;

		settings.setComment(comment);

		QHash<int, QVector<QKeySequence> >::iterator shortcutsIterator;

		for (shortcutsIterator = keyboardProfilesIterator.value().shortcuts.begin(); shortcutsIterator != keyboardProfilesIterator.value().shortcuts.end(); ++shortcutsIterator)
		{
			if (!shortcutsIterator.value().isEmpty())
			{
				QStringList shortcuts;

				for (int i = 0; i < shortcutsIterator.value().count(); ++i)
				{
					shortcuts.append(shortcutsIterator.value().at(i).toString());
				}

				settings.beginGroup(ActionsManager::getActionName(shortcutsIterator.key()));
				settings.setValue(QLatin1String("shortcuts"), shortcuts.join(QLatin1Char(' ')));
				settings.endGroup();
			}
		}

		settings.save();

		needsKeyboardProfilesReload = true;
	}

	QStringList keyboardProfiles;

	for (int i = 0; i < m_ui->keyboardViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->keyboardViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());

		if (!identifier.isEmpty())
		{
			keyboardProfiles.append(identifier);
		}
	}

	if (needsKeyboardProfilesReload && SettingsManager::getValue(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList() == keyboardProfiles && SettingsManager::getValue(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool() == m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked())
	{
		ActionsManager::loadProfiles();
	}

	SettingsManager::setValue(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption, keyboardProfiles);
	SettingsManager::setValue(SettingsManager::Browser_EnableSingleKeyShortcutsOption, m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked());

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("mouse")));

	bool needsMouseProfilesReload(false);
	QHash<QString, MouseProfile>::iterator mouseProfilesIterator;

	for (mouseProfilesIterator = m_mouseProfiles.begin(); mouseProfilesIterator != m_mouseProfiles.end(); ++mouseProfilesIterator)
	{
		if (!mouseProfilesIterator.value().isModified)
		{
			continue;
		}

		Settings settings(SessionsManager::getWritableDataPath(QLatin1String("mouse/") + mouseProfilesIterator.key() + QLatin1String(".ini")));
		settings.clear();

		QString comment;
		QTextStream stream(&comment);
		stream.setCodec("UTF-8");
		stream << QLatin1String("Title: ") << (mouseProfilesIterator.value().title.isEmpty() ? tr("(Untitled)") : mouseProfilesIterator.value().title) << QLatin1Char('\n');
		stream << QLatin1String("Description: ") << mouseProfilesIterator.value().description << QLatin1Char('\n');
		stream << QLatin1String("Type: mouse-profile\n");
		stream << QLatin1String("Author: ") << mouseProfilesIterator.value().author << QLatin1Char('\n');
		stream << QLatin1String("Version: ") << mouseProfilesIterator.value().version;

		settings.setComment(comment);

		QHash<QString, QHash<QString, int> >::iterator contextsIterator;

		for (contextsIterator = mouseProfilesIterator.value().gestures.begin(); contextsIterator != mouseProfilesIterator.value().gestures.end(); ++contextsIterator)
		{
			settings.beginGroup(contextsIterator.key());

			QHash<QString, int>::iterator gesturesIterator;

			for (gesturesIterator = contextsIterator.value().begin(); gesturesIterator != contextsIterator.value().end(); ++gesturesIterator)
			{
				settings.setValue(gesturesIterator.key(), ActionsManager::getActionName(gesturesIterator.value()));
			}

			settings.endGroup();
		}

		settings.save();

		needsMouseProfilesReload = true;
	}

	QStringList mouseProfiles;

	for (int i = 0; i < m_ui->mouseViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->mouseViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());

		if (!identifier.isEmpty())
		{
			mouseProfiles.append(identifier);
		}
	}

	if (needsMouseProfilesReload && SettingsManager::getValue(SettingsManager::Browser_MouseProfilesOrderOption).toStringList() == mouseProfiles && SettingsManager::getValue(SettingsManager::Browser_EnableMouseGesturesOption).toBool() == m_ui->mouseEnableGesturesCheckBox->isChecked())
	{
		GesturesManager::loadProfiles();
	}

	SettingsManager::setValue(SettingsManager::Browser_MouseProfilesOrderOption, mouseProfiles);
	SettingsManager::setValue(SettingsManager::Browser_EnableMouseGesturesOption, m_ui->mouseEnableGesturesCheckBox->isChecked());

	if (!m_javaScriptOptions.isEmpty())
	{
		QHash<int, QVariant>::iterator javaScriptOptionsIterator;

		for (javaScriptOptionsIterator = m_javaScriptOptions.begin(); javaScriptOptionsIterator != m_javaScriptOptions.end(); ++javaScriptOptionsIterator)
		{
			SettingsManager::setValue(javaScriptOptionsIterator.key(), javaScriptOptionsIterator.value());
		}
	}

	updateReaddKeyboardProfileMenu();
}

void PreferencesAdvancedPageWidget::updatePageSwitcher()
{
	int maximumWidth(100);

	for (int i = 0; i < m_ui->advancedViewWidget->model()->rowCount(); ++i)
	{
		const int itemWidth(m_ui->advancedViewWidget->fontMetrics().width(m_ui->advancedViewWidget->model()->index(i, 0).data(Qt::DisplayRole).toString()) + 10);

		if (itemWidth > maximumWidth)
		{
			maximumWidth = itemWidth;
		}
	}

	m_ui->advancedViewWidget->setMinimumWidth(qMin(200, maximumWidth));
}

QString PreferencesAdvancedPageWidget::createProfileIdentifier(ItemViewWidget *view, const QString &base) const
{
	QStringList identifiers;

	for (int i = 0; i < view->getRowCount(); ++i)
	{
		const QString identifier(view->getIndex(i, 0).data(Qt::UserRole).toString());

		if (!identifier.isEmpty())
		{
			identifiers.append(identifier);
		}
	}

	return Utils::createIdentifier(base, identifiers);
}

QStringList PreferencesAdvancedPageWidget::getSelectedUpdateChannels() const
{
	QStandardItemModel *updateListModel(m_ui->updateChannelsItemView->getSourceModel());
	QStringList updateChannels;

	for (int i = 0; i < updateListModel->rowCount(); ++i)
	{
		QStandardItem *item(updateListModel->item(i));

		if (item->checkState() == Qt::Checked)
		{
			updateChannels.append(item->data(Qt::UserRole).toString());
		}
	}

	return updateChannels;
}

KeyboardProfile PreferencesAdvancedPageWidget::loadKeyboardProfile(const QString &identifier, bool loadShortcuts) const
{
	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".ini")));
	const QStringList comments(settings.getComment().split(QLatin1Char('\n')));
	KeyboardProfile profile;
	profile.identifier = identifier;

	for (int i = 0; i < comments.count(); ++i)
	{
		const QString key(comments.at(i).section(QLatin1Char(':'), 0, 0).trimmed());
		const QString value(comments.at(i).section(QLatin1Char(':'), 1).trimmed());

		if (key == QLatin1String("Title"))
		{
			profile.title = value;
		}
		else if (key == QLatin1String("Description"))
		{
			profile.description = value;
		}
		else if (key == QLatin1String("Author"))
		{
			profile.author = value;
		}
		else if (key == QLatin1String("Version"))
		{
			profile.version = value;
		}
	}

	if (!loadShortcuts)
	{
		return profile;
	}

	const QStringList actions(settings.getGroups());

	for (int i = 0; i < actions.count(); ++i)
	{
		const int action(ActionsManager::getActionIdentifier(actions.at(i)));

		if (action < 0)
		{
			continue;
		}

		settings.beginGroup(actions.at(i));

		const QStringList rawShortcuts(settings.getValue(QLatin1String("shortcuts")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts));
		QVector<QKeySequence> shortcuts;
		shortcuts.reserve(rawShortcuts.count());

		for (int j = 0; j < rawShortcuts.count(); ++j)
		{
			const QKeySequence shortcut(QKeySequence(rawShortcuts.at(j)));

			if (!shortcut.isEmpty())
			{
				shortcuts.append(shortcut);
			}
		}

		if (!shortcuts.isEmpty())
		{
			profile.shortcuts[action] = shortcuts;
		}

		settings.endGroup();
	}

	return profile;
}

MouseProfile PreferencesAdvancedPageWidget::loadMouseProfile(const QString &identifier, bool loadGestures) const
{
	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("mouse/") + identifier + QLatin1String(".ini")));
	const QStringList comments(settings.getComment().split(QLatin1Char('\n')));
	MouseProfile profile;
	profile.identifier = identifier;

	for (int i = 0; i < comments.count(); ++i)
	{
		const QString key(comments.at(i).section(QLatin1Char(':'), 0, 0).trimmed());
		const QString value(comments.at(i).section(QLatin1Char(':'), 1).trimmed());

		if (key == QLatin1String("Title"))
		{
			profile.title = value;
		}
		else if (key == QLatin1String("Description"))
		{
			profile.description = value;
		}
		else if (key == QLatin1String("Author"))
		{
			profile.author = value;
		}
		else if (key == QLatin1String("Version"))
		{
			profile.version = value;
		}
	}

	if (!loadGestures)
	{
		return profile;
	}

	const QStringList contexts(settings.getGroups());

	for (int i = 0; i < contexts.count(); ++i)
	{
		settings.beginGroup(contexts.at(i));

		QHash<QString, int> rawGestures;
		const QStringList gestures(settings.getKeys());

		for (int j = 0; j < gestures.count(); ++j)
		{
			const int action(ActionsManager::getActionIdentifier(settings.getValue(gestures.at(j)).toString()));

			if (action < 0)
			{
				continue;
			}

			rawGestures[gestures.at(j)] = action;
		}

		profile.gestures[contexts.at(i)] = rawGestures;

		settings.endGroup();
	}

	return profile;
}

}
