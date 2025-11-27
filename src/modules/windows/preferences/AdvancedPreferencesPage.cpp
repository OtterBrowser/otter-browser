/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AdvancedPreferencesPage.h"
#include "../../../core/Application.h"
#include "../../../core/GesturesManager.h"
#include "../../../core/HandlersManager.h"
#include "../../../core/IniSettings.h"
#include "../../../core/JsonSettings.h"
#include "../../../core/NotificationsManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/preferences/MouseProfileDialog.h"
#include "../../../ui/preferences/ProxyPropertiesDialog.h"
#include "../../../ui/preferences/UserAgentPropertiesDialog.h"
#include "../../../ui/Style.h"

#include "ui_AdvancedPreferencesPage.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMimeDatabase>
#include <QtMultimedia/QSoundEffect>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslCipher>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>

namespace Otter
{

AdvancedPreferencesPage::AdvancedPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_ui(nullptr)
{
}

AdvancedPreferencesPage::~AdvancedPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void AdvancedPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (!wasLoaded())
	{
		return;
	}

	switch (event->type())
	{
		case QEvent::FontChange:
		case QEvent::StyleChange:
			updateStyle();

			break;
		case QEvent::LanguageChange:
			{
				const QStringList navigationTitles({tr("Browsing"), tr("Notifications"), tr("Appearance"), {}, tr("Downloads"), tr("Programs"), {}, tr("History"), tr("Network"), tr("Scripting"), tr("Security"), tr("Updates"), {}, tr("Mouse")});

				m_ui->retranslateUi(this);
				m_ui->appearranceWidgetStyleComboBox->setItemText(0, tr("System Style"));
				m_ui->notificationsPlaySoundFilePathWidget->setFilters({tr("WAV files (*.wav)")});
				m_ui->appearranceStyleSheetFilePathWidget->setFilters({tr("Style sheets (*.css)")});
				m_ui->notificationsItemView->getSourceModel()->setHorizontalHeaderLabels({tr("Name"), tr("Description")});
				m_ui->scriptsCanCloseWindowsComboBox->setItemText(0, tr("Ask"));
				m_ui->scriptsCanCloseWindowsComboBox->setItemText(1, tr("Always"));
				m_ui->scriptsCanCloseWindowsComboBox->setItemText(2, tr("Never"));
				m_ui->enableFullScreenComboBox->setItemText(0, tr("Ask"));
				m_ui->enableFullScreenComboBox->setItemText(1, tr("Always"));
				m_ui->enableFullScreenComboBox->setItemText(2, tr("Never"));

				for (int i = 0; i < navigationTitles.count(); ++i)
				{
					const QString title(navigationTitles.at(i));

					if (!title.isEmpty())
					{
						m_ui->advancedViewWidget->setData(m_ui->advancedViewWidget->getIndex(i), title, Qt::DisplayRole);
					}
				}

				updatePageSwitcher();
			}

			break;
		default:
			break;
	}
}

void AdvancedPreferencesPage::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (wasLoaded())
	{
		updatePageSwitcher();
	}
}

void AdvancedPreferencesPage::playNotificationSound()
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

void AdvancedPreferencesPage::updateNotificationsActions()
{
	m_ui->notificationsPlaySoundFilePathWidget->blockSignals(true);
	m_ui->notificationsShowAlertCheckBox->blockSignals(true);
	m_ui->notificationsShowNotificationCheckBox->blockSignals(true);
	m_ui->preferNativeNotificationsCheckBox->blockSignals(true);

	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));
	const QString path(index.data(SoundPathRole).toString());

	m_ui->notificationOptionsWidget->setEnabled(index.isValid());
	m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
	m_ui->notificationsPlaySoundFilePathWidget->setPath(path);
	m_ui->notificationsShowAlertCheckBox->setChecked(index.data(ShouldShowAlertRole).toBool());
	m_ui->notificationsShowNotificationCheckBox->setChecked(index.data(ShouldShowNotificationRole).toBool());
	m_ui->notificationsPlaySoundFilePathWidget->blockSignals(false);
	m_ui->notificationsShowAlertCheckBox->blockSignals(false);
	m_ui->notificationsShowNotificationCheckBox->blockSignals(false);
	m_ui->preferNativeNotificationsCheckBox->blockSignals(false);
}

void AdvancedPreferencesPage::updateNotificationsOptions()
{
	const QModelIndex index(m_ui->notificationsItemView->getIndex(m_ui->notificationsItemView->getCurrentRow()));

	if (index.isValid())
	{
		disconnect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateNotificationsActions);

		const QString path(m_ui->notificationsPlaySoundFilePathWidget->getPath());

		m_ui->notificationsPlaySoundButton->setEnabled(!path.isEmpty() && QFile::exists(path));
		m_ui->notificationsItemView->setData(index, path, SoundPathRole);
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowAlertCheckBox->isChecked(), ShouldShowAlertRole);
		m_ui->notificationsItemView->setData(index, m_ui->notificationsShowNotificationCheckBox->isChecked(), ShouldShowNotificationRole);

		connect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateNotificationsActions);
	}
}

void AdvancedPreferencesPage::addDownloadsMimeType()
{
	const QString mimeType(QInputDialog::getText(this, tr("MIME Type Name"), tr("Select name of MIME Type:")));

	if (mimeType.isEmpty())
	{
		return;
	}

	const QModelIndexList indexes(m_ui->mimeTypesItemView->getSourceModel()->match(m_ui->mimeTypesItemView->getSourceModel()->index(0, 0), Qt::DisplayRole, mimeType));

	if (!indexes.isEmpty())
	{
		m_ui->mimeTypesItemView->setCurrentIndex(indexes.value(0));
	}
	else if (QRegularExpression(QLatin1String(R"(^[a-zA-Z\-]+/[a-zA-Z0-9\.\+\-_]+$)")).match(mimeType).hasMatch())
	{
		QStandardItem *item(new QStandardItem(mimeType));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		m_ui->mimeTypesItemView->insertRow({item});
		m_ui->mimeTypesItemView->sortByColumn(0, Qt::AscendingOrder);
	}
	else
	{
		QMessageBox::critical(this, tr("Error"), tr("Invalid MIME Type name."));
	}
}

void AdvancedPreferencesPage::removeDownloadsMimeType()
{
	const QModelIndex index(m_ui->mimeTypesItemView->getIndex(m_ui->mimeTypesItemView->getCurrentRow()));

	if (index.isValid() && index.data(Qt::DisplayRole).toString() != QLatin1String("*"))
	{
		m_ui->mimeTypesItemView->removeRow();
	}
}

void AdvancedPreferencesPage::updateDownloadsActions()
{
	m_ui->mimeTypesButtonGroup->blockSignals(true);
	m_ui->mimeTypesAskButton->blockSignals(true);
	m_ui->mimeTypesSaveButton->blockSignals(true);
	m_ui->mimeTypesOpenButton->blockSignals(true);
	m_ui->mimeTypesSaveDirectlyCheckBox->blockSignals(true);
	m_ui->mimeTypesFilePathWidget->blockSignals(true);
	m_ui->mimeTypesPassUrlCheckBox->blockSignals(true);
	m_ui->mimeTypesApplicationComboBoxWidget->blockSignals(true);

	const QModelIndex index(m_ui->mimeTypesItemView->getIndex(m_ui->mimeTypesItemView->getCurrentRow()));
	const HandlersManager::MimeTypeHandlerDefinition::TransferMode mode(static_cast<HandlersManager::MimeTypeHandlerDefinition::TransferMode>(index.data(TransferModeRole).toInt()));

	switch (mode)
	{
		case HandlersManager::MimeTypeHandlerDefinition::SaveTransfer:
		case HandlersManager::MimeTypeHandlerDefinition::SaveAsTransfer:
			m_ui->mimeTypesSaveButton->setChecked(true);

			break;
		case HandlersManager::MimeTypeHandlerDefinition::OpenTransfer:
			m_ui->mimeTypesOpenButton->setChecked(true);

			break;
		default:
			m_ui->mimeTypesAskButton->setChecked(true);

			break;
	}

	m_ui->mimeTypesRemoveMimeTypeButton->setEnabled(index.isValid() && index.data(Qt::DisplayRole).toString() != QLatin1String("*"));
	m_ui->mimeTypesOptionsWidget->setEnabled(index.isValid());
	m_ui->mimeTypesSaveDirectlyCheckBox->setChecked(mode == HandlersManager::MimeTypeHandlerDefinition::SaveTransfer);
	m_ui->mimeTypesFilePathWidget->setPath(index.data(DownloadsPathRole).toString());
	m_ui->mimeTypesApplicationComboBoxWidget->setMimeType(QMimeDatabase().mimeTypeForName(index.data(Qt::DisplayRole).toString()));
	m_ui->mimeTypesApplicationComboBoxWidget->setCurrentCommand(index.data(OpenCommandRole).toString());

	updateDownloadsMode();

	m_ui->mimeTypesButtonGroup->blockSignals(false);
	m_ui->mimeTypesAskButton->blockSignals(false);
	m_ui->mimeTypesSaveButton->blockSignals(false);
	m_ui->mimeTypesOpenButton->blockSignals(false);
	m_ui->mimeTypesSaveDirectlyCheckBox->blockSignals(false);
	m_ui->mimeTypesFilePathWidget->blockSignals(false);
	m_ui->mimeTypesPassUrlCheckBox->blockSignals(false);
	m_ui->mimeTypesApplicationComboBoxWidget->blockSignals(false);
}

void AdvancedPreferencesPage::updateDownloadsOptions()
{
	const QModelIndex index(m_ui->mimeTypesItemView->getIndex(m_ui->mimeTypesItemView->getCurrentRow()));

	if (!index.isValid())
	{
		return;
	}

	disconnect(m_ui->mimeTypesItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateDownloadsActions);
	disconnect(m_ui->mimeTypesButtonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled), this, &AdvancedPreferencesPage::updateDownloadsOptions);

	HandlersManager::MimeTypeHandlerDefinition::TransferMode mode(HandlersManager::MimeTypeHandlerDefinition::IgnoreTransfer);

	if (m_ui->mimeTypesSaveButton->isChecked())
	{
		mode = (m_ui->mimeTypesSaveDirectlyCheckBox->isChecked() ? HandlersManager::MimeTypeHandlerDefinition::SaveTransfer : HandlersManager::MimeTypeHandlerDefinition::SaveAsTransfer);
	}
	else if (m_ui->mimeTypesOpenButton->isChecked())
	{
		mode = HandlersManager::MimeTypeHandlerDefinition::OpenTransfer;
	}
	else
	{
		mode = HandlersManager::MimeTypeHandlerDefinition::AskTransfer;
	}

	m_ui->mimeTypesItemView->setData(index, mode, TransferModeRole);
	m_ui->mimeTypesItemView->setData(index, ((mode == HandlersManager::MimeTypeHandlerDefinition::SaveTransfer || mode == HandlersManager::MimeTypeHandlerDefinition::SaveAsTransfer) ? m_ui->mimeTypesFilePathWidget->getPath() : QString()), DownloadsPathRole);
	m_ui->mimeTypesItemView->setData(index, ((mode == HandlersManager::MimeTypeHandlerDefinition::OpenTransfer) ? m_ui->mimeTypesApplicationComboBoxWidget->getCommand() : QString()), OpenCommandRole);

	connect(m_ui->mimeTypesItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateDownloadsActions);
	connect(m_ui->mimeTypesButtonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled), this, &AdvancedPreferencesPage::updateDownloadsOptions);
}

void AdvancedPreferencesPage::updateDownloadsMode()
{
	m_ui->mimeTypesOpenOptionsWidget->setEnabled(m_ui->mimeTypesOpenButton->isChecked());
	m_ui->mimeTypesSaveOptionsWidget->setEnabled(m_ui->mimeTypesSaveButton->isChecked());
}

void AdvancedPreferencesPage::addUserAgent(QAction *action)
{
	if (!action)
	{
		return;
	}

	ItemModel *model(qobject_cast<ItemModel*>(m_ui->userAgentsViewWidget->getSourceModel()));

	if (!model)
	{
		return;
	}

	QStandardItem *parent(model->itemFromIndex(m_ui->userAgentsViewWidget->getCurrentIndex()));

	if (!parent)
	{
		parent = model->invisibleRootItem();
	}

	int row(-1);

	if (static_cast<ItemModel::ItemType>(parent->data(ItemModel::TypeRole).toInt()) != ItemModel::FolderType)
	{
		row = (parent->row() + 1);

		parent = parent->parent();
	}

	switch (static_cast<ItemModel::ItemType>(action->data().toInt()))
	{
		case ItemModel::FolderType:
			{
				bool result(false);
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, {}, &result));

				if (result)
				{
					QList<QStandardItem*> items({new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title), new QStandardItem()});
					items[0]->setData(items[0]->data(Qt::DisplayRole), Qt::ToolTipRole);
					items[0]->setData(Utils::createIdentifier({}, QVariant(model->getAllData(UserAgentsModel::IdentifierRole, 0)).toStringList()), UserAgentsModel::IdentifierRole);

					model->insertRow(items, parent, row, ItemModel::FolderType);

					m_ui->userAgentsViewWidget->expand(items[0]->index());
				}
			}

			break;
		case ItemModel::EntryType:
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
					items[0]->setData(items[0]->data(Qt::DisplayRole), Qt::ToolTipRole);
					items[0]->setData(userAgent.identifier, UserAgentsModel::IdentifierRole);
					items[1]->setData(items[1]->data(Qt::DisplayRole), Qt::ToolTipRole);

					model->insertRow(items, parent, row, ItemModel::EntryType);
				}
			}

			break;
		case ItemModel::SeparatorType:
			model->insertRow({new QStandardItem(), new QStandardItem()}, parent, row, ItemModel::SeparatorType);

			break;
		default:
			break;
	}
}

void AdvancedPreferencesPage::editUserAgent()
{
	const QModelIndex index(m_ui->userAgentsViewWidget->getCurrentIndex());

	if (!index.isValid())
	{
		return;
	}

	const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(index.data(ItemModel::TypeRole).toInt()));

	if (type == ItemModel::FolderType)
	{
		bool result(false);
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(UserAgentsModel::TitleRole).toString(), &result));

		if (result)
		{
			m_ui->userAgentsViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), UserAgentsModel::TitleRole);
			m_ui->userAgentsViewWidget->setData(index, index.data(Qt::DisplayRole), Qt::ToolTipRole);
		}
	}
	else if (type == ItemModel::EntryType)
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
			m_ui->userAgentsViewWidget->setData(index, index.data(Qt::DisplayRole), Qt::ToolTipRole);
			m_ui->userAgentsViewWidget->setData(index.sibling(index.row(), 1), userAgent.value, Qt::DisplayRole);
			m_ui->userAgentsViewWidget->setData(index.sibling(index.row(), 1), userAgent.value, Qt::ToolTipRole);
		}
	}
}

void AdvancedPreferencesPage::updateUserAgentsActions()
{
	const QModelIndex index(m_ui->userAgentsViewWidget->getCurrentIndex());
	const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(index.data(ItemModel::TypeRole).toInt()));

	m_ui->userAgentsEditButton->setEnabled(index.isValid() && (type == ItemModel::FolderType || type == ItemModel::EntryType));
	m_ui->userAgentsRemoveButton->setEnabled(index.isValid() && index.data(UserAgentsModel::IdentifierRole).toString() != QLatin1String("default"));
}

void AdvancedPreferencesPage::saveUserAgents(QJsonArray *userAgents, const QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		const QStandardItem *item(parent->child(i, 0));

		if (!item)
		{
			continue;
		}

		const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(item->data(ItemModel::TypeRole).toInt()));

		if (type == ItemModel::FolderType || type == ItemModel::EntryType)
		{
			QJsonObject userAgentObject({{QLatin1String("identifier"), item->data(UserAgentsModel::IdentifierRole).toString()}, {QLatin1String("title"), item->data(UserAgentsModel::TitleRole).toString()}});

			if (type == ItemModel::FolderType)
			{
				QJsonArray userAgentsArray;

				saveUserAgents(&userAgentsArray, item);

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

void AdvancedPreferencesPage::addProxy(QAction *action)
{
	if (!action)
	{
		return;
	}

	ItemModel *model(qobject_cast<ItemModel*>(m_ui->proxiesViewWidget->getSourceModel()));

	if (!model)
	{
		return;
	}

	QStandardItem *parent(model->itemFromIndex(m_ui->proxiesViewWidget->getCurrentIndex()));

	if (!parent)
	{
		parent = model->invisibleRootItem();
	}

	int row(-1);

	if (static_cast<ItemModel::ItemType>(parent->data(ItemModel::TypeRole).toInt()) != ItemModel::FolderType)
	{
		row = (parent->row() + 1);

		parent = parent->parent();
	}

	switch (static_cast<ItemModel::ItemType>(action->data().toInt()))
	{
		case ItemModel::FolderType:
			{
				bool result(false);
				const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, {}, &result));

				if (result)
				{
					QStandardItem *item(new QStandardItem(title.isEmpty() ? tr("(Untitled)") : title));
					item->setData(item->data(Qt::DisplayRole), Qt::ToolTipRole);
					item->setData(Utils::createIdentifier({}, QVariant(model->getAllData(ProxiesModel::IdentifierRole, 0)).toStringList()), ProxiesModel::IdentifierRole);

					model->insertRow(item, parent, row, ItemModel::FolderType);

					m_ui->proxiesViewWidget->expand(item->index());
				}
			}

			break;
		case ItemModel::EntryType:
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
					item->setData(item->data(Qt::DisplayRole), Qt::ToolTipRole);
					item->setData(proxy.identifier, ProxiesModel::IdentifierRole);

					model->insertRow(item, parent, row, ItemModel::EntryType);
				}
			}

			break;
		case ItemModel::SeparatorType:
			model->insertRow(new QStandardItem(), parent, row, ItemModel::SeparatorType);

			break;
		default:
			break;
	}
}

void AdvancedPreferencesPage::editProxy()
{
	const QModelIndex index(m_ui->proxiesViewWidget->getCurrentIndex());

	if (!index.isValid())
	{
		return;
	}

	const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(index.data(ItemModel::TypeRole).toInt()));

	if (type == ItemModel::FolderType)
	{
		bool result(false);
		const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select folder name:"), QLineEdit::Normal, index.data(ProxiesModel::TitleRole).toString(), &result));

		if (result)
		{
			m_ui->proxiesViewWidget->setData(index, (title.isEmpty() ? tr("(Untitled)") : title), ProxiesModel::TitleRole);
			m_ui->proxiesViewWidget->setData(index, index.data(Qt::DisplayRole), Qt::ToolTipRole);
		}
	}
	else if (type == ItemModel::EntryType)
	{
		const QString identifier(index.data(ProxiesModel::IdentifierRole).toString());
		ProxyPropertiesDialog dialog((m_proxies.value(identifier, NetworkManagerFactory::getProxy(identifier))), this);

		if (dialog.exec() == QDialog::Accepted)
		{
			const ProxyDefinition proxy(dialog.getProxy());

			m_proxies[identifier] = proxy;

			m_ui->proxiesViewWidget->markAsModified();
			m_ui->proxiesViewWidget->setData(index, (proxy.title.isEmpty() ? tr("(Untitled)") : proxy.title), ProxiesModel::TitleRole);
			m_ui->proxiesViewWidget->setData(index, index.data(Qt::DisplayRole), Qt::ToolTipRole);
		}
	}
}

void AdvancedPreferencesPage::updateProxiesActions()
{
	const QModelIndex index(m_ui->proxiesViewWidget->getCurrentIndex());
	const QString identifier(index.data(ProxiesModel::IdentifierRole).toString());
	const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(index.data(ItemModel::TypeRole).toInt()));
	const bool isReadOnly(identifier == QLatin1String("noProxy") || identifier == QLatin1String("system"));

	m_ui->proxiesEditButton->setEnabled(index.isValid() && !isReadOnly && (type == ItemModel::FolderType || type == ItemModel::EntryType));
	m_ui->proxiesRemoveButton->setEnabled(index.isValid() && !isReadOnly);
}

void AdvancedPreferencesPage::saveProxies(QJsonArray *proxies, const QStandardItem *parent)
{
	for (int i = 0; i < parent->rowCount(); ++i)
	{
		const QStandardItem *item(parent->child(i, 0));

		if (!item)
		{
			continue;
		}

		const ItemModel::ItemType type(static_cast<ItemModel::ItemType>(item->data(ItemModel::TypeRole).toInt()));

		if (type == ItemModel::FolderType)
		{
			QJsonArray proxiesArray;

			saveProxies(&proxiesArray, item);

			proxies->append(QJsonObject({{QLatin1String("identifier"), item->data(ProxiesModel::IdentifierRole).toString()}, {QLatin1String("title"), item->data(ProxiesModel::TitleRole).toString()}, {QLatin1String("children"), proxiesArray}}));
		}
		else if (type == ItemModel::EntryType)
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

void AdvancedPreferencesPage::updateCiphersActions()
{
	const int currentRow(m_ui->ciphersViewWidget->getCurrentRow());

	m_ui->ciphersRemoveButton->setEnabled(currentRow >= 0 && currentRow < m_ui->ciphersViewWidget->getRowCount());
}

void AdvancedPreferencesPage::updateUpdateChannelsActions()
{
	bool hasSelectedUpdateChannels(false);

	for (int i = 0; i < m_ui->updateChannelsItemView->getRowCount(); ++i)
	{
		if (static_cast<Qt::CheckState>(m_ui->updateChannelsItemView->getIndex(i, 0).data(Qt::CheckStateRole).toInt()) == Qt::Checked)
		{
			hasSelectedUpdateChannels = true;

			break;
		}
	}

	m_ui->intervalSpinBox->setEnabled(hasSelectedUpdateChannels);
	m_ui->autoInstallCheckBox->setEnabled(hasSelectedUpdateChannels);
}

void AdvancedPreferencesPage::addMouseProfile()
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

void AdvancedPreferencesPage::readdMouseProfile(QAction *action)
{
	if (!action || action->data().isNull())
	{
		return;
	}

	const QString identifier(action->data().toString());
	const MouseProfile profile(identifier, MouseProfile::FullMode);

	if (!profile.isValid())
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

void AdvancedPreferencesPage::editMouseProfile()
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

void AdvancedPreferencesPage::cloneMouseProfile()
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

void AdvancedPreferencesPage::removeMouseProfile()
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

void AdvancedPreferencesPage::updateMouseProfileActions()
{
	const int currentRow(m_ui->mouseViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->mouseViewWidget->getRowCount());

	m_ui->mouseEditButton->setEnabled(isSelected);
	m_ui->mouseCloneButton->setEnabled(isSelected);
	m_ui->mouseRemoveButton->setEnabled(isSelected);
}

void AdvancedPreferencesPage::updateReaddMouseProfileMenu()
{
	if (!m_ui->mouseAddButton->menu())
	{
		return;
	}

	QStringList availableIdentifiers;
	QVector<MouseProfile> availableMouseProfiles;
	const QList<QFileInfo> allMouseProfiles(QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"))).entryInfoList({QLatin1String("*.json")}, QDir::Files) + QDir(SessionsManager::getReadableDataPath(QLatin1String("mouse"), true)).entryInfoList({QLatin1String("*.json")}, QDir::Files));

	for (int i = 0; i < allMouseProfiles.count(); ++i)
	{
		const QString identifier(allMouseProfiles.at(i).baseName());

		if (!m_mouseProfiles.contains(identifier) && !availableIdentifiers.contains(identifier))
		{
			const MouseProfile profile(identifier, MouseProfile::MetaDataOnlyMode);

			if (profile.isValid())
			{
				availableIdentifiers.append(identifier);

				availableMouseProfiles.append(profile);
			}
		}
	}

	if (!availableIdentifiers.contains(QLatin1String("default")) && !m_mouseProfiles.contains(QLatin1String("default")))
	{
		availableMouseProfiles.prepend(MouseProfile(QLatin1String("default"), MouseProfile::MetaDataOnlyMode));
	}

	QMenu *readdMenu(m_ui->mouseAddButton->menu()->actions().at(1)->menu());
	readdMenu->clear();
	readdMenu->setEnabled(!availableMouseProfiles.isEmpty());

	for (int i = 0; i < availableMouseProfiles.count(); ++i)
	{
		const MouseProfile profile(availableMouseProfiles.at(i));

		readdMenu->addAction(profile.getTitle())->setData(profile.getName());
	}
}

void AdvancedPreferencesPage::updateStyle()
{
	m_ui->updateChannelsItemView->setMaximumHeight(m_ui->updateChannelsItemView->getContentsHeight());
	m_ui->notificationsItemView->setMaximumHeight(m_ui->notificationsItemView->getContentsHeight());
}

void AdvancedPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::AdvancedPreferencesPage();
	m_ui->setupUi(this);

	QStandardItemModel *navigationModel(new QStandardItemModel(this));
	const QStringList navigationTitles({tr("Browsing"), tr("Notifications"), tr("Appearance"), {}, tr("Downloads"), tr("Programs"), {}, tr("History"), tr("Network"), tr("Scripting"), tr("Security"), tr("Updates"), {}, tr("Mouse")});
	int navigationIndex(0);

	for (int i = 0; i < navigationTitles.count(); ++i)
	{
		const QString title(navigationTitles.at(i));
		QStandardItem *item(new QStandardItem(title));
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		if (title.isEmpty())
		{
			item->setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
			item->setEnabled(false);
		}
		else
		{
			item->setData(navigationIndex, Qt::UserRole);

			if (i == 7)
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

	if (SettingsManager::getOption(SettingsManager::AddressField_CompletionDisplayModeOption).toString() == QLatin1String("columns"))
	{
		m_ui->browsingDisplayModeColumnsRadioButton->setChecked(true);
	}
	else
	{
		m_ui->browsingDisplayModeCompactRadioButton->setChecked(true);
	}

	m_ui->browsingCategoriesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::AddressField_ShowCompletionCategoriesOption).toBool());

	m_ui->notificationsPlaySoundButton->setIcon(ThemesManager::createIcon(QLatin1String("media-playback-start")));
	m_ui->notificationsPlaySoundFilePathWidget->setFilters({tr("WAV files (*.wav)")});

	QStandardItemModel *notificationsModel(new QStandardItemModel(this));
	notificationsModel->setHorizontalHeaderLabels({tr("Name"), tr("Description")});

	const QVector<NotificationsManager::EventDefinition> events(NotificationsManager::getEventDefinitions());

	for (int i = 0; i < events.count(); ++i)
	{
		const NotificationsManager::EventDefinition event(events.at(i));
		QList<QStandardItem*> items({new QStandardItem(event.getTitle()), new QStandardItem(event.getDescription())});
		items[0]->setData(event.identifier, IdentifierRole);
		items[0]->setData(event.playSound, SoundPathRole);
		items[0]->setData(event.showAlert, ShouldShowAlertRole);
		items[0]->setData(event.showNotification, ShouldShowNotificationRole);
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

	QStandardItemModel *mimeTypesModel(new QStandardItemModel(this));
	mimeTypesModel->setHorizontalHeaderLabels({tr("Name")});

	const QVector<HandlersManager::MimeTypeHandlerDefinition> handlers(HandlersManager::getMimeTypeHandlers());

	for (int i = 0; i < handlers.count(); ++i)
	{
		const HandlersManager::MimeTypeHandlerDefinition handler(handlers.at(i));
		QStandardItem *item(new QStandardItem(handler.mimeType.isValid() ? handler.mimeType.name() : QLatin1String("*")));
		item->setData(handler.transferMode, TransferModeRole);
		item->setData(handler.downloadsPath, DownloadsPathRole);
		item->setData(handler.openCommand, OpenCommandRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		mimeTypesModel->appendRow(item);
	}

	m_ui->mimeTypesItemView->setModel(mimeTypesModel);
	m_ui->mimeTypesItemView->sortByColumn(0, Qt::AscendingOrder);
	m_ui->mimeTypesFilePathWidget->setOpenMode(FilePathWidget::ExistingDirectoryMode);
	m_ui->mimeTypesApplicationComboBoxWidget->setAlwaysShowDefaultApplication(true);

	QStandardItemModel *protocolsModel(new QStandardItemModel(this));
	protocolsModel->setHorizontalHeaderLabels({tr("Name")});

	m_ui->protocolsItemView->setModel(protocolsModel);
	m_ui->protocolsItemView->sortByColumn(0, Qt::AscendingOrder);

	m_ui->enableJavaScriptCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_EnableJavaScriptOption).toBool());
	m_ui->javaScriptWidget->setEnabled(m_ui->enableJavaScriptCheckBox->isChecked());
	m_ui->scriptsCanChangeWindowGeometryCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption).toBool());
	m_ui->scriptsCanShowStatusMessagesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption).toBool());
	m_ui->scriptsCanAccessClipboardCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption).toBool());
	m_ui->scriptsCanReceiveRightClicksCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption).toBool());
	m_ui->scriptsCanCloseWindowsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->scriptsCanCloseWindowsComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->scriptsCanCloseWindowsComboBox->addItem(tr("Never"), QLatin1String("disallow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->enableFullScreenComboBox->addItem(tr("Always"), QLatin1String("allow"));
	m_ui->enableFullScreenComboBox->addItem(tr("Never"), QLatin1String("disallow"));

	const int scriptsCanCloseWindowsIndex(m_ui->scriptsCanCloseWindowsComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption).toString()));

	m_ui->scriptsCanCloseWindowsComboBox->setCurrentIndex((scriptsCanCloseWindowsIndex < 0) ? 0 : scriptsCanCloseWindowsIndex);

	const int enableFullScreenIndex(m_ui->enableFullScreenComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_EnableFullScreenOption).toString()));

	m_ui->enableFullScreenComboBox->setCurrentIndex((enableFullScreenIndex < 0) ? 0 : enableFullScreenIndex);

	m_ui->sendReferrerCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Network_EnableReferrerOption).toBool());

	ItemModel *userAgentsModel(new UserAgentsModel(SettingsManager::getOption(SettingsManager::Network_UserAgentOption).toString(), true, this));
	userAgentsModel->setHorizontalHeaderLabels({tr("Title"), tr("Value")});

	m_ui->userAgentsViewWidget->setModel(userAgentsModel);
	m_ui->userAgentsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->userAgentsViewWidget->setExclusive(true);
	m_ui->userAgentsViewWidget->setRowsMovable(true);
	m_ui->userAgentsViewWidget->expandAll();

	QMenu *addUserAgentMenu(new QMenu(m_ui->userAgentsAddButton));
	addUserAgentMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(ItemModel::FolderType);
	addUserAgentMenu->addAction(tr("Add User Agent…"))->setData(ItemModel::EntryType);
	addUserAgentMenu->addAction(tr("Add Separator"))->setData(ItemModel::SeparatorType);

	m_ui->userAgentsAddButton->setMenu(addUserAgentMenu);

	ItemModel *proxiesModel(new ProxiesModel(SettingsManager::getOption(SettingsManager::Network_ProxyOption).toString(), true, this));
	proxiesModel->setHorizontalHeaderLabels({tr("Title")});

	m_ui->proxiesViewWidget->setModel(proxiesModel);
	m_ui->proxiesViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->proxiesViewWidget->setExclusive(true);
	m_ui->proxiesViewWidget->setRowsMovable(true);
	m_ui->proxiesViewWidget->expandAll();

	QMenu *addProxyMenu(new QMenu(m_ui->proxiesAddButton));
	addProxyMenu->addAction(ThemesManager::createIcon(QLatin1String("inode-directory")), tr("Add Folder…"))->setData(ItemModel::FolderType);
	addProxyMenu->addAction(tr("Add Proxy…"))->setData(ItemModel::EntryType);
	addProxyMenu->addAction(tr("Add Separator"))->setData(ItemModel::SeparatorType);

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
		const QList<QSslCipher> supportedCiphers(QSslConfiguration::supportedCiphers());

		for (int i = 0; i < supportedCiphers.count(); ++i)
		{
			const QString cipherName(supportedCiphers.at(i).name());

			if (useDefaultCiphers && defaultCiphers.contains(supportedCiphers.at(i)))
			{
				QStandardItem *item(new QStandardItem(cipherName));
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

				ciphersModel->appendRow(item);
			}
			else if (!selectedCiphers.contains(cipherName))
			{
				m_ui->ciphersAddButton->menu()->addAction(cipherName);
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

	m_ui->ciphersViewWidget->setRowsMovable(true);
	m_ui->ciphersMoveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->ciphersMoveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *updateChannelsModel(new QStandardItemModel(this));
	const QStringList activeUpdateChannels(SettingsManager::getOption(SettingsManager::Updates_ActiveChannelsOption).toStringList());
	const QVector<QPair<QString, QString> > updateChannels({{QLatin1String("release"), tr("Stable version")}, {QLatin1String("beta"), tr("Beta version")}, {QLatin1String("weekly"), tr("Weekly development version")}});

	for (int i = 0; i < updateChannels.count(); ++i)
	{
		const QPair<QString, QString> updateChannel(updateChannels.at(i));
		QStandardItem *item(new QStandardItem(updateChannel.second));
		item->setCheckable(true);
		item->setCheckState(activeUpdateChannels.contains(updateChannel.first) ? Qt::Checked : Qt::Unchecked);
		item->setData(updateChannel.first, Qt::UserRole);
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		updateChannelsModel->appendRow(item);
	}

	m_ui->updateChannelsItemView->setModel(updateChannelsModel);

	m_ui->autoInstallCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Updates_AutomaticInstallOption).toBool());
	m_ui->intervalSpinBox->setValue(SettingsManager::getOption(SettingsManager::Updates_CheckIntervalOption).toInt());

	updateUpdateChannelsActions();

	m_ui->mouseMoveDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->mouseMoveUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *mouseProfilesModel(new QStandardItemModel(this));
	const QStringList mouseProfiles(SettingsManager::getOption(SettingsManager::Browser_MouseProfilesOrderOption).toStringList());

	for (int i = 0; i < mouseProfiles.count(); ++i)
	{
		const MouseProfile profile(mouseProfiles.at(i), MouseProfile::FullMode);

		if (!profile.isValid())
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
	m_ui->mouseViewWidget->setRowsMovable(true);

	QMenu *addMouseProfileMenu(new QMenu(m_ui->mouseAddButton));
	addMouseProfileMenu->addAction(tr("New…"), this, &AdvancedPreferencesPage::addMouseProfile);
	addMouseProfileMenu->addAction(tr("Re-add"))->setMenu(new QMenu(m_ui->mouseAddButton));

	m_ui->mouseAddButton->setMenu(addMouseProfileMenu);
	m_ui->mouseEnableGesturesCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Browser_EnableMouseGesturesOption).toBool());

	updateReaddMouseProfileMenu();
	updateStyle();

	connect(m_ui->advancedViewWidget, &ItemViewWidget::needsActionsUpdate, this, [&]()
	{
		const QModelIndex index(m_ui->advancedViewWidget->currentIndex());

		if (index.isValid() && index.data(Qt::UserRole).type() == QVariant::Int)
		{
			m_ui->advancedStackedWidget->setCurrentIndex(index.data(Qt::UserRole).toInt());
		}
	});
	connect(m_ui->notificationsItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateNotificationsActions);
	connect(m_ui->notificationsPlaySoundButton, &QToolButton::clicked, this, &AdvancedPreferencesPage::playNotificationSound);
	connect(m_ui->notificationsPlaySoundFilePathWidget, &FilePathWidget::pathChanged, this, &AdvancedPreferencesPage::updateNotificationsOptions);
	connect(m_ui->notificationsShowNotificationCheckBox, &QCheckBox::toggled, this, &AdvancedPreferencesPage::updateNotificationsOptions);
	connect(m_ui->notificationsShowAlertCheckBox, &QCheckBox::toggled, this, &AdvancedPreferencesPage::updateNotificationsOptions);
	connect(m_ui->preferNativeNotificationsCheckBox, &QCheckBox::toggled, this, &AdvancedPreferencesPage::updateNotificationsOptions);
	connect(m_ui->mimeTypesItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateDownloadsActions);
	connect(m_ui->mimeTypesAddMimeTypeButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::addDownloadsMimeType);
	connect(m_ui->mimeTypesRemoveMimeTypeButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::removeDownloadsMimeType);
	connect(m_ui->mimeTypesButtonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled), this, &AdvancedPreferencesPage::updateDownloadsOptions);
	connect(m_ui->mimeTypesButtonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled), this, &AdvancedPreferencesPage::updateDownloadsMode);
	connect(m_ui->mimeTypesSaveDirectlyCheckBox, &QCheckBox::toggled, this, &AdvancedPreferencesPage::updateDownloadsOptions);
	connect(m_ui->mimeTypesFilePathWidget, &Otter::FilePathWidget::pathChanged, this, &AdvancedPreferencesPage::updateDownloadsOptions);
	connect(m_ui->mimeTypesApplicationComboBoxWidget, &Otter::ApplicationComboBoxWidget::currentCommandChanged, this, &AdvancedPreferencesPage::updateDownloadsOptions);
	connect(m_ui->enableJavaScriptCheckBox, &QCheckBox::toggled, m_ui->javaScriptWidget, &QWidget::setEnabled);
	connect(m_ui->userAgentsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateUserAgentsActions);
	connect(m_ui->userAgentsAddButton->menu(), &QMenu::triggered, this, &AdvancedPreferencesPage::addUserAgent);
	connect(m_ui->userAgentsEditButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::editUserAgent);
	connect(m_ui->userAgentsRemoveButton, &QPushButton::clicked, m_ui->userAgentsViewWidget, &ItemViewWidget::removeRow);
	connect(m_ui->proxiesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateProxiesActions);
	connect(m_ui->proxiesAddButton->menu(), &QMenu::triggered, this, &AdvancedPreferencesPage::addProxy);
	connect(m_ui->proxiesEditButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::editProxy);
	connect(m_ui->proxiesRemoveButton, &QPushButton::clicked, m_ui->proxiesViewWidget, &ItemViewWidget::removeRow);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->ciphersMoveDownButton, &QToolButton::setEnabled);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->ciphersMoveUpButton, &QToolButton::setEnabled);
	connect(m_ui->ciphersViewWidget, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateCiphersActions);
	connect(m_ui->ciphersAddButton->menu(), &QMenu::triggered, this, [&](QAction *action)
	{
		QStandardItem *item(new QStandardItem(action->text()));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

		m_ui->ciphersViewWidget->insertRow({item});
		m_ui->ciphersAddButton->menu()->removeAction(action);
		m_ui->ciphersAddButton->setEnabled(m_ui->ciphersAddButton->menu()->actions().count() > 0);
	});
	connect(m_ui->ciphersRemoveButton, &QPushButton::clicked, this, [&]()
	{
		const int currentRow(m_ui->ciphersViewWidget->getCurrentRow());

		if (currentRow >= 0)
		{
			m_ui->ciphersAddButton->menu()->addAction(m_ui->ciphersViewWidget->getIndex(currentRow, 0).data(Qt::DisplayRole).toString());
			m_ui->ciphersAddButton->setEnabled(true);

			m_ui->ciphersViewWidget->removeRow();
		}
	});
	connect(m_ui->ciphersMoveDownButton, &QToolButton::clicked, m_ui->ciphersViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->ciphersMoveUpButton, &QToolButton::clicked, m_ui->ciphersViewWidget, &ItemViewWidget::moveUpRow);
	connect(m_ui->updateChannelsItemView, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateUpdateChannelsActions);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->mouseMoveDownButton, &QToolButton::setEnabled);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->mouseMoveUpButton, &QToolButton::setEnabled);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::needsActionsUpdate, this, &AdvancedPreferencesPage::updateMouseProfileActions);
	connect(m_ui->mouseViewWidget, &ItemViewWidget::doubleClicked, this, &AdvancedPreferencesPage::editMouseProfile);
	connect(m_ui->mouseAddButton->menu()->actions().at(1)->menu(), &QMenu::triggered, this, &AdvancedPreferencesPage::readdMouseProfile);
	connect(m_ui->mouseEditButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::editMouseProfile);
	connect(m_ui->mouseCloneButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::cloneMouseProfile);
	connect(m_ui->mouseRemoveButton, &QPushButton::clicked, this, &AdvancedPreferencesPage::removeMouseProfile);
	connect(m_ui->mouseMoveDownButton, &QToolButton::clicked, m_ui->mouseViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->mouseMoveUpButton, &QToolButton::clicked, m_ui->mouseViewWidget, &ItemViewWidget::moveUpRow);

	markAsLoaded();
}

void AdvancedPreferencesPage::save()
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	Utils::removeFiles(m_filesToRemove);

	m_filesToRemove.clear();

	SettingsManager::setOption(SettingsManager::Interface_EnableSmoothScrollingOption, m_ui->browsingEnableSmoothScrollingCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Browser_EnableSpellCheckOption, m_ui->browsingEnableSpellCheckCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestBookmarksOption, m_ui->browsingSuggestBookmarksCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestHistoryOption, m_ui->browsingSuggestHistoryCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_SuggestLocalPathsOption, m_ui->browsingSuggestLocalPathsCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::AddressField_CompletionDisplayModeOption, (m_ui->browsingDisplayModeColumnsRadioButton->isChecked() ? QLatin1String("columns") : QLatin1String("compact")));
	SettingsManager::setOption(SettingsManager::AddressField_ShowCompletionCategoriesOption, m_ui->browsingCategoriesCheckBox->isChecked());

	updateNotificationsOptions();

	IniSettings notificationsSettings(SessionsManager::getWritableDataPath(QLatin1String("notifications.ini")), this);
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

	QString styleSheetPath(m_ui->appearranceStyleSheetFilePathWidget->getPath());
	QString styleSheet;

	if (!styleSheetPath.isEmpty())
	{
		QFile file(styleSheetPath);

		if (file.open(QIODevice::ReadOnly))
		{
			styleSheet = QString::fromLatin1(file.readAll());

			file.close();
		}
	}

	Application::getInstance()->setStyleSheet(styleSheet);

	const QMimeDatabase mimeDatabase;

	QFile::remove(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));

	updateDownloadsOptions();

	for (int i = 0; i < m_ui->mimeTypesItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->mimeTypesItemView->getIndex(i, 0));

		if (index.data(Qt::DisplayRole).isNull())
		{
			continue;
		}

		HandlersManager::MimeTypeHandlerDefinition hamdler;

		if (index.data(Qt::DisplayRole).toString() != QLatin1String("*"))
		{
			hamdler.mimeType = mimeDatabase.mimeTypeForName(index.data(Qt::DisplayRole).toString());
		}

		hamdler.openCommand = index.data(OpenCommandRole).toString();
		hamdler.downloadsPath = index.data(DownloadsPathRole).toString();
		hamdler.transferMode = static_cast<HandlersManager::MimeTypeHandlerDefinition::TransferMode>(index.data(TransferModeRole).toInt());

		HandlersManager::setMimeTypeHandler(hamdler.mimeType, hamdler);
	}

	SettingsManager::setOption(SettingsManager::Permissions_EnableJavaScriptOption, m_ui->enableJavaScriptCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanChangeWindowGeometryOption, m_ui->scriptsCanChangeWindowGeometryCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanShowStatusMessagesOption, m_ui->scriptsCanShowStatusMessagesCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanAccessClipboardOption, m_ui->scriptsCanAccessClipboardCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanReceiveRightClicksOption, m_ui->scriptsCanReceiveRightClicksCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, m_ui->scriptsCanCloseWindowsComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Permissions_EnableFullScreenOption, m_ui->enableFullScreenComboBox->currentData().toString());

	SettingsManager::setOption(SettingsManager::Network_EnableReferrerOption, m_ui->sendReferrerCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Network_UserAgentOption, m_ui->userAgentsViewWidget->getCheckedIndex().data(UserAgentsModel::IdentifierRole).toString());

	if (m_ui->userAgentsViewWidget->isModified())
	{
		QJsonArray userAgentsArray;

		saveUserAgents(&userAgentsArray, m_ui->userAgentsViewWidget->getSourceModel()->invisibleRootItem());

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
		ciphers.reserve(m_ui->ciphersViewWidget->getRowCount());

		for (int i = 0; i < m_ui->ciphersViewWidget->getRowCount(); ++i)
		{
			ciphers.append(m_ui->ciphersViewWidget->getIndex(i, 0).data(Qt::DisplayRole).toString());
		}

		SettingsManager::setOption(SettingsManager::Security_CiphersOption, ciphers);
	}

	QStringList updateChannels;
	updateChannels.reserve(m_ui->updateChannelsItemView->getRowCount());

	for (int i = 0; i < m_ui->updateChannelsItemView->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->updateChannelsItemView->getIndex(i, 0));

		if (static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked)
		{
			updateChannels.append(index.data(Qt::UserRole).toString());
		}
	}

	SettingsManager::setOption(SettingsManager::Updates_ActiveChannelsOption, updateChannels);
	SettingsManager::setOption(SettingsManager::Updates_AutomaticInstallOption, m_ui->autoInstallCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Updates_CheckIntervalOption, m_ui->intervalSpinBox->value());

	Utils::ensureDirectoryExists(SessionsManager::getWritableDataPath(QLatin1String("mouse")));

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
	mouseProfiles.reserve(m_ui->mouseViewWidget->getRowCount());

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
}

void AdvancedPreferencesPage::updatePageSwitcher()
{
	int maximumWidth(100);

	for (int i = 0; i < m_ui->advancedViewWidget->model()->rowCount(); ++i)
	{
		const int itemWidth(m_ui->advancedViewWidget->fontMetrics().horizontalAdvance(m_ui->advancedViewWidget->model()->index(i, 0).data(Qt::DisplayRole).toString()) + 10);

		if (itemWidth > maximumWidth)
		{
			maximumWidth = itemWidth;
		}
	}

	m_ui->advancedViewWidget->setMinimumWidth(qMin(200, maximumWidth));
}

QString AdvancedPreferencesPage::createProfileIdentifier(QStandardItemModel *model, const QString &base) const
{
	QStringList identifiers;
	identifiers.reserve(model->rowCount());

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

QString AdvancedPreferencesPage::getTitle() const
{
	return tr("Advanced");
}

}
