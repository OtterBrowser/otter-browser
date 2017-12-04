/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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
#include "../../core/IniSettings.h"
#include "../../core/JsonSettings.h"
#include "../../core/NotificationsManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
#include "../../core/ThemesManager.h"
#include "../../core/Utils.h"

#include "ui_PreferencesAdvancedPageWidget.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeDatabase>
#include <QtCore/QSettings>
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
	const QStringList navigationTitles({tr("Browsing"), tr("Notifications"), tr("Appearance"), tr("Content"), {}, tr("Downloads"), tr("Programs"), {}, tr("History"), tr("Network"), tr("Security"), tr("Updates"), {}, tr("Keyboard"), tr("Mouse")});
	int navigationIndex(0);

	for (int i = 0; i < navigationTitles.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(navigationTitles.at(i)));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

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

	m_ui->browsingEnableSmoothScrollingCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Interface_EnableSmoothScrollingOption).toBool());
	m_ui->browsingEnableSpellCheckCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_EnableSpellCheckOption).toBool());

	m_ui->browsingSuggestBookmarksCheckBox->setChecked(SettingsManager::getOption(SettingsManager::AddressField_SuggestBookmarksOption).toBool());
	m_ui->browsingSuggestHistoryCheckBox->setChecked(SettingsManager::getOption(SettingsManager::AddressField_SuggestHistoryOption).toBool());
	m_ui->browsingSuggestLocalPathsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::AddressField_SuggestLocalPathsOption).toBool());
	m_ui->browsingCategoriesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::AddressField_ShowCompletionCategoriesOption).toBool());

	m_ui->browsingDisplayModeComboBox->addItem(tr("Compact"), QLatin1String("compact"));
	m_ui->browsingDisplayModeComboBox->addItem(tr("Columns"), QLatin1String("columns"));

	const int displayModeIndex(m_ui->browsingDisplayModeComboBox->findData(SettingsManager::getOption(SettingsManager::AddressField_CompletionDisplayModeOption).toString()));

	m_ui->browsingDisplayModeComboBox->setCurrentIndex((displayModeIndex < 0) ? 1 : displayModeIndex);

	m_ui->notificationsPlaySoundButton->setIcon(ThemesManager::createIcon(QLatin1String("media-playback-start")));
	m_ui->notificationsPlaySoundFilePathWidget->setFilters({tr("WAV files (*.wav)")});

	QStandardItemModel *notificationsModel(new QStandardItemModel(this));
	notificationsModel->setHorizontalHeaderLabels({tr("Name"), tr("Description")});

	const QVector<NotificationsManager::EventDefinition> events(NotificationsManager::getEventDefinitions());

	for (int i = 0; i < events.count(); ++i)
	{
		QList<QStandardItem*> items({new QStandardItem(QCoreApplication::translate("notifications", events.at(i).title.toUtf8())), new QStandardItem(QCoreApplication::translate("notifications", events.at(i).description.toUtf8()))});
		items[0]->setData(events.at(i).identifier, IdentifierRole);
		items[0]->setData(events.at(i).playSound, SoundPathRole);
		items[0]->setData(events.at(i).showAlert, ShouldShowAlertRole);
		items[0]->setData(events.at(i).showNotification, ShouldShowNotificationRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		notificationsModel->appendRow(items);
	}

	m_ui->notificationsItemView->setModel(notificationsModel);
	m_ui->preferNativeNotificationsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Interface_UseNativeNotificationsOption).toBool());

	const QStringList widgetStyles(QStyleFactory::keys());

	m_ui->appearranceWidgetStyleComboBox->addItem(tr("System Style"));

	for (int i = 0; i < widgetStyles.count(); ++i)
	{
		m_ui->appearranceWidgetStyleComboBox->addItem(widgetStyles.at(i));
	}

	m_ui->appearranceWidgetStyleComboBox->setCurrentIndex(qMax(0, m_ui->appearranceWidgetStyleComboBox->findData(SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString(), Qt::DisplayRole)));
	m_ui->appearranceStyleSheetFilePathWidget->setPath(SettingsManager::getOption(SettingsManager::Interface_StyleSheetOption).toString());
	m_ui->appearranceStyleSheetFilePathWidget->setFilters({tr("Style sheets (*.css)")});
	m_ui->enableTrayIconCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_EnableTrayIconOption).toBool());

	m_ui->enableImagesComboBox->addItem(tr("All images"), QLatin1String("enabled"));
	m_ui->enableImagesComboBox->addItem(tr("Cached images"), QLatin1String("onlyCached"));
	m_ui->enableImagesComboBox->addItem(tr("No images"), QLatin1String("disabled"));

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
	m_ui->javaScriptOptionsButton->setEnabled(m_ui->enableJavaScriptCheckBox->isChecked());
	m_ui->enablePluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->enablePluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->enablePluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString()));

	m_ui->enablePluginsComboBox->setCurrentIndex((enablePluginsIndex < 0) ? 1 : enablePluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getOption(SettingsManager::Content_UserStyleSheetOption).toString());
	m_ui->userStyleSheetFilePathWidget->setFilters({tr("Style sheets (*.css)")});

	QStandardItemModel *downloadsModel(new QStandardItemModel(this));
	downloadsModel->setHorizontalHeaderLabels({tr("Name")});

	IniSettings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QStringList handlers(handlersSettings.getGroups());

	for (int i = 0; i < handlers.count(); ++i)
	{
		if (handlers.at(i) != QLatin1String("*") && !handlers.at(i).contains(QLatin1Char('/')))
		{
			continue;
		}

		handlersSettings.beginGroup(handlers.at(i));

		QStandardItem *item(new QStandardItem(handlers.at(i)));
		item->setData(handlersSettings.getValue(QLatin1String("transferMode")), TransferModeRole);
		item->setData(handlersSettings.getValue(QLatin1String("downloadsPath")), DownloadsPathRole);
		item->setData(handlersSettings.getValue(QLatin1String("openCommand")), OpenCommandRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		handlersSettings.endGroup();

		downloadsModel->appendRow(item);
	}

	m_ui->downloadsItemView->setModel(downloadsModel);
	m_ui->downloadsItemView->sortByColumn(0, Qt::AscendingOrder);
	m_ui->downloadsFilePathWidget->setSelectFile(false);
	m_ui->downloadsApplicationComboBoxWidget->setAlwaysShowDefaultApplication(true);

	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Network_EnableReferrerOption).toBool());

	TreeModel *userAgentsModel(new UserAgentsModel(SettingsManager::getOption(SettingsManager::Network_UserAgentOption).toString(), true, this));
	userAgentsModel->setHorizontalHeaderLabels({tr("Title"), tr("Value")});

	m_ui->userAgentsViewWidget->setModel(userAgentsModel);
	m_ui->userAgentsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->userAgentsViewWidget->setExclusive(true);
	m_ui->userAgentsViewWidget->expandAll();

	QMenu *addUserAgentMenu(new QMenu(m_ui->userAgentsAddButton));
	addUserAgentMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(TreeModel::FolderType);
	addUserAgentMenu->addAction(tr("Add User Agent…"))->setData(TreeModel::EntryType);
	addUserAgentMenu->addAction(tr("Add Separator"))->setData(TreeModel::SeparatorType);

	m_ui->userAgentsAddButton->setMenu(addUserAgentMenu);

	TreeModel *proxiesModel(new ProxiesModel(SettingsManager::getOption(SettingsManager::Network_ProxyOption).toString(), true, this));
	proxiesModel->setHorizontalHeaderLabels({tr("Title")});

	m_ui->proxiesViewWidget->setModel(proxiesModel);
	m_ui->proxiesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->proxiesViewWidget->setExclusive(true);
	m_ui->proxiesViewWidget->expandAll();

	QMenu *addProxyMenu(new QMenu(m_ui->proxiesAddButton));
	addProxyMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(TreeModel::FolderType);
	addProxyMenu->addAction(tr("Add Proxy…"))->setData(TreeModel::EntryType);
	addProxyMenu->addAction(tr("Add Separator"))->setData(TreeModel::SeparatorType);

	m_ui->proxiesAddButton->setMenu(addProxyMenu);

	m_ui->ciphersAddButton->setMenu(new QMenu(m_ui->ciphersAddButton));

	if (QSslSocket::supportsSsl())
	{
		QStandardItemModel *ciphersModel(new QStandardItemModel(this));
		const bool useDefaultCiphers(SettingsManager::getOption(SettingsManager::Security_CiphersOption).toString() == QLatin1String("default"));
		const QStringList selectedCiphers(useDefaultCiphers ? QStringList() : SettingsManager::getOption(SettingsManager::Security_CiphersOption).toStringList());

		for (int i = 0; i < selectedCiphers.count(); ++i)
		{
			const QSslCipher cipher(selectedCiphers.at(i));

			if (!cipher.isNull())
			{
				QStandardItem *item(new QStandardItem(cipher.name()));
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

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
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

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

	m_ui->ciphersMoveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->ciphersMoveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *updateChannelsModel(new QStandardItemModel(this));
	const QStringList activeUpdateChannels(SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList());
	const QMap<QString, QString> updateChannels({{QLatin1String("release"), tr("Stable version")}, {QLatin1String("beta"), tr("Beta version")}, {QLatin1String("weekly"), tr("Weekly development version")}});
	QMap<QString, QString>::const_iterator iterator;

	for (iterator = updateChannels.constBegin(); iterator != updateChannels.constEnd(); ++iterator)
	{
		QStandardItem *item(new QStandardItem(iterator.value()));
		item->setCheckable(true);
		item->setCheckState(activeUpdateChannels.contains(iterator.key()) ? Qt::Checked : Qt::Unchecked);
		item->setData(iterator.key(), Qt::UserRole);
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		updateChannelsModel->appendRow(item);
	}

	m_ui->updateChannelsItemView->setModel(updateChannelsModel);

	m_ui->autoInstallCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Updates_AutomaticInstallOption).toBool());
	m_ui->intervalSpinBox->setValue(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	updateUpdateChannelsActions();

	m_ui->keyboardMoveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->keyboardMoveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *keyboardProfilesModel(new QStandardItemModel(this));
	const QStringList keyboardProfiles(SettingsManager::getOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList());

	for (int i = 0; i < keyboardProfiles.count(); ++i)
	{
		const KeyboardProfile profile(keyboardProfiles.at(i), KeyboardProfile::FullMode);

		if (profile.getName().isEmpty())
		{
			continue;
		}

		m_keyboardProfiles[keyboardProfiles.at(i)] = profile;

		QStandardItem *item(new QStandardItem(profile.getTitle()));
		item->setToolTip(profile.getDescription());
		item->setData(profile.getName(), Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

		keyboardProfilesModel->appendRow(item);
	}

	m_ui->keyboardViewWidget->setModel(keyboardProfilesModel);

	QMenu *addKeyboardProfileMenu(new QMenu(m_ui->keyboardAddButton));
	addKeyboardProfileMenu->addAction(tr("New…"));
	addKeyboardProfileMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->keyboardAddButton));

	m_ui->keyboardAddButton->setMenu(addKeyboardProfileMenu);
	m_ui->keyboardEnableSingleKeyShortcutsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool());

	updateReaddKeyboardProfileMenu();

	m_ui->mouseMoveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->mouseMoveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *mouseProfilesModel(new QStandardItemModel(this));
	const QStringList mouseProfiles(SettingsManager::getOption(SettingsManager::Browser_MouseProfilesOrderOption).toStringList());

	for (int i = 0; i < mouseProfiles.count(); ++i)
	{
		const MouseProfile profile(mouseProfiles.at(i), MouseProfile::FullMode);

		if (profile.getName().isEmpty())
		{
			continue;
		}

		m_mouseProfiles[mouseProfiles.at(i)] = profile;

		QStandardItem *item(new QStandardItem(profile.getTitle()));
		item->setToolTip(profile.getDescription());
		item->setData(profile.getName(), Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

		mouseProfilesModel->appendRow(item);
	}

	m_ui->mouseViewWidget->setModel(mouseProfilesModel);

	QMenu *addMouseProfileMenu(new QMenu(m_ui->mouseAddButton));
	addMouseProfileMenu->addAction(tr("New…"));
	addMouseProfileMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->mouseAddButton));

	m_ui->mouseAddButton->setMenu(addMouseProfileMenu);
	m_ui->mouseEnableGesturesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_EnableMouseGesturesOption).toBool());

	updateReaddMouseProfileMenu();

	connect(m_ui->advancedViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::changeCurrentPage);
	connect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateNotificationsActions);
	connect(m_ui->notificationsPlaySoundButton, &QToolButton::clicked, this, &PreferencesAdvancedPageWidget::playNotificationSound);
	connect(m_ui->enableJavaScriptCheckBox, &QCheckBox::toggled, m_ui->javaScriptOptionsButton, &QPushButton::setEnabled);
	connect(m_ui->javaScriptOptionsButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::updateJavaScriptOptions);
	connect(m_ui->downloadsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateDownloadsActions);
	connect(m_ui->downloadsAddMimeTypeButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::addDownloadsMimeType);
	connect(m_ui->downloadsRemoveMimeTypeButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::removeDownloadsMimeType);
	connect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	connect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsMode);
	connect(m_ui->userAgentsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateUserAgentsActions);
	connect(m_ui->userAgentsAddButton->menu(), &QMenu::triggered, this, &PreferencesAdvancedPageWidget::addUserAgent);
	connect(m_ui->userAgentsEditButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::editUserAgent);
	connect(m_ui->userAgentsRemoveButton, &QPushButton::clicked, m_ui->userAgentsViewWidget, &ItemViewWidget::removeRow);
	connect(m_ui->proxiesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateProxiesActions);
	connect(m_ui->proxiesAddButton->menu(), &QMenu::triggered, this, &PreferencesAdvancedPageWidget::addProxy);
	connect(m_ui->proxiesEditButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::editProxy);
	connect(m_ui->proxiesRemoveButton, &QPushButton::clicked, m_ui->proxiesViewWidget, &ItemViewWidget::removeRow);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::canMoveDownChanged, m_ui->ciphersMoveDownButton, &QToolButton::setEnabled);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::canMoveUpChanged, m_ui->ciphersMoveUpButton, &QToolButton::setEnabled);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateCiphersActions);
	connect(m_ui->ciphersAddButton->menu(), &QMenu::triggered, this, &PreferencesAdvancedPageWidget::addCipher);
	connect(m_ui->ciphersRemoveButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::removeCipher);
	connect(m_ui->ciphersMoveDownButton, &QToolButton::clicked, m_ui->ciphersViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->ciphersMoveUpButton, &QToolButton::clicked, m_ui->ciphersViewWidget, &ItemViewWidget::moveUpRow);
	connect(m_ui->updateChannelsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateUpdateChannelsActions);
	connect(m_ui->keyboardViewWidget, &ItemViewWidget::canMoveDownChanged, m_ui->keyboardMoveDownButton, &QToolButton::setEnabled);
	connect(m_ui->keyboardViewWidget, &ItemViewWidget::canMoveUpChanged, m_ui->keyboardMoveUpButton, &QToolButton::setEnabled);
	connect(m_ui->keyboardViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateKeyboardProfileActions);
	connect(m_ui->keyboardViewWidget, &ItemViewWidget::doubleClicked, this, &PreferencesAdvancedPageWidget::editKeyboardProfile);
	connect(m_ui->keyboardAddButton->menu()->actions().at(0), &QAction::triggered, this, &PreferencesAdvancedPageWidget::addKeyboardProfile);
	connect(m_ui->keyboardAddButton->menu()->actions().at(1)->menu(), &QMenu::triggered, this, &PreferencesAdvancedPageWidget::readdKeyboardProfile);
	connect(m_ui->keyboardEditButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::editKeyboardProfile);
	connect(m_ui->keyboardCloneButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::cloneKeyboardProfile);
	connect(m_ui->keyboardRemoveButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::removeKeyboardProfile);
	connect(m_ui->keyboardMoveDownButton, &QToolButton::clicked, m_ui->keyboardViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->keyboardMoveUpButton, &QToolButton::clicked, m_ui->keyboardViewWidget, &ItemViewWidget::moveUpRow);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::canMoveDownChanged, m_ui->mouseMoveDownButton, &QToolButton::setEnabled);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::canMoveUpChanged, m_ui->mouseMoveUpButton, &QToolButton::setEnabled);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateMouseProfileActions);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::doubleClicked, this, &PreferencesAdvancedPageWidget::editMouseProfile);
	connect(m_ui->mouseAddButton->menu()->actions().at(0), &QAction::triggered, this, &PreferencesAdvancedPageWidget::addMouseProfile);
	connect(m_ui->mouseAddButton->menu()->actions().at(1)->menu(), &QMenu::triggered, this, &PreferencesAdvancedPageWidget::readdMouseProfile);
	connect(m_ui->mouseEditButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::editMouseProfile);
	connect(m_ui->mouseCloneButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::cloneMouseProfile);
	connect(m_ui->mouseRemoveButton, &QPushButton::clicked, this, &PreferencesAdvancedPageWidget::removeMouseProfile);
	connect(m_ui->mouseMoveDownButton, &QToolButton::clicked, m_ui->mouseViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->mouseMoveUpButton, &QToolButton::clicked, m_ui->mouseViewWidget, &ItemViewWidget::moveUpRow);
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
		connect(effect, &QSoundEffect::playingChanged, this, [=]()
		{
			if (!effect->isPlaying())
			{
				effect->deleteLater();
			}
		});

		effect->play();
	}
}

void PreferencesAdvancedPageWidget::updateNotificationsActions()
{
	disconnect(m_ui->notificationsPlaySoundFilePathWidget, &FilePathWidget::pathChanged, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);
	disconnect(m_ui->notificationsShowAlertCheckBox, &QCheckBox::clicked, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);
	disconnect(m_ui->notificationsShowNotificationCheckBox, &QCheckBox::clicked, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);

	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));
	const QString path(index.data(SoundPathRole).toString());

	m_ui->notificationOptionsWidget->setEnabled(index.isValid());
	m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
	m_ui->notificationsPlaySoundFilePathWidget->setPath(path);
	m_ui->notificationsShowAlertCheckBox->setChecked(index.data(ShouldShowAlertRole).toBool());
	m_ui->notificationsShowNotificationCheckBox->setChecked(index.data(ShouldShowNotificationRole).toBool());

	connect(m_ui->notificationsPlaySoundFilePathWidget, &FilePathWidget::pathChanged, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);
	connect(m_ui->notificationsShowAlertCheckBox, &QCheckBox::clicked, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);
	connect(m_ui->notificationsShowNotificationCheckBox, &QCheckBox::clicked, this, &PreferencesAdvancedPageWidget::updateNotificationsOptions);
}

void PreferencesAdvancedPageWidget::updateNotificationsOptions()
{
	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));

	if (index.isValid())
	{
		disconnect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateNotificationsActions);

		const QString path(m_ui->notificationsPlaySoundFilePathWidget->getPath());

		m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
		m_ui->notificationsItemView->setData(index, path, SoundPathRole);
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowAlertCheckBox->isChecked(), ShouldShowAlertRole);
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowNotificationCheckBox->isChecked(), ShouldShowNotificationRole);

		connect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateNotificationsActions);
	}
}

void PreferencesAdvancedPageWidget::addDownloadsMimeType()
{
	const QString mimeType(QInputDialog::getText(this, tr("MIME Type Name"), tr("Select name of MIME Type:")));

	if (!mimeType.isEmpty())
	{
		const QModelIndexList indexes(m_ui->downloadsItemView->getSourceModel()->match(m_ui->downloadsItemView->getSourceModel()->index(0, 0), Qt::DisplayRole, mimeType));

		if (!indexes.isEmpty())
		{
			m_ui->downloadsItemView->setCurrentIndex(indexes.first());
		}
		else if (QRegularExpression(QLatin1String("^[a-zA-Z\\-]+/[a-zA-Z0-9\\.\\+\\-_]+$")).match(mimeType).hasMatch())
		{
			QStandardItem *item(new QStandardItem(mimeType));
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

			m_ui->downloadsItemView->insertRow({item});
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
	disconnect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	disconnect(m_ui->downloadsSaveDirectlyCheckBox, &QCheckBox::toggled, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	disconnect(m_ui->downloadsFilePathWidget, &FilePathWidget::pathChanged, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	disconnect(m_ui->downloadsPassUrlCheckBox, &QCheckBox::toggled, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	disconnect(m_ui->downloadsApplicationComboBoxWidget, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);

	const QModelIndex index(m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow()));
	const QString mode(index.data(TransferModeRole).toString());

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
	m_ui->downloadsFilePathWidget->setPath(index.data(DownloadsPathRole).toString());
	m_ui->downloadsApplicationComboBoxWidget->setMimeType(QMimeDatabase().mimeTypeForName(index.data(Qt::DisplayRole).toString()));
	m_ui->downloadsApplicationComboBoxWidget->setCurrentCommand(index.data(OpenCommandRole).toString());

	connect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	connect(m_ui->downloadsSaveDirectlyCheckBox, &QCheckBox::toggled, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	connect(m_ui->downloadsFilePathWidget, &FilePathWidget::pathChanged, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	connect(m_ui->downloadsPassUrlCheckBox, &QCheckBox::toggled, this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
	connect(m_ui->downloadsApplicationComboBoxWidget, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
}

void PreferencesAdvancedPageWidget::updateDownloadsOptions()
{
	disconnect(m_ui->downloadsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateDownloadsActions);
	disconnect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);

	const QModelIndex index(m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow()));

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

		m_ui->downloadsItemView->setData(index, mode, TransferModeRole);
		m_ui->downloadsItemView->setData(index, ((mode == QLatin1String("save") || mode == QLatin1String("saveAs")) ? m_ui->downloadsFilePathWidget->getPath() : QString()), DownloadsPathRole);
		m_ui->downloadsItemView->setData(index, ((mode == QLatin1String("open")) ? m_ui->downloadsApplicationComboBoxWidget->getCommand() : QString()), OpenCommandRole);
	}

	connect(m_ui->downloadsItemView, &ItemViewWidget::needsActionsUpdate, this, &PreferencesAdvancedPageWidget::updateDownloadsActions);
	connect(m_ui->downloadsButtonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &PreferencesAdvancedPageWidget::updateDownloadsOptions);
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
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, {}, &result));

				if (result)
				{
					QList<QStandardItem*> items({new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title), new QStandardItem()});
					items[0]->setData(Utils::createIdentifier({}, QVariant(model->getAllData(UserAgentsModel::IdentifierRole, 0)).toStringList()), UserAgentsModel::IdentifierRole);

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
					userAgent.identifier = Utils::createIdentifier({}, QVariant(model->getAllData(UserAgentsModel::IdentifierRole, 0)).toStringList());

					QList<QStandardItem*> items({new QStandardItem(userAgent.title.isEmpty() ? tr("(Untitled)") : userAgent.title), new QStandardItem(userAgent.value)});
					items[0]->setCheckable(true);
					items[0]->setData(userAgent.identifier, UserAgentsModel::IdentifierRole);

					model->insertRow(items, parent, row, TreeModel::EntryType);
				}
			}

			break;
		case TreeModel::SeparatorType:
			model->insertRow({new QStandardItem(), new QStandardItem()}, parent, row, TreeModel::SeparatorType);

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
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(UserAgentsModel::TitleRole).toString(), &result));

		if (result)
		{
			m_ui->userAgentsViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), UserAgentsModel::TitleRole);
		}
	}
	else if (type == TreeModel::EntryType)
	{
		UserAgentDefinition userAgent;
		userAgent.identifier = index.data(UserAgentsModel::IdentifierRole).toString();
		userAgent.title = index.data(UserAgentsModel::TitleRole).toString();
		userAgent.value = index.sibling(index.row(), 1).data(Qt::DisplayRole).toString();

		UserAgentPropertiesDialog dialog(userAgent, this);

		if (dialog.exec() == QDialog::Accepted)
		{
			userAgent = dialog.getUserAgent();

			m_ui->userAgentsViewWidget->setData(index, (userAgent.title.isEmpty() ? tr("(Untitled)") : userAgent.title), UserAgentsModel::TitleRole);
			m_ui->userAgentsViewWidget->setData(index.sibling(index.row(), 1), userAgent.value, Qt::DisplayRole);
		}
	}
}

void PreferencesAdvancedPageWidget::updateUserAgentsActions()
{
	const QModelIndex index(m_ui->userAgentsViewWidget->getCurrentIndex());
	const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(index.data(TreeModel::TypeRole).toInt()));

	m_ui->userAgentsEditButton->setEnabled(index.isValid() && (type == TreeModel::FolderType || type == TreeModel::EntryType));
	m_ui->userAgentsRemoveButton->setEnabled(index.isValid() && index.data(UserAgentsModel::IdentifierRole).toString() != QLatin1String("default"));
}

void PreferencesAdvancedPageWidget::saveUsuerAgents(QJsonArray *userAgents, const QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		const QStandardItem *item(parent->child(i, 0));

		if (item)
		{
			const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(item->data(TreeModel::TypeRole).toInt()));

			if (type == TreeModel::FolderType || type == TreeModel::EntryType)
			{
				QJsonObject userAgentObject({{QLatin1String("identifier"), item->data(UserAgentsModel::IdentifierRole).toString()}, {QLatin1String("title"), item->data(UserAgentsModel::TitleRole).toString()}});

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
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, {}, &result));

				if (result)
				{
					QStandardItem *item(new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title));
					item->setData(Utils::createIdentifier({}, QVariant(model->getAllData(ProxiesModel::IdentifierRole, 0)).toStringList()), ProxiesModel::IdentifierRole);

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
					proxy.identifier = Utils::createIdentifier({}, QVariant(model->getAllData(ProxiesModel::IdentifierRole, 0)).toStringList());

					m_proxies[proxy.identifier] = proxy;

					QStandardItem *item(new QStandardItem(proxy.title.isEmpty() ? tr("(Untitled)") : proxy.title));
					item->setCheckable(true);
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
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(ProxiesModel::TitleRole).toString(), &result));

		if (result)
		{
			m_ui->proxiesViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), ProxiesModel::TitleRole);
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
			m_ui->proxiesViewWidget->setData(index, (proxy.title.isEmpty() ? tr("(Untitled)") : proxy.title), ProxiesModel::TitleRole);
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

void PreferencesAdvancedPageWidget::saveProxies(QJsonArray *proxies, const QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		const QStandardItem *item(parent->child(i, 0));

		if (item)
		{
			const TreeModel::ItemType type(static_cast<TreeModel::ItemType>(item->data(TreeModel::TypeRole).toInt()));

			if (type == TreeModel::FolderType)
			{
				QJsonArray proxiesArray;

				saveProxies(&proxiesArray, item);

				proxies->append(QJsonObject({{QLatin1String("identifier"), item->data(ProxiesModel::IdentifierRole).toString()}, {QLatin1String("title"), item->data(ProxiesModel::TitleRole).toString()}, {QLatin1String("children"), proxiesArray}}));
			}
			else if (type == TreeModel::EntryType)
			{
				const QString identifier(item->data(ProxiesModel::IdentifierRole).toString());
				const ProxyDefinition proxy(m_proxies.value(identifier, NetworkManagerFactory::getProxy(identifier)));
				QJsonObject proxyObject({{QLatin1String("identifier"), identifier}, {QLatin1String("title"), item->data(ProxiesModel::TitleRole).toString()}});

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

								serversArray.append(QJsonObject({{QLatin1String("protocol"), protocol}, {QLatin1String("hostName"), iterator.value().hostName}, {QLatin1String("port"), iterator.value().port}}));
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
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->ciphersViewWidget->insertRow({item});
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
	const QString identifier(createProfileIdentifier(m_ui->keyboardViewWidget->getSourceModel()));

	if (identifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[identifier] = KeyboardProfile(identifier);

	QStandardItem *item(new QStandardItem(tr("(Untitled)")));
	item->setData(identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->keyboardViewWidget->insertRow({item});
}

void PreferencesAdvancedPageWidget::readdKeyboardProfile(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier(action->data().toString());
	const KeyboardProfile profile(identifier, KeyboardProfile::FullMode);

	if (profile.getName().isEmpty())
	{
		return;
	}

	m_keyboardProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(profile.getTitle()));
	item->setToolTip(profile.getDescription());
	item->setData(profile.getName(), Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->keyboardViewWidget->insertRow({item});

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

	KeyboardProfileDialog dialog(identifier, m_keyboardProfiles, m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked(), this);

	if (dialog.exec() == QDialog::Rejected || !dialog.isModified())
	{
		return;
	}

	const KeyboardProfile profile(dialog.getProfile());

	m_keyboardProfiles[identifier] = profile;

	m_ui->keyboardViewWidget->markAsModified();
	m_ui->keyboardViewWidget->setData(index, profile.getTitle(), Qt::DisplayRole);
	m_ui->keyboardViewWidget->setData(index, profile.getDescription(), Qt::ToolTipRole);
}

void PreferencesAdvancedPageWidget::cloneKeyboardProfile()
{
	const QString identifier(m_ui->keyboardViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier(createProfileIdentifier(m_ui->mouseViewWidget->getSourceModel(), identifier));

	if (newIdentifier.isEmpty())
	{
		return;
	}

	const KeyboardProfile profile(identifier, KeyboardProfile::FullMode);
	KeyboardProfile newProfile(newIdentifier, KeyboardProfile::MetaDataOnlyMode);
	newProfile.setAuthor(profile.getAuthor());
	newProfile.setDefinitions(profile.getDefinitions());
	newProfile.setDescription(profile.getDescription());
	newProfile.setTitle(profile.getTitle());
	newProfile.setVersion(profile.getVersion());

	m_keyboardProfiles[newIdentifier] = newProfile;

	QStandardItem *item(new QStandardItem(newProfile.getTitle()));
	item->setToolTip(newProfile.getDescription());
	item->setData(newIdentifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->keyboardViewWidget->insertRow({item});
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

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".json")));

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
	QVector<KeyboardProfile> availableKeyboardProfiles;
	const QList<QFileInfo> allKeyboardProfiles(QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"))).entryInfoList(QDir::Files) + QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allKeyboardProfiles.count(); ++i)
	{
		const QString identifier(allKeyboardProfiles.at(i).baseName());

		if (!m_keyboardProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const KeyboardProfile profile(identifier, KeyboardProfile::MetaDataOnlyMode);

			if (!profile.getName().isEmpty())
			{
				availableIdentifiers.append(identifier);

				availableKeyboardProfiles.append(profile);
			}
		}
	}

	m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->clear();
	m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->setEnabled(!availableKeyboardProfiles.isEmpty());

	for (int i = 0; i < availableKeyboardProfiles.count(); ++i)
	{
		m_ui->keyboardAddButton->menu()->actions().at(1)->menu()->addAction(availableKeyboardProfiles.at(i).getTitle())->setData(availableKeyboardProfiles.at(i).getName());
	}
}

void PreferencesAdvancedPageWidget::addMouseProfile()
{
	const QString identifier(createProfileIdentifier(m_ui->mouseViewWidget->getSourceModel()));

	if (identifier.isEmpty())
	{
		return;
	}

	m_mouseProfiles[identifier] = MouseProfile(identifier);

	QStandardItem *item(new QStandardItem(tr("(Untitled)")));
	item->setData(identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->mouseViewWidget->insertRow({item});
}

void PreferencesAdvancedPageWidget::readdMouseProfile(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier(action->data().toString());
	const MouseProfile profile(identifier, MouseProfile::FullMode);

	if (profile.getName().isEmpty())
	{
		return;
	}

	m_mouseProfiles[identifier] = profile;

	QStandardItem *item(new QStandardItem(profile.getTitle()));
	item->setToolTip(profile.getDescription());
	item->setData(profile.getName(), Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->mouseViewWidget->insertRow({item});

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

	if (dialog.exec() == QDialog::Rejected || !dialog.isModified())
	{
		return;
	}

	const MouseProfile profile(dialog.getProfile());

	m_mouseProfiles[identifier] = profile;

	m_ui->mouseViewWidget->markAsModified();
	m_ui->mouseViewWidget->setData(index, profile.getTitle(), Qt::DisplayRole);
	m_ui->mouseViewWidget->setData(index, profile.getDescription(), Qt::ToolTipRole);
}

void PreferencesAdvancedPageWidget::cloneMouseProfile()
{
	const QString identifier(m_ui->mouseViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_mouseProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier(createProfileIdentifier(m_ui->mouseViewWidget->getSourceModel(), identifier));

	if (newIdentifier.isEmpty())
	{
		return;
	}

	const MouseProfile profile(identifier, MouseProfile::FullMode);
	MouseProfile newProfile(newIdentifier, MouseProfile::MetaDataOnlyMode);
	newProfile.setAuthor(profile.getAuthor());
	newProfile.setDefinitions(profile.getDefinitions());
	newProfile.setDescription(profile.getDescription());
	newProfile.setTitle(profile.getTitle());
	newProfile.setVersion(profile.getVersion());

	m_mouseProfiles[newIdentifier] = newProfile;

	QStandardItem *item(new QStandardItem(newProfile.getTitle()));
	item->setToolTip(newProfile.getDescription());
	item->setData(newIdentifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->mouseViewWidget->insertRow({item});
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

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("mouse/") + identifier + QLatin1String(".json")));

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
	QVector<MouseProfile> availableMouseProfiles;
	const QList<QFileInfo> allMouseProfiles(QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"))).entryInfoList(QDir::Files) + QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allMouseProfiles.count(); ++i)
	{
		const QString identifier(allMouseProfiles.at(i).baseName());

		if (!m_mouseProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			MouseProfile profile(identifier, MouseProfile::MetaDataOnlyMode);

			if (!profile.getName().isEmpty())
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
		m_ui->mouseAddButton->menu()->actions().at(1)->menu()->addAction((availableMouseProfiles.at(i).getTitle()))->setData(availableMouseProfiles.at(i).getName());
	}
}

void PreferencesAdvancedPageWidget::updateJavaScriptOptions()
{
	const bool isSet(!m_javaScriptOptions.isEmpty());

	if (!isSet)
	{
		const QVector<int> javaScriptOptions({SettingsManager::Permissions_EnableFullScreenOption, SettingsManager::Permissions_ScriptsCanAccessClipboardOption, SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, SettingsManager::Permissions_ScriptsCanCloseWindowsOption, SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption, SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption});

		for (int i = 0; i < javaScriptOptions.count(); ++i)
		{
			m_javaScriptOptions[javaScriptOptions.at(i)] = SettingsManager::getOption(javaScriptOptions.at(i));
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

	SettingsManager::setOption(SettingsManager::Interface_EnableSmoothScrollingOption, m_ui->browsingEnableSmoothScrollingCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Browser_EnableSpellCheckOption, m_ui->browsingEnableSpellCheckCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestBookmarksOption, m_ui->browsingSuggestBookmarksCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestHistoryOption, m_ui->browsingSuggestHistoryCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestLocalPathsOption, m_ui->browsingSuggestLocalPathsCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_ShowCompletionCategoriesOption, m_ui->browsingCategoriesCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_CompletionDisplayModeOption, m_ui->browsingDisplayModeComboBox->currentData(Qt::UserRole).toString());

	QSettings notificationsSettings(SessionsManager::getWritableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);
	notificationsSettings.setIniCodec("UTF-8");
	notificationsSettings.clear();

	for (int i = 0; i < m_ui->notificationsItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->notificationsItemView->getIndex(i, 0));
		const QString eventName(NotificationsManager::getEventName(index.data(IdentifierRole).toInt()));

		if (eventName.isEmpty())
		{
			continue;
		}

		notificationsSettings.beginGroup(eventName);
		notificationsSettings.setValue(QLatin1String("playSound"), index.data(SoundPathRole).toString());
		notificationsSettings.setValue(QLatin1String("showAlert"), index.data(ShouldShowAlertRole).toBool());
		notificationsSettings.setValue(QLatin1String("showNotification"), index.data(ShouldShowNotificationRole).toBool());
		notificationsSettings.endGroup();
	}

	SettingsManager::setOption(SettingsManager::Interface_UseNativeNotificationsOption, m_ui->preferNativeNotificationsCheckBox->isChecked());

	const QString widgetStyle((m_ui->appearranceWidgetStyleComboBox->currentIndex() == 0) ? QString() : m_ui->appearranceWidgetStyleComboBox->currentText());

	if (widgetStyle != SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString())
	{
		Application::setStyle(ThemesManager::createStyle(widgetStyle));
	}

	SettingsManager::setOption(SettingsManager::Interface_WidgetStyleOption, widgetStyle);
	SettingsManager::setOption(SettingsManager::Interface_StyleSheetOption, m_ui->appearranceStyleSheetFilePathWidget->getPath());
	SettingsManager::setOption(SettingsManager::Browser_EnableTrayIconOption, m_ui->enableTrayIconCheckBox->isChecked());

	if (m_ui->appearranceStyleSheetFilePathWidget->getPath().isEmpty())
	{
		Application::getInstance()->setStyleSheet({});
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
			Application::getInstance()->setStyleSheet({});
		}
	}

	SettingsManager::setOption(SettingsManager::Permissions_EnableImagesOption, m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setOption(SettingsManager::Permissions_EnableJavaScriptOption, m_ui->enableJavaScriptCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_EnablePluginsOption, m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setOption(SettingsManager::Content_UserStyleSheetOption, m_ui->userStyleSheetFilePathWidget->getPath());

	IniSettings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
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
		handlersSettings.setValue(QLatin1String("transferMode"), index.data(TransferModeRole).toString());
		handlersSettings.setValue(QLatin1String("downloadsPath"), index.data(DownloadsPathRole).toString());
		handlersSettings.setValue(QLatin1String("openCommand"), index.data(OpenCommandRole).toString());
		handlersSettings.endGroup();
	}

	handlersSettings.save(SessionsManager::getWritableDataPath(QLatin1String("handlers.ini")));

	SettingsManager::setOption(SettingsManager::Network_EnableReferrerOption, m_ui->sendReferrerCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Network_UserAgentOption, m_ui->userAgentsViewWidget->getCheckedIndex().data(UserAgentsModel::IdentifierRole).toString());

	if (m_ui->userAgentsViewWidget->isModified())
	{
		QJsonArray userAgentsArray;

		saveUsuerAgents(&userAgentsArray, m_ui->userAgentsViewWidget->getSourceModel()->invisibleRootItem());

		JsonSettings settings;
		settings.setArray(userAgentsArray);
		settings.save(SessionsManager::getWritableDataPath(QLatin1String("userAgents.json")));

		NetworkManagerFactory::loadUserAgents();
	}

	SettingsManager::setOption(SettingsManager::Network_ProxyOption, m_ui->proxiesViewWidget->getCheckedIndex().data(ProxiesModel::IdentifierRole).toString());

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

		SettingsManager::setOption(SettingsManager::Security_CiphersOption, ciphers);
	}

	SettingsManager::setOption(SettingsManager::Updates_ActiveChannelsOption, getSelectedUpdateChannels());
	SettingsManager::setOption(SettingsManager::Updates_AutomaticInstallOption, m_ui->autoInstallCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Updates_CheckIntervalOption, m_ui->intervalSpinBox->value());

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("keyboard")));

	bool needsKeyboardProfilesReload(false);
	QHash<QString, KeyboardProfile>::iterator keyboardProfilesIterator;

	for (keyboardProfilesIterator = m_keyboardProfiles.begin(); keyboardProfilesIterator != m_keyboardProfiles.end(); ++keyboardProfilesIterator)
	{
		if (keyboardProfilesIterator.value().isModified())
		{
			keyboardProfilesIterator.value().save();

			needsKeyboardProfilesReload = true;
		}
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

	if (needsKeyboardProfilesReload && SettingsManager::getOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList() == keyboardProfiles && SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool() == m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked())
	{
		ActionsManager::loadProfiles();
	}

	SettingsManager::setOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption, keyboardProfiles);
	SettingsManager::setOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption, m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked());

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("mouse")));

	bool needsMouseProfilesReload(false);
	QHash<QString, MouseProfile>::iterator mouseProfilesIterator;

	for (mouseProfilesIterator = m_mouseProfiles.begin(); mouseProfilesIterator != m_mouseProfiles.end(); ++mouseProfilesIterator)
	{
		if (mouseProfilesIterator.value().isModified())
		{
			mouseProfilesIterator.value().save();

			needsMouseProfilesReload = true;
		}
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

	if (needsMouseProfilesReload && SettingsManager::getOption(SettingsManager::Browser_MouseProfilesOrderOption).toStringList() == mouseProfiles && SettingsManager::getOption(SettingsManager::Browser_EnableMouseGesturesOption).toBool() == m_ui->mouseEnableGesturesCheckBox->isChecked())
	{
		GesturesManager::loadProfiles();
	}

	SettingsManager::setOption(SettingsManager::Browser_MouseProfilesOrderOption, mouseProfiles);
	SettingsManager::setOption(SettingsManager::Browser_EnableMouseGesturesOption, m_ui->mouseEnableGesturesCheckBox->isChecked());

	if (!m_javaScriptOptions.isEmpty())
	{
		QHash<int, QVariant>::iterator javaScriptOptionsIterator;

		for (javaScriptOptionsIterator = m_javaScriptOptions.begin(); javaScriptOptionsIterator != m_javaScriptOptions.end(); ++javaScriptOptionsIterator)
		{
			SettingsManager::setOption(javaScriptOptionsIterator.key(), javaScriptOptionsIterator.value());
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

QString PreferencesAdvancedPageWidget::createProfileIdentifier(QStandardItemModel *model, const QString &base) const
{
	QStringList identifiers;

	for (int i = 0; i < model->rowCount(); ++i)
	{
		const QString identifier(model->index(i, 0).data(Qt::UserRole).toString());

		if (!identifier.isEmpty())
		{
			identifiers.append(identifier);
		}
	}

	return Utils::createIdentifier(base, identifiers);
}

QStringList PreferencesAdvancedPageWidget::getSelectedUpdateChannels() const
{
	QStringList updateChannels;

	for (int i = 0; i < m_ui->updateChannelsItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->updateChannelsItemView->getIndex(i, 0));

		if (static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked)
		{
			updateChannels.append(index.data(Qt::UserRole).toString());
		}
	}

	return updateChannels;
}

}
