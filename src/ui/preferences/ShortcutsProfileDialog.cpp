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

ShortcutsProfileDialog::ShortcutsProfileDialog(const QString &profile, const QHash<QString, ShortcutsProfile> &profiles, QWidget *parent) : QDialog(parent),
	m_profile(profile),
	m_isModified(profiles[profile].isModified),
	m_ui(new Ui::ShortcutsProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model = new QStandardItemModel(this);
	const QVector<ActionDefinition> definitions = ActionsManager::getActionDefinitions();
	QStringList labels;
	labels << tr("Action");

	for (int i = 0; i < definitions.count(); ++i)
	{
		QStandardItem *item = new QStandardItem(definitions.at(i).icon, QCoreApplication::translate("actions", (definitions.at(i).description.isEmpty() ? definitions.at(i).text : definitions.at(i).description).toUtf8().constData()));
		item->setData(definitions.at(i).identifier, Qt::UserRole);
		item->setToolTip(ActionsManager::getActionName(definitions.at(i).identifier));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (profiles[profile].shortcuts.contains(definitions.at(i).identifier))
		{
			QStringList shortcuts;

			for (int j = 0; j < profiles[profile].shortcuts[definitions.at(i).identifier].count(); ++j)
			{
				shortcuts.append(profiles[profile].shortcuts[definitions.at(i).identifier].at(j).toString());
			}

			item->setData(shortcuts.join(QLatin1Char(' ')), (Qt::UserRole + 1));
		}

		model->appendRow(item);
	}

	model->setHorizontalHeaderLabels(labels);
	model->sort(0);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->shortcutsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->shortcutsViewWidget->setItemDelegate(new KeyboardShortcutDelegate(this));
	m_ui->titleLineEdit->setText(profiles[profile].title);
	m_ui->descriptionLineEdit->setText(profiles[profile].description);
	m_ui->versionLineEdit->setText(profiles[profile].version);
	m_ui->authorLineEdit->setText(profiles[profile].author);

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

	m_isModified = true;
}

void ShortcutsProfileDialog::removeShortcut()
{
	m_ui->shortcutsViewWidget->removeRow();

	saveShortcuts();

	m_isModified = true;
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

ShortcutsProfile ShortcutsProfileDialog::getProfile() const
{
	ShortcutsProfile profile;
	profile.title = m_ui->titleLineEdit->text();
	profile.description = m_ui->descriptionLineEdit->text();
	profile.version = m_ui->versionLineEdit->text();
	profile.author = m_ui->authorLineEdit->text();
	profile.isModified = m_isModified;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		const QStringList rawShortcuts = m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::UserRole + 1).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);

		if (!rawShortcuts.isEmpty())
		{
			QVector<QKeySequence> shortcuts;

			for (int j = 0; j < rawShortcuts.count(); ++j)
			{
				shortcuts.append(QKeySequence(rawShortcuts.at(j)));
			}

			profile.shortcuts[m_ui->actionsViewWidget->getIndex(i, 0).data(Qt::UserRole).toInt()] = shortcuts;
		}
	}

	return profile;
}

}
