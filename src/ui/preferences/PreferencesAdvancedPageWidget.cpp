/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "PreferencesAdvancedPageWidget.h"
#include "JavaScriptPreferencesDialog.h"
#include "ShortcutsProfileDialog.h"
#include "../ItemDelegate.h"
#include "../OptionDelegate.h"
#include "../UserAgentsManagerDialog.h"
#include "../../core/ActionsManager.h"
#include "../../core/Application.h"
#include "../../core/NetworkManagerFactory.h"
#include "../../core/NotificationsManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/Settings.h"
#include "../../core/SettingsManager.h"
#include "../../core/Utils.h"

#include "ui_PreferencesAdvancedPageWidget.h"

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

	QStandardItemModel *navigationModel = new QStandardItemModel(this);
	QStringList navigationTitles;
	navigationTitles << tr("Browsing") << tr("Notifications") << tr("Appearance") << tr("Content") << QString() << tr("Downloads") << tr("Programs") << QString() << tr("History") << tr("Network") << tr("Security") << tr("Updates") << QString() << tr("Keyboard") << tr("Mouse");

	int navigationIndex = 0;

	for (int i = 0; i < navigationTitles.count(); ++i)
	{
		QStandardItem *item = new QStandardItem(navigationTitles.at(i));

		if (navigationTitles.at(i).isEmpty())
		{
			item->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
			item->setEnabled(false);
		}
		else
		{
			item->setData(navigationIndex, Qt::UserRole);

			if (i == 6 || i == 8 || i == 14)
			{
				item->setEnabled(false);
			}

			++navigationIndex;
		}

		navigationModel->appendRow(item);
	}

	m_ui->advancedViewWidget->setModel(navigationModel);
	m_ui->advancedViewWidget->setItemDelegate(new ItemDelegate(m_ui->advancedViewWidget));
	m_ui->advancedViewWidget->selectionModel()->select(navigationModel->index(0, 0), QItemSelectionModel::Select);
	m_ui->advancedViewWidget->setMinimumWidth(qMax(100, m_ui->advancedViewWidget->sizeHint().width()));

	m_ui->suggestBookmarksCheckBox->setChecked(SettingsManager::getValue(QLatin1String("AddressField/SuggestBookmarks")).toBool());

	m_ui->notificationsPlaySoundButton->setIcon(Utils::getIcon(QLatin1String("media-playback-start")));

	QStringList notificationsLabels;
	notificationsLabels << tr("Name") << tr("Description");

	QStandardItemModel *notificationsModel = new QStandardItemModel(this);
	notificationsModel->setHorizontalHeaderLabels(notificationsLabels);

	const QVector<EventDefinition> events = NotificationsManager::getEventDefinitions();

	for (int i = 0; i < events.count(); ++i)
	{
		QList<QStandardItem*> items;
		items.append(new QStandardItem(QCoreApplication::translate("notifications", events.at(i).title.toUtf8())));
		items[0]->setData(events.at(i).identifier, Qt::UserRole);
		items[0]->setData(events.at(i).playSound, (Qt::UserRole + 1));
		items[0]->setData(events.at(i).showAlert, (Qt::UserRole + 2));
		items[0]->setData(events.at(i).showNotification, (Qt::UserRole + 3));
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items.append(new QStandardItem(QCoreApplication::translate("notifications", events.at(i).description.toUtf8())));
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		notificationsModel->appendRow(items);
	}

	m_ui->notificationsItemView->setModel(notificationsModel);
	m_ui->preferNativeNotificationsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Interface/UseNativeNotifications")).toBool());

	const QStringList widgetStyles = QStyleFactory::keys();

	m_ui->appearranceWidgetStyleComboBox->addItem(tr("System Style"));

	for (int i = 0; i < widgetStyles.count(); ++i)
	{
		m_ui->appearranceWidgetStyleComboBox->addItem(widgetStyles.at(i));
	}

	m_ui->appearranceWidgetStyleComboBox->setCurrentIndex(qMax(0, m_ui->appearranceWidgetStyleComboBox->findData(SettingsManager::getValue(QLatin1String("Interface/WidgetStyle")).toString(), Qt::DisplayRole)));
	m_ui->appearranceStyleSheetFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Interface/StyleSheet")).toString());
	m_ui->enableTrayIconCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableTrayIcon")).toBool());

	m_ui->enableImagesCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableImages")).toBool());
	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJavaScript")).toBool());
	m_ui->javaScriptOptionsButton->setEnabled(m_ui->enableJavaScriptCheckBox->isChecked());
	m_ui->enableJavaCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableJava")).toBool());

	m_ui->pluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->pluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->pluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	const int pluginsIndex = m_ui->pluginsComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/EnablePlugins")).toString());

	m_ui->pluginsComboBox->setCurrentIndex((pluginsIndex < 0) ? 1 : pluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Content/UserStyleSheet")).toString());

	QStringList downloadsLabels;
	downloadsLabels << tr("Name");

	QStandardItemModel *downloadsModel = new QStandardItemModel(this);
	downloadsModel->setHorizontalHeaderLabels(downloadsLabels);

	Settings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QStringList handlers = handlersSettings.getGroups();

	for (int i = 0; i < handlers.count(); ++i)
	{
		if (handlers.at(i) != QLatin1String("*") && !handlers.at(i).contains(QLatin1Char('/')))
		{
			continue;
		}

		handlersSettings.beginGroup(handlers.at(i));

		QStandardItem *item = new QStandardItem(handlers.at(i));
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

	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Network/EnableReferrer")).toBool());

	const QStringList userAgents = NetworkManagerFactory::getUserAgents();

	m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

	for (int i = 0; i < userAgents.count(); ++i)
	{
		const UserAgentInformation userAgent = NetworkManagerFactory::getUserAgent(userAgents.at(i));
		const QString title = userAgent.title;

		m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i));
		m_ui->userAgentComboBox->setItemData((i + 1), userAgent.value, (Qt::UserRole + 1));
	}

	m_ui->userAgentComboBox->setCurrentIndex(m_ui->userAgentComboBox->findData(SettingsManager::getValue(QLatin1String("Network/UserAgent")).toString()));

	QStandardItemModel *proxyExceptionsModel = new QStandardItemModel(this);
	const QStringList currentProxyExceptions = SettingsManager::getValue(QLatin1String("Proxy/Exceptions")).toStringList();

	for (int i = 0; i < currentProxyExceptions.count(); ++i)
	{
		proxyExceptionsModel->appendRow(new QStandardItem(currentProxyExceptions.at(i)));
	}

	m_ui->proxyExceptionsListView->setModel(proxyExceptionsModel);
	m_ui->proxyExceptionsListView->setItemDelegate(new OptionDelegate(true, this));

	const QString proxyString = SettingsManager::getValue(QLatin1String("Network/ProxyMode")).toString();
	int proxyIndex = 0;

	if (proxyString == QLatin1String("system"))
	{
		proxyIndex = 1;
	}
	else if (proxyString == QLatin1String("manual"))
	{
		proxyIndex = 2;
	}
	else if (proxyString == QLatin1String("automatic"))
	{
		proxyIndex = 3;
	}

	m_ui->proxyModeComboBox->setCurrentIndex(proxyIndex);

	if (proxyIndex == 2 || proxyIndex == 3)
	{
		proxyModeChanged(proxyIndex);
	}

	m_ui->allProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseCommon")).toBool());
	m_ui->httpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttp")).toBool());
	m_ui->httpsProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseHttps")).toBool());
	m_ui->ftpProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseFtp")).toBool());
	m_ui->socksProxyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseSocks")).toBool());
	m_ui->allProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/CommonServers")).toString());
	m_ui->httpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpServers")).toString());
	m_ui->httpsProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/HttpsServers")).toString());
	m_ui->ftpProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/FtpServers")).toString());
	m_ui->socksProxyServersLineEdit->setText(SettingsManager::getValue(QLatin1String("Proxy/SocksServers")).toString());
	m_ui->allProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/CommonPort")).toInt());
	m_ui->httpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpPort")).toInt());
	m_ui->httpsProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/HttpsPort")).toInt());
	m_ui->ftpProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/FtpPort")).toInt());
	m_ui->socksProxyPortSpinBox->setValue(SettingsManager::getValue(QLatin1String("Proxy/SocksPort")).toInt());
	m_ui->automaticProxyConfigurationFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Proxy/AutomaticConfigurationPath")).toString());
	m_ui->proxySystemAuthentication->setChecked(SettingsManager::getValue(QLatin1String("Proxy/UseSystemAuthentication")).toBool());

	m_ui->ciphersAddButton->setMenu(new QMenu(m_ui->ciphersAddButton));

	if (QSslSocket::supportsSsl())
	{
		QStandardItemModel *ciphersModel = new QStandardItemModel(this);
		const bool useDefaultCiphers = (SettingsManager::getValue(QLatin1String("Security/Ciphers")).toString() == QLatin1String("default"));
		const QStringList selectedCiphers = (useDefaultCiphers ? QStringList() : SettingsManager::getValue(QLatin1String("Security/Ciphers")).toStringList());
		const QList<QSslCipher> defaultCiphers = NetworkManagerFactory::getDefaultCiphers();
		const QList<QSslCipher> supportedCiphers = QSslSocket::supportedCiphers();

		for (int i = 0; i < supportedCiphers.count(); ++i)
		{
			if ((useDefaultCiphers && defaultCiphers.contains(supportedCiphers.at(i))) || (!useDefaultCiphers && (selectedCiphers.isEmpty() || selectedCiphers.contains(supportedCiphers.at(i).name()))))
			{
				QStandardItem *item = new QStandardItem(supportedCiphers.at(i).name());
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

				ciphersModel->appendRow(item);
			}
			else
			{
				m_ui->ciphersAddButton->menu()->addAction(supportedCiphers.at(i).name());
			}
		}

		m_ui->ciphersViewWidget->setModel(ciphersModel);
		m_ui->ciphersViewWidget->setItemDelegate(new OptionDelegate(true, this));
		m_ui->ciphersAddButton->setEnabled(m_ui->ciphersAddButton->menu()->actions().count() > 0);
	}
	else
	{
		m_ui->ciphersViewWidget->setEnabled(false);
	}

	m_ui->ciphersMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->ciphersMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *updateChannelsModel = new QStandardItemModel(this);
	const QStringList availableUpdateChannels = SettingsManager::getValue(QLatin1String("Updates/ActiveChannels")).toStringList();

	QMap<QString, QString> defaultChannels;
	defaultChannels[QLatin1String("release")] = tr("Stable version");
	defaultChannels[QLatin1String("beta")] = tr("Beta version");
	defaultChannels[QLatin1String("weekly")] = tr("Weekly development version");

	QMap<QString, QString>::iterator iterator;

	for (iterator = defaultChannels.begin(); iterator != defaultChannels.end(); ++iterator)
	{
		QStandardItem *item = new QStandardItem(iterator.value());
		item->setData(iterator.key(), Qt::UserRole);
		item->setCheckable(true);

		if (availableUpdateChannels.contains(iterator.key()))
		{
			item->setCheckState(Qt::Checked);
		}

		updateChannelsModel->appendRow(item);
	}

	m_ui->updateChannelsItemView->setModel(updateChannelsModel);
	m_ui->updateChannelsItemView->setItemDelegate(new OptionDelegate(true, this));
	m_ui->updateChannelsItemView->setHeaderHidden(true);

	m_ui->autoInstallCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Updates/AutomaticInstall")).toBool());
	m_ui->intervalSpinBox->setValue(SettingsManager::getValue(QLatin1String("Updates/CheckInterval")).toInt());

	updateUpdateChannelsActions();

	m_ui->keyboardMoveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->keyboardMoveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));

	QStandardItemModel *keyboardProfilesModel = new QStandardItemModel(this);
	const QStringList keyboardProfiles = SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder")).toStringList();

	for (int i = 0; i < keyboardProfiles.count(); ++i)
	{
		const ShortcutsProfile profile = loadKeyboardProfile(keyboardProfiles.at(i), true);

		if (profile.identifier.isEmpty())
		{
			continue;
		}

		m_keyboardProfiles[keyboardProfiles.at(i)] = profile;

		QStandardItem *item = new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title);
		item->setToolTip(profile.description);
		item->setData(profile.identifier, Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

		keyboardProfilesModel->appendRow(item);
	}

	m_ui->keyboardViewWidget->setModel(keyboardProfilesModel);
	m_ui->keyboardViewWidget->setItemDelegate(new OptionDelegate(true, this));

	QMenu *addKeyboardProfileMenu = new QMenu(m_ui->keyboardAddButton);
	addKeyboardProfileMenu->addAction(tr("New…"));
	addKeyboardProfileMenu->addAction(tr("Readd"))->setMenu(new QMenu(m_ui->keyboardAddButton));

	m_ui->keyboardAddButton->setMenu(addKeyboardProfileMenu);
	m_ui->keyboardEnableSingleKeyShortcutsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/EnableSingleKeyShortcuts")).toBool());

	updateReaddKeyboardProfileMenu();

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
	connect(m_ui->proxyExceptionsListView, SIGNAL(needsActionsUpdate()), this, SLOT(updateProxyExceptionsActions()));
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
	connect(m_ui->keyboardViewWidget, SIGNAL(modified()), this, SIGNAL(settingsModified()));
	connect(m_ui->keyboardAddButton->menu()->actions().at(0), SIGNAL(triggered()), this, SLOT(addKeyboardProfile()));
	connect(m_ui->keyboardAddButton->menu()->actions().at(1)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(readdKeyboardProfile(QAction*)));
	connect(m_ui->keyboardEditButton, SIGNAL(clicked()), this, SLOT(editKeyboardProfile()));
	connect(m_ui->keyboardCloneButton, SIGNAL(clicked()), this, SLOT(cloneKeyboardProfile()));
	connect(m_ui->keyboardRemoveButton, SIGNAL(clicked()), this, SLOT(removeKeyboardProfile()));
	connect(m_ui->keyboardMoveDownButton, SIGNAL(clicked()), m_ui->keyboardViewWidget, SLOT(moveDownRow()));
	connect(m_ui->keyboardMoveUpButton, SIGNAL(clicked()), m_ui->keyboardViewWidget, SLOT(moveUpRow()));
}

PreferencesAdvancedPageWidget::~PreferencesAdvancedPageWidget()
{
	delete m_ui;
}

void PreferencesAdvancedPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);
			m_ui->advancedViewWidget->setMinimumWidth(qMax(100, m_ui->advancedViewWidget->sizeHint().width()));

			break;
		default:
			break;
	}
}

void PreferencesAdvancedPageWidget::playNotificationSound()
{
	QSoundEffect *effect = new QSoundEffect(this);
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
	disconnect(m_ui->notificationsPlaySoundFilePathWidget, SIGNAL(pathChanged()), this, SLOT(updateNotificationsOptions()));
	disconnect(m_ui->notificationsShowAlertCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
	disconnect(m_ui->notificationsShowNotificationCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));

	const QModelIndex index = m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow());

	m_ui->notificationOptionsWidget->setEnabled(index.isValid());
	m_ui->notificationsPlaySoundFilePathWidget->setPath(index.data(Qt::UserRole + 1).toString());
	m_ui->notificationsPlaySoundFilePathWidget->setFilter(tr("WAV files (*.wav)"));
	m_ui->notificationsShowAlertCheckBox->setChecked(index.data(Qt::UserRole + 2).toBool());
	m_ui->notificationsShowNotificationCheckBox->setChecked(index.data(Qt::UserRole + 3).toBool());

	connect(m_ui->notificationsPlaySoundFilePathWidget, SIGNAL(pathChanged()), this, SLOT(updateNotificationsOptions()));
	connect(m_ui->notificationsShowAlertCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
	connect(m_ui->notificationsShowNotificationCheckBox, SIGNAL(clicked()), this, SLOT(updateNotificationsOptions()));
}

void PreferencesAdvancedPageWidget::updateNotificationsOptions()
{
	const QModelIndex index = m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow());

	if (index.isValid())
	{
		connect(m_ui->notificationsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateNotificationsActions()));

		m_ui->notificationsItemView->setData(index, m_ui->notificationsPlaySoundFilePathWidget->getPath(), (Qt::UserRole + 1));
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowAlertCheckBox->isChecked(), (Qt::UserRole + 2));
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowNotificationCheckBox->isChecked(), (Qt::UserRole + 3));

		emit settingsModified();

		connect(m_ui->notificationsItemView, SIGNAL(needsActionsUpdate()), this, SLOT(updateNotificationsActions()));
	}
}

void PreferencesAdvancedPageWidget::addDownloadsMimeType()
{
	const QString mimeType = QInputDialog::getText(this, tr("MIME Type Name"), tr("Select name of MIME Type:"));

	if (!mimeType.isEmpty())
	{
		if (QRegularExpression("^[a-zA-Z\\-]+/[a-zA-Z0-9\\.\\+\\-_]+$").match(mimeType).hasMatch())
		{
			QStandardItem *item = new QStandardItem(mimeType);
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
	const QModelIndex index = m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow());

	if (index.isValid() && index.data(Qt::DisplayRole).toString() != QLatin1String("*"))
	{
		m_ui->downloadsItemView->removeRow();
	}
}

void PreferencesAdvancedPageWidget::updateDownloadsActions()
{
	disconnect(m_ui->downloadsButtonGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsSaveDirectlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsFilePathWidget, SIGNAL(pathChanged()), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsPassUrlCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	disconnect(m_ui->downloadsApplicationComboBoxWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDownloadsOptions()));

	const QModelIndex index = m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow());
	const QString mode = index.data(Qt::UserRole).toString();

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
	connect(m_ui->downloadsFilePathWidget, SIGNAL(pathChanged()), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsPassUrlCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateDownloadsOptions()));
	connect(m_ui->downloadsApplicationComboBoxWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDownloadsOptions()));
}

void PreferencesAdvancedPageWidget::updateDownloadsOptions()
{
	const QModelIndex index = m_ui->downloadsItemView->getIndex(m_ui->downloadsItemView->getCurrentRow());

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
	QList<UserAgentInformation> userAgents;

	for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
	{
		UserAgentInformation userAgent;
		userAgent.identifier = m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString();
		userAgent.title = m_ui->userAgentComboBox->itemText(i);
		userAgent.value = m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString();

		userAgents.append(userAgent);
	}

	const QString selectedUserAgent = m_ui->userAgentComboBox->currentData().toString();

	UserAgentsManagerDialog dialog(userAgents, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_userAgentsModified = true;

		m_ui->userAgentComboBox->clear();

		userAgents = dialog.getUserAgents();

		m_ui->userAgentComboBox->addItem(tr("Default"), QLatin1String("default"));

		for (int i = 0; i < userAgents.count(); ++i)
		{
			const QString title = userAgents.at(i).title;

			m_ui->userAgentComboBox->addItem((title.isEmpty() ? tr("(Untitled)") : title), userAgents.at(i).identifier);
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
	m_ui->proxyExceptionsListView->insertRow();

	editProxyException();
}

void PreferencesAdvancedPageWidget::editProxyException()
{
	m_ui->proxyExceptionsListView->edit(m_ui->proxyExceptionsListView->getIndex(m_ui->proxyExceptionsListView->getCurrentRow()));

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::removeProxyException()
{
	m_ui->proxyExceptionsListView->removeRow();
	m_ui->proxyExceptionsListView->setFocus();

	updateProxyExceptionsActions();

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::updateProxyExceptionsActions()
{
	const bool isEditable = (m_ui->proxyExceptionsListView->getCurrentRow() >= 0);

	m_ui->editProxyExceptionButton->setEnabled(isEditable);
	m_ui->removeProxyExceptionButton->setEnabled(isEditable);
}

void PreferencesAdvancedPageWidget::addCipher(QAction *action)
{
	if (!action)
	{
		return;
	}

	QStandardItem *item = new QStandardItem(action->text());
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->ciphersViewWidget->insertRow(item);
	m_ui->ciphersAddButton->menu()->removeAction(action);
	m_ui->ciphersAddButton->setEnabled(m_ui->ciphersAddButton->menu()->actions().count() > 0);
}

void PreferencesAdvancedPageWidget::removeCipher()
{
	const int currentRow = m_ui->ciphersViewWidget->getCurrentRow();

	if (currentRow >= 0)
	{
		m_ui->ciphersAddButton->menu()->addAction(m_ui->ciphersViewWidget->getIndex(currentRow, 0).data(Qt::DisplayRole).toString());
		m_ui->ciphersAddButton->setEnabled(true);

		m_ui->ciphersViewWidget->removeRow();
	}
}

void PreferencesAdvancedPageWidget::updateCiphersActions()
{
	const int currentRow = m_ui->ciphersViewWidget->getCurrentRow();

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
	const QString identifier = createProfileIdentifier(m_ui->keyboardViewWidget);

	if (identifier.isEmpty())
	{
		return;
	}

	ShortcutsProfile profile;
	profile.title = tr("(Untitled)");

	m_keyboardProfiles[identifier] = profile;

	QStandardItem *item = new QStandardItem(tr("(Untitled)"));
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

	const QString identifier = action->data().toString();
	const ShortcutsProfile profile = loadKeyboardProfile(identifier, true);

	if (profile.identifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[identifier] = profile;

	QStandardItem *item = new QStandardItem(profile.title.isEmpty() ? tr("(Untitled)") : profile.title);
	item->setToolTip(profile.description);
	item->setData(profile.identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->keyboardViewWidget->insertRow(item);

	updateReaddKeyboardProfileMenu();

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::editKeyboardProfile()
{
	const QModelIndex index = m_ui->keyboardViewWidget->currentIndex();
	const QString identifier = index.data(Qt::UserRole).toString();

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	ShortcutsProfileDialog dialog(identifier, m_keyboardProfiles, this);

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
	const QString identifier = m_ui->keyboardViewWidget->currentIndex().data(Qt::UserRole).toString();

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier = createProfileIdentifier(m_ui->keyboardViewWidget, identifier);

	if (newIdentifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[newIdentifier] = m_keyboardProfiles[identifier];
	m_keyboardProfiles[newIdentifier].identifier = newIdentifier;
	m_keyboardProfiles[newIdentifier].isModified = true;

	QStandardItem *item = new QStandardItem(m_keyboardProfiles[newIdentifier].title.isEmpty() ? tr("(Untitled)") : m_keyboardProfiles[newIdentifier].title);
	item->setToolTip(m_keyboardProfiles[newIdentifier].description);
	item->setData(newIdentifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_ui->keyboardViewWidget->insertRow(item);

	emit settingsModified();
}

void PreferencesAdvancedPageWidget::removeKeyboardProfile()
{
	const QString identifier = m_ui->keyboardViewWidget->currentIndex().data(Qt::UserRole).toString();

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

	const QString path = SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".ini"));

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
	const int currentRow = m_ui->keyboardViewWidget->getCurrentRow();
	const bool isSelected = (currentRow >= 0 && currentRow < m_ui->keyboardViewWidget->getRowCount());

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
	QList<ShortcutsProfile> availableShortcutsProfiles;
	QList<QFileInfo> allShortcutsProfiles = QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"))).entryInfoList(QDir::Files);
	allShortcutsProfiles.append(QDir(SessionsManager::getReadableDataPath(QLatin1String("keyboard"), true)).entryInfoList(QDir::Files));

	for (int i = 0; i < allShortcutsProfiles.count(); ++i)
	{
		const QString identifier = allShortcutsProfiles.at(i).baseName();

		if (!m_keyboardProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const ShortcutsProfile profile = loadKeyboardProfile(identifier, false);

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

void PreferencesAdvancedPageWidget::updateJavaScriptOptions()
{
	const bool isSet = !m_javaScriptOptions.isEmpty();

	if (!isSet)
	{
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"));
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanShowStatusMessages")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"));
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanAccessClipboard")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanAccessClipboard"));
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanDisableContextMenu")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanDisableContextMenu"));
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanOpenWindows")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanOpenWindows"));
		m_javaScriptOptions[QLatin1String("Browser/JavaScriptCanCloseWindows")] = SettingsManager::getValue(QLatin1String("Browser/JavaScriptCanCloseWindows"));
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
	const QModelIndex index = m_ui->advancedViewWidget->currentIndex();

	if (index.isValid() && index.data(Qt::UserRole).type() == QVariant::Int)
	{
		m_ui->advancedStackedWidget->setCurrentIndex(index.data(Qt::UserRole).toInt());
	}
}

void PreferencesAdvancedPageWidget::save()
{
	for (int i = 0; i < m_filesToRemove.count(); ++i)
	{
		QFile::remove(m_filesToRemove.at(i));
	}

	m_filesToRemove.clear();

	SettingsManager::setValue(QLatin1String("AddressField/SuggestBookmarks"), m_ui->suggestBookmarksCheckBox->isChecked());

	QSettings notificationsSettings(SessionsManager::getWritableDataPath(QLatin1String("notifications.ini")), QSettings::IniFormat);
	notificationsSettings.setIniCodec("UTF-8");
	notificationsSettings.clear();

	for (int i = 0; i < m_ui->notificationsItemView->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->notificationsItemView->getIndex(i, 0);
		const QString eventName = NotificationsManager::getEventName(index.data(Qt::UserRole).toInt());

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

	SettingsManager::setValue(QLatin1String("Interface/UseNativeNotifications"), m_ui->preferNativeNotificationsCheckBox->isChecked());

	const QString widgetStyle = ((m_ui->appearranceWidgetStyleComboBox->currentIndex() == 0) ? QString() : m_ui->appearranceWidgetStyleComboBox->currentText());

	SettingsManager::setValue(QLatin1String("Interface/WidgetStyle"), widgetStyle);
	SettingsManager::setValue(QLatin1String("Interface/StyleSheet"), m_ui->appearranceStyleSheetFilePathWidget->getPath());
	SettingsManager::setValue(QLatin1String("Browser/EnableTrayIcon"), m_ui->enableTrayIconCheckBox->isChecked());

	Application::setStyle(widgetStyle);

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

	SettingsManager::setValue(QLatin1String("Browser/EnableImages"), m_ui->enableImagesCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/EnableJavaScript"), m_ui->enableJavaScriptCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/EnableJava"), m_ui->enableJavaCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/EnablePlugins"), m_ui->pluginsComboBox->currentData(Qt::UserRole).toString());

	SettingsManager::setValue(QLatin1String("Content/UserStyleSheet"), m_ui->userStyleSheetFilePathWidget->getPath());

	Settings handlersSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QStringList handlers = handlersSettings.getGroups();

	for (int i = 0; i < handlers.count(); ++i)
	{
		if (handlers.at(i).contains(QLatin1Char('/')))
		{
			handlersSettings.removeGroup(handlers.at(i));
		}
	}

	for (int i = 0; i < m_ui->downloadsItemView->getRowCount(); ++i)
	{
		const QModelIndex index = m_ui->downloadsItemView->getIndex(i, 0);

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

	SettingsManager::setValue(QLatin1String("Network/EnableReferrer"), m_ui->sendReferrerCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Network/UserAgent"), m_ui->userAgentComboBox->currentData().toString());

	const int proxyIndex = m_ui->proxyModeComboBox->currentIndex();
	QLatin1String proxyString = QLatin1String("noproxy");

	if (proxyIndex == 1)
	{
		proxyString = QLatin1String("system");
	}
	else if (proxyIndex == 2)
	{
		proxyString = QLatin1String("manual");
	}
	else if (proxyIndex == 3)
	{
		proxyString = QLatin1String("automatic");
	}

	SettingsManager::setValue(QLatin1String("Network/ProxyMode"), proxyString);

	if (!m_ui->allProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseCommon"), m_ui->allProxyCheckBox->isChecked());
	}

	if (!m_ui->httpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseHttp"), m_ui->httpProxyCheckBox->isChecked());
	}

	if (!m_ui->httpsProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseHttps"), m_ui->httpsProxyCheckBox->isChecked());
	}

	if (!m_ui->ftpProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseFtp"), m_ui->ftpProxyCheckBox->isChecked());
	}

	if (!m_ui->socksProxyServersLineEdit->text().isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Proxy/UseSocks"), m_ui->socksProxyCheckBox->isChecked());
	}

	SettingsManager::setValue(QLatin1String("Proxy/CommonServers"), m_ui->allProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpServers"), m_ui->httpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsServers"), m_ui->httpsProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/FtpServers"), m_ui->ftpProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/SocksServers"), m_ui->socksProxyServersLineEdit->text());
	SettingsManager::setValue(QLatin1String("Proxy/CommonPort"), m_ui->allProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpPort"), m_ui->httpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/HttpsPort"), m_ui->httpsProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/FtpPort"), m_ui->ftpProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/SocksPort"), m_ui->socksProxyPortSpinBox->value());
	SettingsManager::setValue(QLatin1String("Proxy/AutomaticConfigurationPath"), m_ui->automaticProxyConfigurationFilePathWidget->getPath());
	SettingsManager::setValue(QLatin1String("Proxy/UseSystemAuthentication"), m_ui->proxySystemAuthentication->isChecked());

	QStandardItemModel *proxyListModel = m_ui->proxyExceptionsListView->getModel();
	QStringList proxyExceptions;

	for (int i = 0; i < proxyListModel->rowCount(); ++i)
	{
		const QString value = proxyListModel->item(i)->text();

		if (!value.isEmpty())
		{
			proxyExceptions.append(value);
		}
	}

	SettingsManager::setValue(QLatin1String("Proxy/Exceptions"), proxyExceptions);

	if (m_userAgentsModified)
	{
		QSettings userAgents(SessionsManager::getWritableDataPath(QLatin1String("userAgents.ini")), QSettings::IniFormat);
		userAgents.setIniCodec("UTF-8");
		userAgents.clear();

		for (int i = 1; i < m_ui->userAgentComboBox->count(); ++i)
		{
			userAgents.setValue(m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString() + "/title", m_ui->userAgentComboBox->itemText(i));
			userAgents.setValue(m_ui->userAgentComboBox->itemData(i, Qt::UserRole).toString() + "/value", m_ui->userAgentComboBox->itemData(i, (Qt::UserRole + 1)).toString());
		}

		userAgents.sync();

		NetworkManagerFactory::loadUserAgents();
	}

	if (m_ui->ciphersViewWidget->isModified())
	{
		QStringList ciphers;

		for (int i = 0; i < m_ui->ciphersViewWidget->getRowCount(); ++i)
		{
			ciphers.append(m_ui->ciphersViewWidget->getIndex(i, 0).data(Qt::DisplayRole).toString());
		}

		SettingsManager::setValue(QLatin1String("Security/Ciphers"), ciphers);
	}

	SettingsManager::setValue(QLatin1String("Updates/ActiveChannels"), getSelectedUpdateChannels());
	SettingsManager::setValue(QLatin1String("Updates/AutomaticInstall"), m_ui->autoInstallCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Updates/CheckInterval"), m_ui->intervalSpinBox->value());

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("keyboard")));

	QHash<QString, ShortcutsProfile>::iterator keyboardProfilesIterator;

	for (keyboardProfilesIterator = m_keyboardProfiles.begin(); keyboardProfilesIterator != m_keyboardProfiles.end(); ++keyboardProfilesIterator)
	{
		if (!keyboardProfilesIterator.value().isModified)
		{
			continue;
		}

		QFile file(SessionsManager::getWritableDataPath(QLatin1String("keyboard/") + keyboardProfilesIterator.key() + QLatin1String(".ini")));

		if (!file.open(QIODevice::WriteOnly))
		{
			continue;
		}

		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream << QLatin1String("; Title: ") << (keyboardProfilesIterator.value().title.isEmpty() ? tr("(Untitled)") : keyboardProfilesIterator.value().title) << QLatin1Char('\n');
		stream << QLatin1String("; Description: ") << keyboardProfilesIterator.value().description << QLatin1Char('\n');
		stream << QLatin1String("; Type: keyboard-profile\n");
		stream << QLatin1String("; Author: ") << keyboardProfilesIterator.value().author << QLatin1Char('\n');
		stream << QLatin1String("; Version: ") << keyboardProfilesIterator.value().version << QLatin1String("\n\n");

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

				stream << QLatin1Char('[') << ActionsManager::getActionName(shortcutsIterator.key()) << QLatin1String("]\n");
				stream << QLatin1String("shortcuts=") << shortcuts.join(QLatin1Char(' ')) << QLatin1String("\n\n");
			}
		}

		file.close();
	}

	QStringList keyboardProfiles;

	for (int i = 0; i < m_ui->keyboardViewWidget->getRowCount(); ++i)
	{
		const QString identifier = m_ui->keyboardViewWidget->getIndex(i, 0).data(Qt::UserRole).toString();

		if (!identifier.isEmpty())
		{
			keyboardProfiles.append(identifier);
		}
	}

	SettingsManager::setValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder"), keyboardProfiles);
	SettingsManager::setValue(QLatin1String("Browser/EnableSingleKeyShortcuts"), m_ui->keyboardEnableSingleKeyShortcutsCheckBox->isChecked());

	if (!m_javaScriptOptions.isEmpty())
	{
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanChangeWindowGeometry")));
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanShowStatusMessages"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanShowStatusMessages")));
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanAccessClipboard"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanAccessClipboard")));
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanDisableContextMenu"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanDisableContextMenu")));
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanOpenWindows"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanOpenWindows")));
		SettingsManager::setValue(QLatin1String("Browser/JavaScriptCanCloseWindows"), m_javaScriptOptions.value(QLatin1String("Browser/JavaScriptCanCloseWindows")));
	}

	updateReaddKeyboardProfileMenu();
}

QString PreferencesAdvancedPageWidget::createProfileIdentifier(ItemViewWidget *view, const QString &base) const
{
	QStringList identifiers;

	for (int i = 0; i < view->getRowCount(); ++i)
	{
		const QString identifier = view->getIndex(i, 0).data(Qt::UserRole).toString();

		if (!identifier.isEmpty())
		{
			identifiers.append(identifier);
		}
	}

	return Utils::createIdentifier(base, identifiers);
}

QStringList PreferencesAdvancedPageWidget::getSelectedUpdateChannels() const
{
	QStandardItemModel *updateListModel = m_ui->updateChannelsItemView->getModel();
	QStringList updateChannels;

	for (int i = 0; i < updateListModel->rowCount(); ++i)
	{
		QStandardItem *item = updateListModel->item(i);

		if (item->checkState() == Qt::Checked)
		{
			updateChannels.append(item->data(Qt::UserRole).toString());
		}
	}

	return updateChannels;
}

ShortcutsProfile PreferencesAdvancedPageWidget::loadKeyboardProfile(const QString &identifier, bool loadShortcuts) const
{
	const QString path = SessionsManager::getReadableDataPath(QLatin1String("keyboard/") + identifier + QLatin1String(".ini"));
	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		return ShortcutsProfile();
	}

	const QRegularExpression expression(QLatin1String(";\\s*"));
	ShortcutsProfile profile;
	profile.identifier = identifier;

	while (!file.atEnd())
	{
		const QString line = QString(file.readLine()).trimmed();

		if (!line.startsWith(QLatin1Char(';')))
		{
			break;
		}

		const QString key = line.section(QLatin1Char(':'), 0, 0).remove(expression);
		const QString value = line.section(QLatin1Char(':'), 1).trimmed();

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

	file.close();

	if (!loadShortcuts)
	{
		return profile;
	}

	QSettings settings(path, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	const QStringList actions = settings.childGroups();

	for (int i = 0; i < actions.count(); ++i)
	{
		const int action = ActionsManager::getActionIdentifier(actions.at(i));

		if (action < 0)
		{
			continue;
		}

		settings.beginGroup(actions.at(i));

		const QStringList rawShortcuts = settings.value(QLatin1String("shortcuts")).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
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

}
