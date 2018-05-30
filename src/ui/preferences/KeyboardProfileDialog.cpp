/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "KeyboardProfileDialog.h"
#include "../ActionComboBoxWidget.h"
#include "../../core/ThemesManager.h"

#include "ui_KeyboardProfileDialog.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

namespace Otter
{

ShortcutWidget::ShortcutWidget(const QKeySequence &shortcut, QWidget *parent) : QKeySequenceEdit(shortcut, parent),
	m_clearButton(nullptr)
{
	QVBoxLayout *layout(findChild<QVBoxLayout*>());

	if (layout)
	{
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
}

void ShortcutWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange && m_clearButton)
	{
		m_clearButton->setText(tr("Clear"));
	}
}

KeyboardActionDelegate::KeyboardActionDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void KeyboardActionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const ActionComboBoxWidget *widget(qobject_cast<ActionComboBoxWidget*>(editor));

	if (widget && widget->getActionIdentifier() >= 0)
	{
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(widget->getActionIdentifier()));
		const QString name(ActionsManager::getActionName(widget->getActionIdentifier()));

		model->setData(index, definition.getText(true), Qt::DisplayRole);
		model->setData(index, QStringLiteral("%1 (%2)").arg(definition.getText(true)).arg(name), Qt::ToolTipRole);
		model->setData(index, widget->getActionIdentifier(), KeyboardProfileDialog::IdentifierRole);
		model->setData(index, name, KeyboardProfileDialog::NameRole);

		if (definition.defaultState.icon.isNull())
		{
			model->setData(index, QColor(Qt::transparent), Qt::DecorationRole);
		}
		else
		{
			model->setData(index, definition.defaultState.icon, Qt::DecorationRole);
		}
	}
}

QWidget* KeyboardActionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ActionComboBoxWidget *widget(new ActionComboBoxWidget(parent));
	widget->setActionIdentifier(index.data(KeyboardProfileDialog::IdentifierRole).toInt());
	widget->setFocus();

	return widget;
}

KeyboardShortcutDelegate::KeyboardShortcutDelegate(KeyboardProfileDialog *parent) : ItemDelegate(parent),
	m_dialog(parent)
{
}

void KeyboardShortcutDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	option->text = QKeySequence(index.data(Qt::DisplayRole).toString()).toString(QKeySequence::NativeText);
}

void KeyboardShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const ShortcutWidget *widget(qobject_cast<ShortcutWidget*>(editor));

	if (widget)
	{
		const QKeySequence shortcut(widget->keySequence());
		const KeyboardProfileDialog::ValidationResult result(m_dialog->validateShortcut(shortcut, index));
		const QModelIndex statusIndex(index.sibling(index.row(), 0));

		if (result.text.isEmpty())
		{
			model->setData(statusIndex, {}, Qt::DecorationRole);
			model->setData(statusIndex, {}, Qt::ToolTipRole);
			model->setData(statusIndex, KeyboardProfileDialog::NormalStatus, KeyboardProfileDialog::StatusRole);
		}
		else
		{
			model->setData(statusIndex, result.icon, Qt::DecorationRole);
			model->setData(statusIndex, result.text, Qt::ToolTipRole);
			model->setData(statusIndex, (result.isError ? KeyboardProfileDialog::ErrorStatus : KeyboardProfileDialog::WarningStatus), KeyboardProfileDialog::StatusRole);
		}

		model->setData(index, (shortcut.isEmpty() ? QVariant() : QVariant(shortcut.toString())));
	}
}

QWidget* KeyboardShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ShortcutWidget *widget(new ShortcutWidget(QKeySequence(index.data(Qt::DisplayRole).toString()), parent));
	widget->setFocus();

	connect(widget, &ShortcutWidget::commitData, this, &KeyboardShortcutDelegate::commitData);
	connect(widget, &ShortcutWidget::keySequenceChanged, [=](const QKeySequence &shortcut)
	{
		if (!shortcut.isEmpty())
		{
			const KeyboardProfileDialog::ValidationResult result(m_dialog->validateShortcut(shortcut, index));

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

KeyboardProfileDialog::KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, bool areSingleKeyShortcutsAllowed, QWidget *parent) : Dialog(parent),
	m_profile(profiles[profile]),
	m_areSingleKeyShortcutsAllowed(areSingleKeyShortcutsAllowed),
	m_ui(new Ui::KeyboardProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<KeyboardProfile::Action> definitions(m_profile.getDefinitions().value(ActionsManager::GenericContext));

	for (int i = 0; i < definitions.count(); ++i)
	{
		const ActionsManager::ActionDefinition action(ActionsManager::getActionDefinition(definitions.at(i).action));
		const QString name(ActionsManager::getActionName(definitions.at(i).action));
		const QString parameters(definitions.at(i).parameters.isEmpty() ? QString() : QJsonDocument(QJsonObject::fromVariantMap(definitions.at(i).parameters)).toJson(QJsonDocument::Compact));

		for (int j = 0; j < definitions.at(i).shortcuts.count(); ++j)
		{
			const QKeySequence shortcut(definitions.at(i).shortcuts.at(j));
			QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(action.getText(true)), new QStandardItem(parameters), new QStandardItem(shortcut.toString())});
			items[0]->setData(NormalStatus, StatusRole);
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
			items[1]->setData(QColor(Qt::transparent), Qt::DecorationRole);
			items[1]->setData(definitions.at(i).action, IdentifierRole);
			items[1]->setData(name, NameRole);
			items[1]->setData(definitions.at(i).parameters, ParametersRole);
			items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
			items[1]->setToolTip(QStringLiteral("%1 (%2)").arg(action.getText(true)).arg(name));
			items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
			items[2]->setToolTip(parameters);
			items[3]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

			if (!action.defaultState.icon.isNull())
			{
				items[1]->setIcon(action.defaultState.icon);
			}

			model->appendRow(items);

			const ValidationResult result(validateShortcut(shortcut, items[3]->index()));

			if (!result.text.isEmpty())
			{
				items[0]->setData(result.icon, Qt::DecorationRole);
				items[0]->setData(result.text, Qt::ToolTipRole);
				items[0]->setData((result.isError ? ErrorStatus : WarningStatus), StatusRole);
			}
		}
	}

	model->setHorizontalHeaderLabels({tr("Status"), tr("Action"), tr("Parameters"), tr("Shortcut")});
	model->setHeaderData(0, Qt::Horizontal, 28, HeaderViewWidget::WidthRole);
	model->sort(1);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->actionsViewWidget->setItemDelegateForColumn(1, new KeyboardActionDelegate(this));
	m_ui->actionsViewWidget->setItemDelegateForColumn(3, new KeyboardShortcutDelegate(this));
	m_ui->actionsViewWidget->setFilterRoles({Qt::DisplayRole, NameRole});
	m_ui->actionsViewWidget->setSortRoleMapping({{0, StatusRole}});
	m_ui->actionsViewWidget->setModified(m_profile.isModified());
	m_ui->titleLineEditWidget->setText(m_profile.getTitle());
	m_ui->descriptionLineEditWidget->setText(m_profile.getDescription());
	m_ui->versionLineEditWidget->setText(m_profile.getVersion());
	m_ui->authorLineEditWidget->setText(m_profile.getAuthor());

	connect(m_ui->titleLineEditWidget, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->descriptionLineEditWidget, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->versionLineEditWidget, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->authorLineEditWidget, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->filterLineEditWidget, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->actionsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &KeyboardProfileDialog::updateActions);
	connect(m_ui->addActionButton, &QPushButton::clicked, this, &KeyboardProfileDialog::addAction);
	connect(m_ui->removeActionButton, &QPushButton::clicked, this, &KeyboardProfileDialog::removeAction);
}

KeyboardProfileDialog::~KeyboardProfileDialog()
{
	delete m_ui;
}

void KeyboardProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->actionsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Status"), tr("Action"), tr("Parameters"), tr("Shortcut")});
	}
}

void KeyboardProfileDialog::addAction()
{
	QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(), new QStandardItem(), new QStandardItem()});
	items[0]->setData(NormalStatus, StatusRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
	items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	items[3]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

	m_ui->actionsViewWidget->insertRow(items);
	m_ui->actionsViewWidget->setCurrentIndex(items[1]->index());
}

void KeyboardProfileDialog::removeAction()
{
	m_ui->actionsViewWidget->removeRow();
}

void KeyboardProfileDialog::updateActions()
{
	m_ui->removeActionButton->setEnabled(m_ui->actionsViewWidget->getCurrentIndex().isValid());
}

KeyboardProfile KeyboardProfileDialog::getProfile() const
{
	KeyboardProfile profile(m_profile);
	profile.setTitle(m_ui->titleLineEditWidget->text());
	profile.setDescription(m_ui->descriptionLineEditWidget->text());
	profile.setVersion(m_ui->versionLineEditWidget->text());
	profile.setAuthor(m_ui->authorLineEditWidget->text());

	QMap<int, QVector<QPair<QVariantMap, QVector<QKeySequence> > > > actions;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		if (static_cast<ShortcutStatus>(m_ui->actionsViewWidget->getIndex(i, 0).data(StatusRole).toInt()) == ErrorStatus)
		{
			continue;
		}

		const QKeySequence shortcut(m_ui->actionsViewWidget->getIndex(i, 3).data(Qt::DisplayRole).toString());
		const int action(m_ui->actionsViewWidget->getIndex(i, 1).data(IdentifierRole).toInt());

		if (action >= 0 && !shortcut.isEmpty())
		{
			const QVariantMap parameters(m_ui->actionsViewWidget->getIndex(i, 1).data(ParametersRole).toMap());
			bool hasMatch(false);

			if (actions.contains(action))
			{
				QVector<QPair<QVariantMap, QVector<QKeySequence> > > actionVariants(actions[action]);

				for (int j = 0; j < actionVariants.count(); ++j)
				{
					if (actionVariants.at(j).first == parameters)
					{
						actionVariants[j].second.append(shortcut);

						actions[action] = actionVariants;

						hasMatch = true;

						break;
					}
				}
			}

			if (!hasMatch)
			{
				actions[action] = {{parameters, {shortcut}}};
			}
		}
	}

	QMap<int, QVector<QPair<QVariantMap, QVector<QKeySequence> > > >::iterator iterator;
	QVector<KeyboardProfile::Action> definitions;
	definitions.reserve(actions.count());

	for (iterator = actions.begin(); iterator != actions.end(); ++iterator)
	{
		const QVector<QPair<QVariantMap, QVector<QKeySequence> > > actionVariants(iterator.value());

		for (int j = 0; j < actionVariants.count(); ++j)
		{
			KeyboardProfile::Action definition;
			definition.parameters = actionVariants.at(j).first;
			definition.shortcuts = actionVariants.at(j).second;
			definition.action = iterator.key();

			definitions.append(definition);
		}
	}

	profile.setDefinitions({{ActionsManager::GenericContext, definitions}});
	profile.setModified(m_ui->actionsViewWidget->isModified());

	return profile;
}

KeyboardProfileDialog::ValidationResult KeyboardProfileDialog::validateShortcut(const QKeySequence &shortcut, const QModelIndex &index) const
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
		const QModelIndex matchedIndex(indexes.first());
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(matchedIndex.sibling(matchedIndex.row(), 1).data(IdentifierRole).toInt()));

		messages.append(tr("This shortcut already used by %1").arg(definition.isValid() ? definition.getText(true) : tr("unknown action")));

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

bool KeyboardProfileDialog::isModified() const
{
	return m_ui->actionsViewWidget->isModified();
}

}
