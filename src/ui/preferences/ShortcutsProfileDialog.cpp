/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

ShortcutsProfileDialog::ShortcutsProfileDialog(const QHash<QString, QString> &information, const QHash<QString, QVariantHash> &data, const QHash<QString, QList<QKeySequence> > &shortcuts, QWidget *parent) : QDialog(parent),
	m_shortcuts(shortcuts),
	m_ui(new Ui::ShortcutsProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model = new QStandardItemModel(this);
	const QList<ActionDefinition> actions = ActionsManager::getActionDefinitions();
	QStringList labels;
	labels << tr("Action");

	for (int i = 0; i < actions.count(); ++i)
	{
		const QString name = ActionsManager::getActionName(actions.at(i).identifier);
		QList<QStandardItem*> items;
		items.append(new QStandardItem(actions.at(i).icon, (actions.at(i).description.isEmpty() ? actions.at(i).text : actions.at(i).description)));
		items[0]->setData(name, Qt::UserRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (data.contains(name))
		{
			items[0]->setData(data[name].value(QLatin1String("shortcuts"), QString()), (Qt::UserRole + 1));
		}

		model->appendRow(items);
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

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->actionsViewWidget, SLOT(setFilter(QString)));
	connect(m_ui->actionsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateActionsActions()));
	connect(m_ui->shortcutsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateShortcutsActions()));
	connect(m_ui->addShortcutButton, SIGNAL(clicked()), this, SLOT(addShortcut()));
	connect(m_ui->removeShortcutButton, SIGNAL(clicked()), this, SLOT(removeShortcut()));
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

void ShortcutsProfileDialog::addShortcut()
{
	QList<QStandardItem*> items;
	items.append(new QStandardItem(QString()));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

	m_ui->shortcutsViewWidget->insertRow(items);
}

void ShortcutsProfileDialog::removeShortcut()
{
	m_ui->shortcutsViewWidget->removeRow();

	saveShortcuts();
}

void ShortcutsProfileDialog::updateActionsActions()
{
	disconnect(m_ui->shortcutsViewWidget->getModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveShortcuts()));

	m_ui->shortcutsViewWidget->getModel()->clear();

	QStringList labels;
	labels << tr("Shortcut");

	m_currentAction = m_ui->actionsViewWidget->getIndex(m_ui->actionsViewWidget->getCurrentRow(), 0);

	m_ui->shortcutsViewWidget->getModel()->setHorizontalHeaderLabels(labels);

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
		const QKeySequence shortcut(rawShortcuts.at(i));

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

		if (!hash[QLatin1String("shortcuts")].isNull())
		{
			data[m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::UserRole).toString()] = hash;
		}
	}

	return data;
}

}
