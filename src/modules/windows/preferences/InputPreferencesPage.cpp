/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "InputPreferencesPage.h"
#include "../../../core/ItemModel.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/ActionComboBoxWidget.h"
#include "../../../ui/ActionParametersDialog.h"
#include "../../../ui/MetaDataDialog.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>

#include "ui_InputPreferencesPage.h"

namespace Otter
{

ShortcutWidget::ShortcutWidget(const QKeySequence &shortcut, QWidget *parent) : QKeySequenceEdit(shortcut, parent),
	m_clearButton(nullptr)
{
	QVBoxLayout *layout(findChild<QVBoxLayout*>());

	if (!layout)
	{
		return;
	}

	m_clearButton = new QToolButton(this);
	m_clearButton->setText(tr("Clear"));
	m_clearButton->setEnabled(!shortcut.isEmpty());

	layout->setDirection(QBoxLayout::LeftToRight);
	layout->addWidget(m_clearButton);

	connect(m_clearButton, &QToolButton::clicked, this, &ShortcutWidget::clear);
	connect(this, &ShortcutWidget::keySequenceChanged, [&]()
	{
		const bool isEmpty(keySequence().isEmpty());

		m_clearButton->setEnabled(!isEmpty);

		if (isEmpty)
		{
			setStyleSheet({});
			setToolTip({});
		}

		emit commitData(this);
	});
}

void ShortcutWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && m_clearButton)
	{
		m_clearButton->setText(tr("Clear"));
	}
}

ActionDelegate::ActionDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ActionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const ActionComboBoxWidget *widget(qobject_cast<ActionComboBoxWidget*>(editor));

	if (!widget || widget->getActionIdentifier() <= 0)
	{
		return;
	}

	const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(widget->getActionIdentifier()));
	const QString name(ActionsManager::getActionName(widget->getActionIdentifier()));

	model->setData(index, definition.getText(true), Qt::DisplayRole);
	model->setData(index, QStringLiteral("%1 (%2)").arg(definition.getText(true), name), Qt::ToolTipRole);
	model->setData(index, ItemModel::createDecoration(definition.defaultState.icon), Qt::DecorationRole);
	model->setData(index, widget->getActionIdentifier(), InputPreferencesPage::IdentifierRole);
	model->setData(index, name, InputPreferencesPage::NameRole);
}

QWidget* ActionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ActionComboBoxWidget *widget(new ActionComboBoxWidget(parent));
	widget->setActionIdentifier(index.data(InputPreferencesPage::IdentifierRole).toInt());
	widget->setFocus();

	return widget;
}

ParametersDelegate::ParametersDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ParametersDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	option->text = InputPreferencesPage::createParamatersPreview(index.sibling(index.row(), 1).data(InputPreferencesPage::ParametersRole).toMap(), QLatin1String("; "));
}

ShortcutDelegate::ShortcutDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ShortcutDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	option->text = QKeySequence(index.data(Qt::DisplayRole).toString()).toString(QKeySequence::NativeText);

	if (index.data(InputPreferencesPage::IsDisabledRole).toBool())
	{
		QFont font(option->font);
		font.setStrikeOut(true);

		option->font = font;
		option->state &= ~QStyle::State_Enabled;
	}
}

void ShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const ShortcutWidget *widget(qobject_cast<ShortcutWidget*>(editor));

	if (!widget)
	{
		return;
	}

	const QKeySequence shortcut(widget->keySequence());

	model->setData(index, (shortcut.isEmpty() ? QVariant() : QVariant(shortcut.toString())));

	if (index.sibling(index.row(), 3).data(InputPreferencesPage::IsDisabledRole).toBool())
	{
		return;
	}

	const InputPreferencesPage::ValidationResult result(InputPreferencesPage::validateShortcut(shortcut, index));
	const QModelIndex statusIndex(index.sibling(index.row(), 0));

	if (result.text.isEmpty())
	{
		model->setData(statusIndex, {}, Qt::DecorationRole);
		model->setData(statusIndex, {}, Qt::ToolTipRole);
		model->setData(statusIndex, InputPreferencesPage::NormalStatus, InputPreferencesPage::StatusRole);
	}
	else
	{
		model->setData(statusIndex, result.icon, Qt::DecorationRole);
		model->setData(statusIndex, result.text, Qt::ToolTipRole);
		model->setData(statusIndex, (result.isError ? InputPreferencesPage::ErrorStatus : InputPreferencesPage::WarningStatus), InputPreferencesPage::StatusRole);
	}
}

QWidget* ShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ShortcutWidget *widget(new ShortcutWidget(QKeySequence(index.data(Qt::DisplayRole).toString()), parent));
	widget->setFocus();

	connect(widget, &ShortcutWidget::commitData, this, &ShortcutDelegate::commitData);
	connect(widget, &ShortcutWidget::keySequenceChanged, [=](const QKeySequence &shortcut)
	{
		if (!shortcut.isEmpty() && !index.data(InputPreferencesPage::IsDisabledRole).toBool())
		{
			const InputPreferencesPage::ValidationResult result(InputPreferencesPage::validateShortcut(shortcut, index));

			if (!result.text.isEmpty())
			{
				widget->setStyleSheet(QLatin1String("QLineEdit {background:#F1E7E4;}"));
				widget->setToolTip(result.text);

				QTimer::singleShot(5000, widget, [=]()
				{
					widget->setStyleSheet({});
					widget->setToolTip({});
				});
			}
		}
	});

	return widget;
}

bool InputPreferencesPage::m_areSingleKeyShortcutsAllowed(true);

InputPreferencesPage::InputPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_keyboardShortcutsModel(new QStandardItemModel(this)),
	m_advancedButton(new QPushButton(tr("Advanced…"), this)),
	m_ui(nullptr)
{
}

InputPreferencesPage::~InputPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void InputPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && wasLoaded())
	{
		m_ui->retranslateUi(this);

		m_keyboardShortcutsModel->setHorizontalHeaderLabels({tr("Status"), tr("Action"), tr("Parameters"), tr("Shortcut")});
		m_advancedButton->setText(tr("Advanced…"));
	}
}

void InputPreferencesPage::loadKeyboardDefinitions(const QString &identifier)
{
	if (identifier == m_activeKeyboardProfile)
	{
		return;
	}

	if (m_ui->keyboardShortcutsViewWidget->isModified())
	{
		m_keyboardProfiles[m_activeKeyboardProfile].setDefinitions(getKeyboardDefinitions());
	}

	m_activeKeyboardProfile = identifier;

	m_keyboardShortcutsModel->removeRows(0, m_keyboardShortcutsModel->rowCount());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	const KeyboardProfile profile(m_keyboardProfiles[identifier]);
	const QVector<KeyboardProfile::Action> definitions(profile.getDefinitions().value(ActionsManager::GenericContext));

	for (int i = 0; i < definitions.count(); ++i)
	{
		const KeyboardProfile::Action shortcutsDefinition(definitions.at(i));
		const ActionsManager::ActionDefinition actionDefinition(ActionsManager::getActionDefinition(shortcutsDefinition.action));
		const QString name(ActionsManager::getActionName(shortcutsDefinition.action));
		const QString description(actionDefinition.getText(true));

		addKeyboardShortcuts(shortcutsDefinition.action, name, description, actionDefinition.defaultState.icon, shortcutsDefinition.parameters, shortcutsDefinition.shortcuts, false);
		addKeyboardShortcuts(shortcutsDefinition.action, name, description, actionDefinition.defaultState.icon, shortcutsDefinition.parameters, shortcutsDefinition.disabledShortcuts, true);
	}

	m_keyboardShortcutsModel->sort(1);

	m_ui->keyboardShortcutsViewWidget->setModified(profile.isModified());
}

void InputPreferencesPage::addKeyboardShortcuts(int identifier, const QString &name, const QString &text, const QIcon &icon, const QVariantMap &rawParameters, const QVector<QKeySequence> &shortcuts, bool areShortcutsDisabled)
{
	const QString parameters(createParamatersPreview(rawParameters, QLatin1String("\n")));

	for (int i = 0; i < shortcuts.count(); ++i)
	{
		const QKeySequence shortcut(shortcuts.at(i));
		QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(text), new QStandardItem(parameters), new QStandardItem(shortcut.toString())});
		items[0]->setData(NormalStatus, StatusRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setData(ItemModel::createDecoration(icon), Qt::DecorationRole);
		items[1]->setData(identifier, IdentifierRole);
		items[1]->setData(name, NameRole);
		items[1]->setData(rawParameters, ParametersRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
		items[1]->setToolTip(QStringLiteral("%1 (%2)").arg(text, name));
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[2]->setToolTip(parameters);
		items[3]->setData(areShortcutsDisabled, IsDisabledRole);
		items[3]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

		m_keyboardShortcutsModel->appendRow(items);

		if (areShortcutsDisabled)
		{
			return;
		}

		const ValidationResult result(validateShortcut(shortcut, items[3]->index()));

		if (!result.text.isEmpty())
		{
			items[0]->setData(result.icon, Qt::DecorationRole);
			items[0]->setData(result.text, Qt::ToolTipRole);
			items[0]->setData((result.isError ? ErrorStatus : WarningStatus), StatusRole);
		}
	}
}

void InputPreferencesPage::addKeyboardShortcut(bool isDisabled)
{
	QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(), new QStandardItem(), new QStandardItem()});
	items[0]->setData(NormalStatus, StatusRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
	items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	items[3]->setData(isDisabled, IsDisabledRole);
	items[3]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

	m_ui->keyboardShortcutsViewWidget->insertRow(items);
	m_ui->keyboardShortcutsViewWidget->setCurrentIndex(items[1]->index());
}

void InputPreferencesPage::addKeyboardProfile()
{
	const QString identifier(createProfileIdentifier(m_ui->keyboardProfilesViewWidget->getSourceModel()));

	if (identifier.isEmpty())
	{
		return;
	}

	m_keyboardProfiles[identifier] = KeyboardProfile(identifier);

	QStandardItem *item(new QStandardItem(tr("(Untitled)")));
	item->setData(identifier, Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
	item->setCheckState(Qt::Checked);

	m_ui->keyboardProfilesViewWidget->insertRow({item});
}

void InputPreferencesPage::editKeyboardProfile()
{
	const QModelIndex index(m_ui->keyboardProfilesViewWidget->currentIndex());
	const QString identifier(index.data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		if (!index.isValid())
		{
			addKeyboardProfile();
		}

		return;
	}

	MetaDataDialog dialog(m_keyboardProfiles[identifier].getMetaData(), this);

	if (dialog.exec() == QDialog::Rejected || !dialog.isModified())
	{
		return;
	}

	const Addon::MetaData metaData(dialog.getMetaData());

	m_keyboardProfiles[identifier].setMetaData(metaData);

	m_ui->keyboardProfilesViewWidget->markAsModified();
	m_ui->keyboardProfilesViewWidget->setData(index, metaData.title, Qt::DisplayRole);
	m_ui->keyboardProfilesViewWidget->setData(index, metaData.description, Qt::ToolTipRole);
}

void InputPreferencesPage::cloneKeyboardProfile()
{
	const QString identifier(m_ui->keyboardProfilesViewWidget->currentIndex().data(Qt::UserRole).toString());

	if (identifier.isEmpty() || !m_keyboardProfiles.contains(identifier))
	{
		return;
	}

	const QString newIdentifier(createProfileIdentifier(m_ui->keyboardProfilesViewWidget->getSourceModel(), identifier));

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
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
	item->setCheckState(Qt::Checked);

	m_ui->keyboardProfilesViewWidget->insertRow({item});
}

void InputPreferencesPage::removeKeyboardProfile()
{
	const QString identifier(m_ui->keyboardProfilesViewWidget->currentIndex().data(Qt::UserRole).toString());

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

		m_ui->keyboardProfilesViewWidget->removeRow();
	}
}

void InputPreferencesPage::updateKeyboardProfileActions()
{
	const int currentRow(m_ui->keyboardProfilesViewWidget->getCurrentRow());
	const bool isSelected(currentRow >= 0 && currentRow < m_ui->keyboardProfilesViewWidget->getRowCount());

	m_ui->keyboardEditProfileButton->setEnabled(isSelected);
	m_ui->keyboardCloneProfileButton->setEnabled(isSelected);
	m_ui->keyboardRemoveProfileButton->setEnabled(isSelected);

	loadKeyboardDefinitions(m_ui->keyboardProfilesViewWidget->getCurrentIndex().data(Qt::UserRole).toString());
	updateKeyboardShortcutActions();
}

void InputPreferencesPage::editShortcutParameters()
{
	const QModelIndex index(m_ui->keyboardShortcutsViewWidget->getCurrentIndex(1));
	ActionParametersDialog dialog(index.data(IdentifierRole).toInt(), index.sibling(index.row(), 1).data(ParametersRole).toMap(), this);

	if (dialog.exec() == QDialog::Accepted && dialog.isModified())
	{
		const QVariantMap parameters(dialog.getParameters());

		m_keyboardShortcutsModel->setData(index, parameters, ParametersRole);
		m_keyboardShortcutsModel->setData(index.sibling(index.row(), 2), createParamatersPreview(parameters, QLatin1String("\n")), Qt::ToolTipRole);
	}
}

void InputPreferencesPage::updateKeyboardShortcutActions()
{
	m_ui->keyboardShortcutsButtonsWidget->setEnabled(!m_activeKeyboardProfile.isEmpty() && m_keyboardProfiles.contains(m_activeKeyboardProfile));
}

void InputPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_areSingleKeyShortcutsAllowed = SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool();

	m_advancedButton->setCheckable(true);
	m_advancedButton->setChecked(true);
	m_advancedButton->setEnabled(false);

	m_ui = new Ui::InputPreferencesPage();
	m_ui->setupUi(this);
	m_ui->tabWidget->setCornerWidget(m_advancedButton);
	m_ui->keyboardEnableSingleKeyShortcutsCheckBox->setChecked(m_areSingleKeyShortcutsAllowed);
	m_ui->keyboardMoveProfileDownButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->keyboardMoveProfileUpButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));

	QStandardItemModel *keyboardProfilesModel(new QStandardItemModel(this));
	const QStringList keyboardProfiles(SettingsManager::getOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList());

	for (int i = 0; i < keyboardProfiles.count(); ++i)
	{
		const QString identifier(keyboardProfiles.at(i));
		const KeyboardProfile profile(identifier, KeyboardProfile::FullMode);

		if (!profile.isValid())
		{
			continue;
		}

		m_keyboardProfiles[identifier] = profile;

		QStandardItem *item(new QStandardItem(profile.getTitle()));
		item->setToolTip(profile.getDescription());
		item->setData(profile.getName(), Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
		item->setCheckState(Qt::Checked);

		keyboardProfilesModel->appendRow(item);
	}

	m_ui->keyboardProfilesViewWidget->setModel(keyboardProfilesModel);
	m_ui->keyboardProfilesViewWidget->setRowsMovable(true);
	m_ui->keyboardProfilesViewWidget->setCurrentIndex(keyboardProfilesModel->index(0, 0));

	m_keyboardShortcutsModel->setHorizontalHeaderLabels({tr("Status"), tr("Action"), tr("Parameters"), tr("Shortcut")});
	m_keyboardShortcutsModel->setHeaderData(0, Qt::Horizontal, 28, HeaderViewWidget::WidthRole);
	m_keyboardShortcutsModel->setHeaderData(1, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);
	m_keyboardShortcutsModel->setHeaderData(2, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);

	m_ui->keyboardShortcutsViewWidget->setModel(m_keyboardShortcutsModel);
	m_ui->keyboardShortcutsViewWidget->setItemDelegateForColumn(1, new ActionDelegate(this));
	m_ui->keyboardShortcutsViewWidget->setItemDelegateForColumn(2, new ParametersDelegate(this));
	m_ui->keyboardShortcutsViewWidget->setItemDelegateForColumn(3, new ShortcutDelegate(this));
	m_ui->keyboardShortcutsViewWidget->setFilterRoles({Qt::DisplayRole, NameRole});
	m_ui->keyboardShortcutsViewWidget->setSortRoleMapping({{0, StatusRole}});

	loadKeyboardDefinitions(keyboardProfilesModel->index(0, 0).data(Qt::UserRole).toString());
	updateKeyboardProfileActions();
	updateKeyboardShortcutActions();

	m_ui->tabWidget->setTabEnabled(1, false);

	connect(m_ui->keyboardEnableSingleKeyShortcutsCheckBox, &QCheckBox::toggled, this, [&](bool isChecked)
	{
		m_areSingleKeyShortcutsAllowed = isChecked;
	});
	connect(m_ui->keyboardProfilesViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->keyboardMoveProfileDownButton, &QToolButton::setEnabled);
	connect(m_ui->keyboardProfilesViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->keyboardMoveProfileUpButton, &QToolButton::setEnabled);
	connect(m_ui->keyboardProfilesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &InputPreferencesPage::updateKeyboardProfileActions);
	connect(m_ui->keyboardProfilesViewWidget, &ItemViewWidget::doubleClicked, this, &InputPreferencesPage::editKeyboardProfile);
	connect(m_ui->keyboardAddProfileButton, &QPushButton::clicked, this, &InputPreferencesPage::addKeyboardProfile);
	connect(m_ui->keyboardEditProfileButton, &QPushButton::clicked, this, &InputPreferencesPage::editKeyboardProfile);
	connect(m_ui->keyboardCloneProfileButton, &QPushButton::clicked, this, &InputPreferencesPage::cloneKeyboardProfile);
	connect(m_ui->keyboardRemoveProfileButton, &QPushButton::clicked, this, &InputPreferencesPage::removeKeyboardProfile);
	connect(m_ui->keyboardMoveProfileDownButton, &QToolButton::clicked, m_ui->keyboardProfilesViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->keyboardMoveProfileUpButton, &QToolButton::clicked, m_ui->keyboardProfilesViewWidget, &ItemViewWidget::moveUpRow);
	connect(m_ui->keyboardShortcutsFilterLineEditWidget, &QLineEdit::textChanged, m_ui->keyboardShortcutsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->keyboardShortcutsViewWidget, &ItemViewWidget::needsActionsUpdate, this, [&]()
	{
		const bool isValid(m_ui->keyboardShortcutsViewWidget->getCurrentIndex().isValid());

		m_ui->keyboardShortcutParametersButton->setEnabled(isValid);
		m_ui->keyboardRemoveShortcutButton->setEnabled(isValid);
	});
	connect(m_ui->keyboardAddShortcutButton, &QPushButton::clicked, this, [&]()
	{
		addKeyboardShortcut(false);
	});
	connect(m_ui->keyboardDisableShortcutButton, &QPushButton::clicked, this, [&]()
	{
		addKeyboardShortcut(true);
	});
	connect(m_ui->keyboardShortcutParametersButton, &QPushButton::clicked, this, &InputPreferencesPage::editShortcutParameters);
	connect(m_ui->keyboardRemoveShortcutButton, &QPushButton::clicked, m_ui->keyboardShortcutsViewWidget, &ItemViewWidget::removeRow);

	markAsLoaded();
}

void InputPreferencesPage::save()
{
	Utils::removeFiles(m_filesToRemove);

	m_filesToRemove.clear();

	Utils::ensureDirectoryExists(SessionsManager::getWritableDataPath(QLatin1String("keyboard")));

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
	keyboardProfiles.reserve(m_ui->keyboardProfilesViewWidget->getRowCount());

	for (int i = 0; i < m_ui->keyboardProfilesViewWidget->getRowCount(); ++i)
	{
		const QString identifier(m_ui->keyboardProfilesViewWidget->getIndex(i, 0).data(Qt::UserRole).toString());

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
}

QString InputPreferencesPage::createProfileIdentifier(QStandardItemModel *model, const QString &base) const
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

QString InputPreferencesPage::createParamatersPreview(const QVariantMap &rawParameters, const QString &separator)
{
	QStringList parameters;
	parameters.reserve(rawParameters.count());

	QVariantMap::const_iterator iterator;

	for (iterator = rawParameters.begin(); iterator != rawParameters.end(); ++iterator)
	{
		QString value;

		switch (iterator.value().type())
		{
			case QVariant::List:
			case QVariant::StringList:
				value = QLatin1Char('[') + iterator.value().toStringList().join(QLatin1String(", ")) + QLatin1Char(']');

				break;
			case QVariant::Map:
				value = QStringLiteral("{…}");

				break;
			default:
				value = iterator.value().toString();

				break;
		}

		parameters.append(iterator.key() + QLatin1String(": ") + value);
	}

	return parameters.join(separator);
}

QString InputPreferencesPage::getTitle() const
{
	return tr("Input");
}

InputPreferencesPage::ValidationResult InputPreferencesPage::validateShortcut(const QKeySequence &shortcut, const QModelIndex &index)
{
	if (shortcut.isEmpty())
	{
		return {};
	}

	ValidationResult result;
	QStringList messages;
	QModelIndexList indexes(index.model()->match(index.model()->index(0, 3), Qt::DisplayRole, shortcut.toString(), 2, Qt::MatchExactly));
	indexes.removeAll(index);

	if (!indexes.isEmpty())
	{
		const QModelIndex matchedIndex(indexes.value(0));
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(matchedIndex.sibling(matchedIndex.row(), 1).data(IdentifierRole).toInt()));

		messages.append(tr("This shortcut is already used by %1").arg(definition.isValid() ? definition.getText(true) : tr("unknown action")));

		result.isError = true;
	}

	if (!ActionsManager::isShortcutAllowed(shortcut, ActionsManager::DisallowStandardShortcutCheck, false))
	{
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(index.sibling(index.row(), 1).data(IdentifierRole).toInt()));

		if (!definition.isValid() || definition.category != ActionsManager::ActionDefinition::EditingCategory)
		{
			messages.append(tr("This shortcut cannot be used because it would be overriden by a native hotkey used by an editing action"));

			result.isError = true;
		}
	}

	if (!m_areSingleKeyShortcutsAllowed && !ActionsManager::isShortcutAllowed(shortcut, ActionsManager::DisallowSingleKeyShortcutCheck, false))
	{
		messages.append(tr("Single key shortcuts are currently disabled"));
	}

	if (!messages.isEmpty())
	{
		result.text = messages.join(QLatin1Char('\n'));
		result.icon = ThemesManager::createIcon(result.isError ? QLatin1String("dialog-error") : QLatin1String("dialog-warning"));
	}

	return result;
}

QHash<int, QVector<KeyboardProfile::Action> > InputPreferencesPage::getKeyboardDefinitions() const
{
	QMap<int, QVector<ShortcutsDefinition> > actions;

	for (int i = 0; i < m_ui->keyboardShortcutsViewWidget->getRowCount(); ++i)
	{
		if (static_cast<EntryStatus>(m_ui->keyboardShortcutsViewWidget->getIndex(i, 0).data(StatusRole).toInt()) == ErrorStatus)
		{
			continue;
		}

		const QKeySequence shortcut(m_ui->keyboardShortcutsViewWidget->getIndex(i, 3).data(Qt::DisplayRole).toString());
		const int action(m_ui->keyboardShortcutsViewWidget->getIndex(i, 1).data(IdentifierRole).toInt());

		if (action < 0 || shortcut.isEmpty())
		{
			continue;
		}

		const QVariantMap parameters(m_ui->keyboardShortcutsViewWidget->getIndex(i, 1).data(ParametersRole).toMap());
		const bool isDisabled(m_ui->keyboardShortcutsViewWidget->getIndex(i, 3).data(IsDisabledRole).toBool());
		bool hasMatch(false);

		if (actions.contains(action))
		{
			QVector<ShortcutsDefinition> actionVariants(actions[action]);

			for (int j = 0; j < actionVariants.count(); ++j)
			{
				if (actionVariants.at(j).parameters != parameters)
				{
					continue;
				}

				if (isDisabled)
				{
					actionVariants[j].disabledShortcuts.append(shortcut);
				}
				else
				{
					actionVariants[j].shortcuts.append(shortcut);
				}

				actions[action] = actionVariants;

				hasMatch = true;

				break;
			}
		}
		else
		{
			actions[action] = {};
		}

		if (!hasMatch)
		{
			if (isDisabled)
			{
				actions[action].append({parameters, {}, {shortcut}});
			}
			else
			{
				actions[action].append({parameters, {shortcut}, {}});
			}
		}
	}

	QMap<int, QVector<ShortcutsDefinition> >::iterator iterator;
	QVector<KeyboardProfile::Action> definitions;
	definitions.reserve(actions.count());

	for (iterator = actions.begin(); iterator != actions.end(); ++iterator)
	{
		const QVector<ShortcutsDefinition> actionVariants(iterator.value());

		for (int j = 0; j < actionVariants.count(); ++j)
		{
			const ShortcutsDefinition actionVariant(actionVariants.at(j));
			KeyboardProfile::Action definition;
			definition.parameters = actionVariant.parameters;
			definition.shortcuts = actionVariant.shortcuts;
			definition.disabledShortcuts = actionVariant.disabledShortcuts;
			definition.action = iterator.key();

			definitions.append(definition);
		}
	}

	return {{ActionsManager::GenericContext, definitions}};
}

}
