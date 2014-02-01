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
#include "../../core/ActionsManager.h"

#include "ui_ShortcutsProfileDialog.h"

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
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			items.append(new QStandardItem(iterator.value().value(QLatin1String("actions"), QStringList()).toStringList().join(QLatin1String(", "))));
			items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);

			model->appendRow(items);
		}
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
	m_ui->titleLineEdit->setText(information.value(QLatin1String("Title"), QString()));
	m_ui->descriptionLineEdit->setText(information.value(QLatin1String("Description"), QString()));
	m_ui->versionLineEdit->setText(information.value(QLatin1String("Version"), QString()));
	m_ui->authorLineEdit->setText(information.value(QLatin1String("Author"), QString()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->actionsViewWidget, SLOT(setFilter(QString)));
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
