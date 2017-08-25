/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ui_KeyboardProfileDialog.h"

#include <QtWidgets/QInputDialog>
#include <QtWidgets/QKeySequenceEdit>

namespace Otter
{

KeyboardShortcutDelegate::KeyboardShortcutDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void KeyboardShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QKeySequenceEdit *widget(qobject_cast<QKeySequenceEdit*>(editor));

	if (widget)
	{
		model->setData(index, widget->keySequence().toString());
	}
}

QWidget* KeyboardShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QKeySequenceEdit *widget(new QKeySequenceEdit(QKeySequence(index.data().toString()), parent));
	widget->setFocus();

	return widget;
}

KeyboardProfileDialog::KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profiles[profile]),
	m_ui(new Ui::KeyboardProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<KeyboardProfile::Action> definitions(m_profile.getDefinitions().value(ActionsManager::GenericContext));

	for (int i = 0; i < definitions.count(); ++i)
	{
		const ActionsManager::ActionDefinition action(ActionsManager::getActionDefinition(definitions.at(i).action));
		QStandardItem *item(new QStandardItem(action.getText(true)));
		item->setData(QColor(Qt::transparent), Qt::DecorationRole);
		item->setData(definitions.at(i).action, IdentifierRole);
		item->setData(definitions.at(i).parameters, ParametersRole);
		item->setToolTip(ActionsManager::getActionName(definitions.at(i).action));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		if (!action.defaultState.icon.isNull())
		{
			item->setIcon(action.defaultState.icon);
		}

		QStringList shortcuts;

		for (int j = 0; j < definitions.at(i).shortcuts.count(); ++j)
		{
			shortcuts.append(definitions.at(i).shortcuts.at(j).toString());
		}

		item->setData(shortcuts.join(QLatin1Char(' ')), ShortcutsRole);

		model->appendRow(item);
	}

	model->setHorizontalHeaderLabels(QStringList(tr("Action")));
	model->sort(0);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->shortcutsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->shortcutsViewWidget->setItemDelegate(new KeyboardShortcutDelegate(this));
	m_ui->shortcutsViewWidget->setModified(m_profile.isModified());
	m_ui->titleLineEdit->setText(m_profile.getTitle());
	m_ui->descriptionLineEdit->setText(m_profile.getDescription());
	m_ui->versionLineEdit->setText(m_profile.getVersion());
	m_ui->authorLineEdit->setText(m_profile.getAuthor());

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->actionsViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->actionsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActionsActions()));
	connect(m_ui->shortcutsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateShortcutsActions()));
	connect(m_ui->addShortcutButton, SIGNAL(clicked()), this, SLOT(addShortcut()));
	connect(m_ui->removeShortcutButton, SIGNAL(clicked()), this, SLOT(removeShortcut()));
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
	}
}

void KeyboardProfileDialog::addShortcut()
{
	QStandardItem* item(new QStandardItem());
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

	m_ui->shortcutsViewWidget->insertRow(item);
}

void KeyboardProfileDialog::removeShortcut()
{
	m_ui->shortcutsViewWidget->removeRow();

	saveShortcuts();
}

void KeyboardProfileDialog::updateActionsActions()
{
	disconnect(m_ui->shortcutsViewWidget->getSourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveShortcuts()));

	m_ui->shortcutsViewWidget->getSourceModel()->clear();

	m_currentAction = m_ui->actionsViewWidget->getIndex(m_ui->actionsViewWidget->getCurrentRow(), 0);

	m_ui->shortcutsViewWidget->getSourceModel()->setHorizontalHeaderLabels(QStringList(tr("Shortcut")));

	if (!m_currentAction.isValid())
	{
		m_ui->addShortcutButton->setEnabled(false);
		m_ui->removeShortcutButton->setEnabled(true);

		return;
	}

	updateShortcutsActions();

	m_ui->addShortcutButton->setEnabled(true);

	const QStringList rawShortcuts(m_currentAction.data(ShortcutsRole).toString().split(QLatin1Char(' '), QString::SkipEmptyParts));

	for (int i = 0; i < rawShortcuts.count(); ++i)
	{
		const QKeySequence shortcut(rawShortcuts.at(i));

		if (!shortcut.isEmpty())
		{
			QList<QStandardItem*> items({new QStandardItem(shortcut.toString())});
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemNeverHasChildren);

			m_ui->shortcutsViewWidget->getSourceModel()->appendRow(items);
		}
	}

	connect(m_ui->shortcutsViewWidget->getSourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveShortcuts()));
}

void KeyboardProfileDialog::updateShortcutsActions()
{
	m_ui->removeShortcutButton->setEnabled(m_ui->shortcutsViewWidget->getCurrentRow() >= 0);
}

void KeyboardProfileDialog::saveShortcuts()
{
	if (!m_currentAction.isValid())
	{
		return;
	}

	QStringList shortcuts;

	for (int i = 0; i < m_ui->shortcutsViewWidget->getRowCount(); ++i)
	{
		const QKeySequence shortcut(m_ui->shortcutsViewWidget->getIndex(i, 0).data().toString());

		if (!shortcut.isEmpty())
		{
			shortcuts.append(shortcut.toString());
		}
	}

	m_ui->actionsViewWidget->setData(m_currentAction, shortcuts.join(QLatin1Char(' ')), ShortcutsRole);
}

KeyboardProfile KeyboardProfileDialog::getProfile() const
{
	KeyboardProfile profile;
	profile.setTitle(m_ui->titleLineEdit->text());
	profile.setDescription(m_ui->descriptionLineEdit->text());
	profile.setVersion(m_ui->versionLineEdit->text());
	profile.setAuthor(m_ui->authorLineEdit->text());

	QVector<KeyboardProfile::Action> definitions;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		const QStringList rawShortcuts(m_ui->actionsViewWidget->getIndex(i, 0).data(ShortcutsRole).toString().split(QLatin1Char(' '), QString::SkipEmptyParts));

		if (!rawShortcuts.isEmpty())
		{
			KeyboardProfile::Action definition;
			definition.parameters = m_ui->actionsViewWidget->getIndex(i, 0).data(ParametersRole).toMap();
			definition.shortcuts.reserve(rawShortcuts.count());
			definition.action = m_ui->actionsViewWidget->getIndex(i, 0).data(IdentifierRole).toInt();

			for (int j = 0; j < rawShortcuts.count(); ++j)
			{
				definition.shortcuts.append(QKeySequence(rawShortcuts.at(j)));
			}

			definitions.append(definition);
		}
	}

	profile.setDefinitions({{ActionsManager::GenericContext, definitions}});
	profile.setModified(m_ui->actionsViewWidget->isModified());

	return profile;
}

bool KeyboardProfileDialog::isModified() const
{
	return m_ui->actionsViewWidget->isModified();
}

}
