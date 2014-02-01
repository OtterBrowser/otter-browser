/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ShortcutsProfileDialog.h"
#include "KeyboardShortcutDelegate.h"
#include "../../core/ActionsManager.h"

#include "ui_ShortcutsProfileDialog.h"

#include <QtWidgets/QInputDialog>

namespace Otter
{

ShortcutsProfileDialog::ShortcutsProfileDialog(const QHash<QString, QString> &information, const QHash<QString, QVariantHash> &data, const QHash<QString, QList<QKeySequence> > &shortcuts, bool macrosMode, QWidget *parent) : QDialog(parent),
	m_shortcuts(shortcuts),
	m_macrosMode(macrosMode),
	m_ui(new Ui::ShortcutsProfileDialog)
{
	m_ui->setupUi(this);

	QStringList labels;
	QStandardItemModel *model = new QStandardItemModel(this);

	if (m_macrosMode)
	{
		labels << tr("Macro") << tr("Actions");

		QHash<QString, QVariantHash>::const_iterator iterator;

		for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator)
		{
			QList<QStandardItem*> items;
			items.append(new QStandardItem(iterator.value().value(QLatin1String("title"), tr("(Untitled)")).toString()));
			items[0]->setData(iterator.key(), Qt::UserRole);
			items[0]->setData(iterator.value().value(QLatin1String("shortcuts")), (Qt::UserRole + 1));
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			items.append(new QStandardItem(iterator.value().value(QLatin1String("actions"), QStringList()).toStringList().join(QLatin1String(", "))));
			items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);

			model->appendRow(items);
		}

		m_ui->addMacroButton->setEnabled(true);

		connect(m_ui->addMacroButton, SIGNAL(clicked()), this, SLOT(addMacro()));
		connect(m_ui->removeMacroButton, SIGNAL(clicked()), m_ui->actionsViewWidget, SLOT(removeRow()));
	}
	else
	{
		labels << tr("Action");

		const QStringList actions = ActionsManager::getIdentifiers();

		for (int i = 0; i < actions.count(); ++i)
		{
			QAction *action = ActionsManager::getAction(actions.at(i));

			if (!action || action->isSeparator())
			{
				continue;
			}

			QList<QStandardItem*> items;
			items.append(new QStandardItem(action->icon(), action->text()));
			items[0]->setData(actions.at(i), Qt::UserRole);
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

			if (data.contains(actions.at(i)))
			{
				items[0]->setData(data[actions.at(i)].value(QLatin1String("shortcuts"), QString()), (Qt::UserRole + 1));
			}

			model->appendRow(items);
		}

		m_ui->macrosWidget->hide();
	}

	model->setHorizontalHeaderLabels(labels);
	model->sort(0);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->shortcutsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->shortcutsViewWidget->setItemDelegate(new KeyboardShortcutDelegate(this));
	m_ui->titleLineEdit->setText(information.value(QLatin1String("Title"), QString()));
	m_ui->descriptionLineEdit->setText(information.value(QLatin1String("Description"), QString()));
	m_ui->versionLineEdit->setText(information.value(QLatin1String("Version"), QString()));
	m_ui->authorLineEdit->setText(information.value(QLatin1String("Author"), QString()));

	updateMacrosActions();

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->actionsViewWidget, SLOT(setFilter(QString)));
	connect(m_ui->actionsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateMacrosActions()));
	connect(m_ui->shortcutsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateShortcutsActions()));
	connect(m_ui->addShortcutButton, SIGNAL(clicked()), this, SLOT(addShortcut()));
	connect(m_ui->removeShortcutButton, SIGNAL(clicked()), m_ui->shortcutsViewWidget, SLOT(removeRow()));
}

ShortcutsProfileDialog::~ShortcutsProfileDialog()
{
	delete m_ui;
}

void ShortcutsProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void ShortcutsProfileDialog::addMacro()
{
//FIXME create list from profiles, like shortcuts
	const QStringList identifiers = ActionsManager::getIdentifiers();
	QString identifier;

	do
	{
		identifier = QInputDialog::getText(this, tr("Select Identifier"), tr("Input Unique Macro Identifier:"));

		if (identifier.isEmpty())
		{
			return;
		}
	}
	while (identifiers.contains(identifier));

	QList<QStandardItem*> items;
	items.append(new QStandardItem(tr("(Untitled)")));
	items[0]->setData(identifier, Qt::UserRole);
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	items.append(new QStandardItem(QString()));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);

	m_ui->actionsViewWidget->insertRow(items);
}

void ShortcutsProfileDialog::addShortcut()
{
	QList<QStandardItem*> items;
	items.append(new QStandardItem(QString()));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

	m_ui->shortcutsViewWidget->insertRow(items);
}

void ShortcutsProfileDialog::updateMacrosActions()
{
	disconnect(m_ui->shortcutsViewWidget->getModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveShortcuts()));

	m_ui->shortcutsViewWidget->getModel()->clear();

	QStringList labels;
	labels << tr("Shortcut");

	m_currentAction = m_ui->actionsViewWidget->getIndex(m_ui->actionsViewWidget->getCurrentRow(), 0);

	m_ui->shortcutsViewWidget->getModel()->setHorizontalHeaderLabels(labels);
	m_ui->removeMacroButton->setEnabled(m_macrosMode && m_currentAction.isValid());

	if (!m_currentAction.isValid())
	{
		m_ui->addShortcutButton->setEnabled(false);
		m_ui->removeShortcutButton->setEnabled(true);

		return;
	}

	updateShortcutsActions();

	m_ui->addShortcutButton->setEnabled(true);

	const QStringList rawShortcuts = m_currentAction.data(Qt::UserRole + 1).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);

	for (int i = 0; i < rawShortcuts.count(); ++i)
	{
		const QKeySequence shortcut = ((rawShortcuts.at(i) == QLatin1String("native")) ? ActionsManager::getNativeShortcut(m_currentAction.data(Qt::UserRole).toString()) : QKeySequence(rawShortcuts.at(i)));

		if (!shortcut.isEmpty())
		{
			QList<QStandardItem*> items;
			items.append(new QStandardItem(shortcut.toString()));
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

			m_ui->shortcutsViewWidget->getModel()->appendRow(items);
		}
	}

	connect(m_ui->shortcutsViewWidget->getModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveShortcuts()));
}

void ShortcutsProfileDialog::updateShortcutsActions()
{
	m_ui->removeShortcutButton->setEnabled(m_ui->shortcutsViewWidget->getCurrentRow() >= 0);
}

void ShortcutsProfileDialog::saveShortcuts()
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

	m_ui->actionsViewWidget->setData(m_currentAction, shortcuts.join(QLatin1Char(' ')), (Qt::UserRole + 1));
}

QHash<QString, QString> ShortcutsProfileDialog::getInformation() const
{
	QHash<QString, QString> information;
	information[QLatin1String("Title")] = m_ui->titleLineEdit->text();
	information[QLatin1String("Description")] = m_ui->descriptionLineEdit->text();
	information[QLatin1String("Version")] = m_ui->versionLineEdit->text();
	information[QLatin1String("Author")] = m_ui->authorLineEdit->text();

	return information;
}

QHash<QString, QVariantHash> ShortcutsProfileDialog::getData() const
{
	QHash<QString, QVariantHash> data;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		QVariantHash hash;
		hash[QLatin1String("shortcuts")] = m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::UserRole + 1).toString();

		if (m_macrosMode)
		{
			hash[QLatin1String("title")] = m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::DisplayRole);
			hash[QLatin1String("actions")] = m_ui->actionsViewWidget->getIndex(i, 1).data(Qt::DisplayRole).toStringList();
		}
		else if (hash[QLatin1String("shortcuts")].isNull())
		{
			continue;
		}

		data[m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::UserRole).toString()] = hash;
	}

	return data;
}

}
