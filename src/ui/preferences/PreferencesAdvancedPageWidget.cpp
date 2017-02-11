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
#include "ProxyPropertiesDialog.h"
#include "UserAgentPropertiesDialog.h"
#include "../Style.h"
#include "../../core/ActionsManager.h"
#include "../../core/Application.h"
#include "../../core/GesturesManager.h"
#include "../../core/JsonSettings.h"
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

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_EnableImagesOption).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
	m_ui->javaScriptOptionsButton->setEnabled(m_ui->enableJavaScriptCheckBox->isChecked());
	m_ui->enablePluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->enablePluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->enablePluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getValue(SettingsManager::Permissions_EnablePluginsOption).toString()));

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

	TreeModel *userAgentsModel(new UserAgentsModel(SettingsManager::getValue(SettingsManager::Network_UserAgentOption).toString(), true, this));
	userAgentsModel->setHorizontalHeaderLabels({tr("Title"), tr("Value")});

	m_ui->userAgentsViewWidget->setModel(userAgentsModel);
	m_ui->userAgentsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->userAgentsViewWidget->setExclusive(true);
	m_ui->userAgentsViewWidget->expandAll();

	QMenu *addUserAgentMenu(new QMenu(m_ui->userAgentsAddButton));
	addUserAgentMenu->addAction(ThemesManager::getIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(TreeModel::FolderType);
	addUserAgentMenu->addAction(tr("Add User Agent…"))->setData(TreeModel::EntryType);
	addUserAgentMenu->addAction(tr("Add Separator"))->setData(TreeModel::SeparatorType);

	m_ui->userAgentsAddButton->setMenu(addUserAgentMenu);

	TreeModel *proxiesModel(new ProxiesModel(SettingsManager::getValue(SettingsManager::Network_ProxyOption).toString(), true, this));
	proxiesModel->setHorizontalHeaderLabels({tr("Title")});

	m_ui->proxiesViewWidget->setModel(proxiesModel);
	m_ui->proxiesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->proxiesViewWidget->setExclusive(true);
	m_ui->proxiesViewWidget->expandAll();

	QMenu *addProxyMenu(new QMenu(m_ui->proxiesAddButton));
	addProxyMenu->addAction(ThemesManager::getIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(TreeModel::FolderType);
	addProxyMenu->addAction(tr("Add Proxy…"))->setData(TreeModel::EntryType);
	addProxyMenu->addAction(tr("Add Separator"))->setData(TreeModel::SeparatorType);

	m_ui->proxiesAddButton->setMenu(addProxyMenu);

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
		m_ui->ciphersViewWidget->setLayoutDirection(Qt::LeftToRight);
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
	connect(m_ui->userAgentsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateUserAgentsActions()));
	connect(m_ui->userAgentsAddButton->menu(), SIGNAL(triggered(QAction*)), this, SLOT(addUserAgent(QAction*)));
	connect(m_ui->userAgentsEditButton, SIGNAL(clicked()), this, SLOT(editUserAgent()));
	connect(m_ui->userAgentsRemoveButton, SIGNAL(clicked()), m_ui->userAgentsViewWidget, SLOT(removeRow()));
	connect(m_ui->proxiesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateProxiesActions()));
	connect(m_ui->proxiesAddButton->menu(), SIGNAL(triggered(QAction*)), this, SLOT(addProxy(QAction*)));
	connect(m_ui->proxiesEditButton, SIGNAL(clicked()), this, SLOT(editProxy()));
	connect(m_ui->proxiesRemoveButton, SIGNAL(clicked()), m_ui->proxiesViewWidget, SLOT(removeRow()));
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

	connect(m_ui->downloadsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateDownloadsActions()));
	connect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
}

void PreferencesAdvancedPageWidget::updateDownloadsMode()
{
	m_ui->downloadsOpenOptionsWidget->setEnabled(m_ui->downloadsOpenButton->isChecked());
	m_ui->downloadsSaveOptionsWidget->setEnabled(m_ui->downloadsSaveButton->isChecked());
}

void PreferencesAdvancedPageWidget::addUserAgent(QAction *action)
{
	if (!action)
	{
		return;
	}

	TreeModel *model(qobject_cast<TreeModel*>(m_ui->userAgentsViewWidget->getSourceModel()));
	QStandardItem *parent(model->itemFromIndex(m_ui->userAgentsViewWidget->getCurrentIndex()));

	if (!parent)
	{
		parent = model->invisibleRootItem();
	}

	int row(-1);

	if (static_cast<TreeModel::ItemType>(parent->data(TreeModel::TypeRole).toInt()) != TreeModel::FolderType)
	{
		row = (parent->row() + 1);

		parent = parent->parent();
	}

	switch (static_cast<TreeModel::ItemType>(action->data().toInt()))
	{
		case TreeModel::FolderType:
			{
				bool result(false);
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, QString(), &result));

				if (result)
				{
					QList<QStandardItem*> items({new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title), new QStandardItem()});
					items[0]->setData(Utils::createIdentifier(QString(), QVariant(model->getAllData(UserAgentsModel::IdentifierRole, 0)).toStringList()), UserAgentsModel::IdentifierRole);

					model->insertRow(items, parent, row, TreeModel::FolderType);

					m_ui->userAgentsViewWidget->expand(items[0]->index());
				}
			}

			break;
		case TreeModel::EntryType:
			{
				UserAgentDefinition userAgent;
				userAgent.title = tr("Custom");
				userAgent.value = NetworkManagerFactory::getUserAgent(QLatin1String("default")).value;

				UserAgentPropertiesDialog dialog(userAgent, this);

				if (dialog.exec() == QDialog::Accepted)
				{
					userAgent = dialog.getUserAgent();
					userAgent.identifier = Utils::createIdentifier(QString(), QVariant(model->getAllData(UserAgentsModel::IdentifierRole, 0)).toStringList());

					QList<QStandardItem*> items({new QStandardItem(userAgent.title.isEmpty() ? tr("(Untitled)") : userAgent.title), new QStandardItem(userAgent.value)});
					items[0]->setData(userAgent.identifier, UserAgentsModel::IdentifierRole);

					model->insertRow(items, parent, row, TreeModel::EntryType);
				}
			}

			break;
		case TreeModel::SeparatorType:
			model->insertRow(QList<QStandardItem*>({new QStandardItem(), new QStandardItem()}), parent, row, TreeModel::SeparatorType);

			break;
		default:
			break;
	}
}

void PreferencesAdvancedPageWidget::editUserAgent()
{
	const QModelIndex index(m_ui->userAgentsViewWidget->getCurrentIndex());

	if (!index.isValid())
	{
		return;
	}

	const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(index.data(TreeModel::TypeRole).toInt()));

	if (type == TreeModel::FolderType)
	{
		bool result(false);
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(Qt::DisplayRole).toString(), &result));

		if (result)
		{
			m_ui->userAgentsViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), Qt::DisplayRole);
		}
	}
	else if (type == TreeModel::EntryType)
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = index.data(UserAgentsModel::IdentifierRole).toString();
		userAgent.title = index.data(Qt::DisplayRole).toString();
		userAgent.value = index.sibling(index.row(), 1).data(Qt::DisplayRole).toString();

		UserAgentPropertiesDialog dialog(userAgent, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			userAgent = dialog.getUserAgent();

			m_ui->userAgentsViewWidget->setData(index, (userAgent.title.isEmpty() ? tr("(Untitled)") : userAgent.title), Qt::DisplayRole);
			m_ui->userAgentsViewWidget->setData(index.sibling(index.row(), 1), userAgent.value, Qt::DisplayRole);
		}
	}
}

void PreferencesAdvancedPageWidget::updateUserAgentsActions()
{
	const QModelIndex index(m_ui->userAgentsViewWidget->getCurrentIndex());
	const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(index.data(TreeModel::TypeRole).toInt()));

	m_ui->userAgentsEditButton->setEnabled(index.isValid() && (type == TreeModel::FolderType || type == TreeModel::EntryType));
	m_ui->userAgentsRemoveButton->setEnabled(index.isValid());
}

void PreferencesAdvancedPageWidget::saveUsuerAgents(QJsonArray *userAgents, QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		QStandardItem *item(parent->child(i, 0));

		if (item)
		{
			const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(item->data(TreeModel::TypeRole).toInt()));

			if (type == TreeModel::FolderType || type == TreeModel::EntryType)
			{
				QJsonObject userAgentObject;
				userAgentObject.insert(QLatin1String("identifier"), item->data(UserAgentsModel::IdentifierRole).toString());
				userAgentObject.insert(QLatin1String("title"), item->data(Qt::DisplayRole).toString());

				if (type == TreeModel::FolderType)
				{
					QJsonArray userAgentsArray;

					saveUsuerAgents(&userAgentsArray, item);

					userAgentObject.insert(QLatin1String("children"), userAgentsArray);
				}
				else
				{
					userAgentObject.insert(QLatin1String("value"), item->index().sibling(i, 1).data(Qt::DisplayRole).toString());
				}

				userAgents->append(userAgentObject);
			}
			else
			{
				userAgents->append(QJsonValue(QLatin1String("separator")));
			}
		}
	}
}

void PreferencesAdvancedPageWidget::addProxy(QAction *action)
{
	if (!action)
	{
		return;
	}

	TreeModel *model(qobject_cast<TreeModel*>(m_ui->proxiesViewWidget->getSourceModel()));
	QStandardItem *parent(model->itemFromIndex(m_ui->proxiesViewWidget->getCurrentIndex()));

	if (!parent)
	{
		parent = model->invisibleRootItem();
	}

	int row(-1);

	if (static_cast<TreeModel::ItemType>(parent->data(TreeModel::TypeRole).toInt()) != TreeModel::FolderType)
	{
		row = (parent->row() + 1);

		parent = parent->parent();
	}

	switch (static_cast<TreeModel::ItemType>(action->data().toInt()))
	{
		case TreeModel::FolderType:
			{
				bool result(false);
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, QString(), &result));

				if (result)
				{
					QStandardItem *item(new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title));
					item->setData(Utils::createIdentifier(QString(), QVariant(model->getAllData(ProxiesModel::IdentifierRole, 0)).toStringList()), ProxiesModel::IdentifierRole);

					model->insertRow(item, parent, row, TreeModel::FolderType);

					m_ui->proxiesViewWidget->expand(item->index());
				}
			}

			break;
		case TreeModel::EntryType:
			{
				ProxyDefinition proxy;
				proxy.title = tr("Custom");

				ProxyPropertiesDialog dialog(proxy, this);

				if (dialog.exec() == QDialog::Accepted)
				{
					proxy = dialog.getProxy();
					proxy.identifier = Utils::createIdentifier(QString(), QVariant(model->getAllData(ProxiesModel::IdentifierRole, 0)).toStringList());

					m_proxies[proxy.identifier] = proxy;

					QStandardItem *item(new QStandardItem(proxy.title.isEmpty() ? tr("(Untitled)") : proxy.title));
					item->setData(proxy.identifier, ProxiesModel::IdentifierRole);

					model->insertRow(item, parent, row, TreeModel::EntryType);
				}
			}

			break;
		case TreeModel::SeparatorType:
			model->insertRow(new QStandardItem(), parent, row, TreeModel::SeparatorType);

			break;
		default:
			break;
	}
}

void PreferencesAdvancedPageWidget::editProxy()
{
	const QModelIndex index(m_ui->proxiesViewWidget->getCurrentIndex());

	if (!index.isValid())
	{
		return;
	}

	const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(index.data(TreeModel::TypeRole).toInt()));

	if (type == TreeModel::FolderType)
	{
		bool result(false);
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(Qt::DisplayRole).toString(), &result));

		if (result)
		{
			m_ui->proxiesViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), Qt::DisplayRole);
		}
	}
	else if (type == TreeModel::EntryType)
	{
		const QString identifier(index.data(ProxiesModel::IdentifierRole).toString());
		ProxyPropertiesDialog dialog((m_proxies.value(identifier, NetworkManagerFactory::getProxy(identifier))), this);

		if (dialog.exec() == QDialog::Accepted)
		{
			const ProxyDefinition proxy(dialog.getProxy());

			m_proxies[identifier] = proxy;

			m_ui->proxiesViewWidget->markAsModified();
			m_ui->proxiesViewWidget->setData(index, (proxy.title.isEmpty() ? tr("(Untitled)") : proxy.title), Qt::DisplayRole);
		}
	}
}

void PreferencesAdvancedPageWidget::updateProxiesActions()
{
	const QModelIndex index(m_ui->proxiesViewWidget->getCurrentIndex());
	const QString identifier(index.data(ProxiesModel::IdentifierRole).toString());
	const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(index.data(TreeModel::TypeRole).toInt()));
	const bool isReadOnly(identifier == QLatin1String("noProxy") || identifier == QLatin1String("system"));

	m_ui->proxiesEditButton->setEnabled(index.isValid() && !isReadOnly && (type == TreeModel::FolderType || type == TreeModel::EntryType));
	m_ui->proxiesRemoveButton->setEnabled(index.isValid() && !isReadOnly);
}

void PreferencesAdvancedPageWidget::saveProxies(QJsonArray *proxies, QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		QStandardItem *item(parent->child(i, 0));

		if (item)
		{
			const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(item->data(TreeModel::TypeRole).toInt()));

			if (type == TreeModel::FolderType)
			{
				QJsonArray proxiesArray;

				saveProxies(&proxiesArray, item);

				QJsonObject proxyObject;
				proxyObject.insert(QLatin1String("identifier"), item->data(ProxiesModel::IdentifierRole).toString());
				proxyObject.insert(QLatin1String("title"), item->data(Qt::DisplayRole).toString());
				proxyObject.insert(QLatin1String("children"), proxiesArray);

				proxies->append(proxyObject);
			}
			else if (type == TreeModel::EntryType)
			{
				const QString identifier(item->data(ProxiesModel::IdentifierRole).toString());
				const ProxyDefinition proxy(m_proxies.value(identifier, NetworkManagerFactory::getProxy(identifier)));
				QJsonObject proxyObject;
				proxyObject.insert(QLatin1String("identifier"), identifier);
				proxyObject.insert(QLatin1String("title"), item->data(Qt::DisplayRole).toString());

				switch (proxy.type)
				{
					case ProxyDefinition::NoProxy:
						proxyObject.insert(QLatin1String("type"), QLatin1String("noProxy"));

						break;
					case ProxyDefinition::ManualProxy:
						{
							QJsonArray serversArray;
							QHash<ProxyDefinition::ProtocolType, ProxyDefinition::ProxyServer>::const_iterator iterator;

							for (iterator = proxy.servers.constBegin(); iterator != proxy.servers.constEnd(); ++iterator)
							{
								QString protocol(QLatin1String("any"));

								switch (iterator.key())
								{
									case ProxyDefinition::HttpProtocol:
										protocol = QLatin1String("http");

										break;
									case ProxyDefinition::HttpsProtocol:
										protocol = QLatin1String("https");

										break;
									case ProxyDefinition::FtpProtocol:
										protocol = QLatin1String("ftp");

										break;
									case ProxyDefinition::SocksProtocol:
										protocol = QLatin1String("socks");

										break;
									default:
										break;
								}

								QJsonObject serverObject;
								serverObject.insert(QLatin1String("protocol"), protocol);
								serverObject.insert(QLatin1String("hostName"), iterator.value().hostName);
								serverObject.insert(QLatin1String("port"), iterator.value().port);

								serversArray.append(serverObject);
							}

							proxyObject.insert(QLatin1String("type"), QLatin1String("manualProxy"));
							proxyObject.insert(QLatin1String("servers"), serversArray);
						}

						break;
					case ProxyDefinition::AutomaticProxy:
						proxyObject.insert(QLatin1String("type"), QLatin1String("automaticProxy"));
						proxyObject.insert(QLatin1String("path"), proxy.path);

						break;
					default:
						proxyObject.insert(QLatin1String("type"), QLatin1String("systemProxy"));

						break;
				}

				if (!proxy.exceptions.isEmpty())
				{
					proxyObject.insert(QLatin1String("exceptions"), QJsonArray::fromStringList(proxy.exceptions));
				}

				if (proxy.usesSystemAuthentication)
				{
					proxyObject.insert(QLatin1String("usesSystemAuthentication"), true);
				}

				proxies->append(proxyObject);
			}
			else
			{
				proxies->append(QJsonValue(QLatin1String("separator")));
			}
		}
	}
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
		const QList<int> javaScriptOptions({SettingsManager::Permissions_EnableFullScreenOption, SettingsManager::Permissions_ScriptsCanAccessClipboardOption, SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, SettingsManager::Permissions_ScriptsCanCloseWindowsOption, SettingsManager::Permissions_ScriptsCanOpenWindowsOption, SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption, SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption});

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

	if (widgetStyle != SettingsManager::getValue(SettingsManager::Interface_WidgetStyleOption).toString())
	{
		Application::setStyle(ThemesManager::createStyle(widgetStyle));
	}

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

	SettingsManager::setValue(SettingsManager::Permissions_EnableImagesOption, m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setValue(SettingsManager::Permissions_EnableJavaScriptOption, m_ui->enableJavaScriptCheckBox->isChecked());
	SettingsManager::setValue(SettingsManager::Permissions_EnablePluginsOption, m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString());
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
	SettingsManager::setValue(SettingsManager::Network_UserAgentOption, m_ui->userAgentsViewWidget->getCheckedIndex().data(UserAgentsModel::IdentifierRole).toString());

	if (m_ui->userAgentsViewWidget->isModified())
	{
		QJsonArray userAgentsArray;

		saveUsuerAgents(&userAgentsArray, m_ui->userAgentsViewWidget->getSourceModel()->invisibleRootItem());

		JsonSettings settings;
		settings.setArray(userAgentsArray);
		settings.save(SessionsManager::getWritableDataPath(QLatin1String("userAgents.json")));

		NetworkManagerFactory::loadUserAgents();
	}

	SettingsManager::setValue(SettingsManager::Network_ProxyOption, m_ui->proxiesViewWidget->getCheckedIndex().data(ProxiesModel::IdentifierRole).toString());

	if (m_ui->proxiesViewWidget->isModified())
	{
		QJsonArray proxiesArray;

		saveProxies(&proxiesArray, m_ui->proxiesViewWidget->getSourceModel()->invisibleRootItem());

		JsonSettings settings;
		settings.setArray(proxiesArray);
		settings.save(SessionsManager::getWritableDataPath(QLatin1String("proxies.json")));

		NetworkManagerFactory::loadProxies();
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
