/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "KeyboardProfileDialog.h"
#include "../../core/ActionsManager.h"

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

	return new QKeySequenceEdit(QKeySequence(index.data().toString()), parent);
}

KeyboardProfileDialog::KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profile),
	m_isModified(profiles[profile].isModified),
	m_ui(new Ui::KeyboardProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(QCoreApplication::translate("actions", (definitions.at(i).description.isEmpty() ? definitions.at(i).text : definitions.at(i).description).toUtf8().constData())));
		item->setData(QColor(Qt::transparent), Qt::DecorationRole);
		item->setData(definitions.at(i).identifier, IdentifierRole);
		item->setToolTip(ActionsManager::getActionName(definitions.at(i).identifier));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		if (!definitions.at(i).icon.isNull())
		{
			item->setIcon(definitions.at(i).icon);
		}

		if (profiles[profile].shortcuts.contains(definitions.at(i).identifier))
		{
			QStringList shortcuts;

			for (int j = 0; j < profiles[profile].shortcuts[definitions.at(i).identifier].count(); ++j)
			{
				shortcuts.append(profiles[profile].shortcuts[definitions.at(i).identifier].at(j).toString());
			}

			item->setData(shortcuts.join(QLatin1Char(' ')), ShortcutsRole);
		}

		model->appendRow(item);
	}

	model->setHorizontalHeaderLabels(QStringList(tr("Action")));
	model->sort(0);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->shortcutsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->shortcutsViewWidget->setItemDelegate(new KeyboardShortcutDelegate(this));
	m_ui->titleLineEdit->setText(profiles[profile].title);
	m_ui->descriptionLineEdit->setText(profiles[profile].description);
	m_ui->versionLineEdit->setText(profiles[profile].version);
	m_ui->authorLineEdit->setText(profiles[profile].author);

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
	QList<QStandardItem*> items({new QStandardItem()});
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

	m_ui->shortcutsViewWidget->insertRow(items);

	m_isModified = true;
}

void KeyboardProfileDialog::removeShortcut()
{
	m_ui->shortcutsViewWidget->removeRow();

	saveShortcuts();

	m_isModified = true;
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
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);

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
	profile.title = m_ui->titleLineEdit->text();
	profile.description = m_ui->descriptionLineEdit->text();
	profile.version = m_ui->versionLineEdit->text();
	profile.author = m_ui->authorLineEdit->text();
	profile.isModified = m_isModified;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		const QStringList rawShortcuts(m_ui->actionsViewWidget->getIndex(i, 0).data(ShortcutsRole).toString().split(QLatin1Char(' '), QString::SkipEmptyParts));

		if (!rawShortcuts.isEmpty())
		{
			QVector<QKeySequence> shortcuts;

			for (int j = 0; j < rawShortcuts.count(); ++j)
			{
				shortcuts.append(QKeySequence(rawShortcuts.at(j)));
			}

			profile.shortcuts[m_ui->actionsViewWidget->getIndex(i, 0).data(IdentifierRole).toInt()] = shortcuts;
		}
	}

	return profile;
}

}
