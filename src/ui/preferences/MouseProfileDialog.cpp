/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "MouseProfileDialog.h"
#include "../ActionComboBoxWidget.h"
#include "../../core/ActionsManager.h"

#include "ui_MouseProfileDialog.h"

namespace Otter
{

GestureActionDelegate::GestureActionDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void GestureActionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	ActionComboBoxWidget *widget(qobject_cast<ActionComboBoxWidget*>(editor));

	if (widget && widget->getActionIdentifier() >= 0)
	{
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(widget->getActionIdentifier()));

		model->setData(index, definition.getText(true), Qt::DisplayRole);
		model->setData(index, widget->getActionIdentifier(), Qt::UserRole);

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

QWidget* GestureActionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ActionComboBoxWidget *widget(new ActionComboBoxWidget(parent));
	widget->setActionIdentifier(index.data(Qt::UserRole).toInt());
	widget->setFocus();

	return widget;
}

MouseProfileDialog::MouseProfileDialog(const QString &profile, const QHash<QString, MouseProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profile),
	m_isModified(profiles[profile].isModified),
	m_ui(new Ui::MouseProfileDialog)
{
	m_ui->setupUi(this);
	m_ui->gesturesViewWidget->setItemDelegateForColumn(1, new GestureActionDelegate(this));

	QStandardItemModel *gesturesModel(new QStandardItemModel(this));
	const QVector<QPair<QString, QString> > contexts({{QLatin1String("Generic"), tr("Generic")}, {QLatin1String("Link"), tr("Link")}, {QLatin1String("ContentEditable"), tr("Editable Content")}, {QLatin1String("TabHandle"), tr("Tab Handle")}, {QLatin1String("ActiveTabHandle"), tr("Tab Handle of Active Tab")}, {QLatin1String("NoTabHandle"), tr("Empty Area of Tab Bar")}, {QLatin1String("ToolBar"), tr("Any Toolbar")}});

	for (int i = 0; i < contexts.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(contexts.at(i).second));
		item->setData(contexts.at(i).first, Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (profiles[profile].gestures.contains(contexts.at(i).first))
		{
			QHash<QString, int>::const_iterator iterator;

			for (iterator = profiles[profile].gestures[contexts.at(i).first].constBegin(); iterator != profiles[profile].gestures[contexts.at(i).first].constEnd(); ++iterator)
			{
				const ActionsManager::ActionDefinition action(ActionsManager::getActionDefinition(iterator.value()));
				QList<QStandardItem*> items({new QStandardItem(QString(iterator.key()).replace(QLatin1Char(','), QLatin1String(", "))), new QStandardItem(action.getText(true))});
				items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
				items[1]->setData(QColor(Qt::transparent), Qt::DecorationRole);
				items[1]->setData(action.identifier, Qt::UserRole);
				items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsEditable);

				if (!action.defaultState.icon.isNull())
				{
					items[1]->setIcon(action.defaultState.icon);
				}

				item->appendRow(items);
			}
		}

		QList<QStandardItem*> items({item, new QStandardItem()});
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		gesturesModel->appendRow(items);
	}

	gesturesModel->setHorizontalHeaderLabels(QStringList({tr("Context and Steps"), tr("Action")}));
	gesturesModel->sort(0);

	QStandardItemModel *stepsModel(new QStandardItemModel(this));
	stepsModel->setHorizontalHeaderLabels(QStringList(tr("Step")));

	m_ui->gesturesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->gesturesViewWidget->setModel(gesturesModel);
	m_ui->stepsViewWidget->setModel(stepsModel);
	m_ui->titleLineEdit->setText(profiles[profile].title);
	m_ui->descriptionLineEdit->setText(profiles[profile].description);
	m_ui->versionLineEdit->setText(profiles[profile].version);
	m_ui->authorLineEdit->setText(profiles[profile].author);

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->gesturesViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->gesturesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateGesturesActions()));
	connect(m_ui->addGestureButton, SIGNAL(clicked()), this, SLOT(addGesture()));
	connect(m_ui->removeGestureButton, SIGNAL(clicked()), this, SLOT(removeGesture()));
	connect(m_ui->stepsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateStepsActions()));
	connect(m_ui->addStepButton, SIGNAL(clicked()), this, SLOT(addStep()));
	connect(m_ui->removeStepButton, SIGNAL(clicked()), this, SLOT(removeStep()));
}

MouseProfileDialog::~MouseProfileDialog()
{
	delete m_ui;
}

void MouseProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void MouseProfileDialog::addGesture()
{
	QStandardItem *item(m_ui->gesturesViewWidget->getSourceModel()->itemFromIndex(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0)));

	if (item && item->flags().testFlag(Qt::ItemNeverHasChildren))
	{
		item = item->parent();
	}

	if (item)
	{
		QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(tr("Select Action"))});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setData(QColor(Qt::transparent), Qt::DecorationRole);
		items[1]->setData(-1, Qt::UserRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsEditable);

		item->appendRow(items);

		m_ui->gesturesViewWidget->setCurrentIndex(items[0]->index());

		m_isModified = true;
	}
}

void MouseProfileDialog::removeGesture()
{
	QStandardItem *item(m_ui->gesturesViewWidget->getSourceModel()->itemFromIndex(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0)));

	if (item && item->flags().testFlag(Qt::ItemNeverHasChildren))
	{
		item->parent()->removeRow(item->row());

		m_isModified = true;
	}
}

void MouseProfileDialog::saveGesture()
{
	QStringList steps;

	for (int i = 0; i < m_ui->stepsViewWidget->getRowCount(); ++i)
	{
		const QString step(m_ui->stepsViewWidget->getIndex(i, 0).data().toString());

		if (!step.isEmpty())
		{
			steps.append(step);
		}
	}

	const QModelIndex index(m_ui->gesturesViewWidget->currentIndex());

	m_ui->gesturesViewWidget->setData(index.sibling(index.row(), 0), steps.join(QLatin1String(", ")), Qt::DisplayRole);

	m_isModified = true;
}

void MouseProfileDialog::addStep()
{
	m_ui->stepsViewWidget->insertRow();

	m_isModified = true;
}

void MouseProfileDialog::removeStep()
{
	m_ui->stepsViewWidget->removeRow();

	saveGesture();

	m_isModified = true;
}

void MouseProfileDialog::updateGesturesActions()
{
	disconnect(m_ui->stepsViewWidget->getSourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveGesture()));

	const QModelIndex index(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0));
	const bool isGesture(index.flags().testFlag(Qt::ItemNeverHasChildren));

	m_ui->gesturesViewWidget->setCurrentIndex(index.sibling(index.row(), 1));
	m_ui->stepsViewWidget->getSourceModel()->removeRows(0, m_ui->stepsViewWidget->getSourceModel()->rowCount());
	m_ui->addGestureButton->setEnabled(index.isValid());
	m_ui->removeGestureButton->setEnabled(isGesture);

	if (isGesture)
	{
		const QStringList steps(index.data(Qt::DisplayRole).toString().split(QLatin1String(", ")));

		for (int i = 0; i < steps.count(); ++i)
		{
			QStandardItem *item(new QStandardItem(steps.at(i)));
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			m_ui->stepsViewWidget->getSourceModel()->appendRow(item);
		}
	}

	updateStepsActions();

	connect(m_ui->stepsViewWidget->getSourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(saveGesture()));
}

void MouseProfileDialog::updateStepsActions()
{
	QStandardItem *item(m_ui->gesturesViewWidget->getSourceModel()->itemFromIndex(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0)));
	const bool isGesture(item && item->flags().testFlag(Qt::ItemNeverHasChildren));

	item = m_ui->stepsViewWidget->getSourceModel()->itemFromIndex(m_ui->stepsViewWidget->currentIndex().sibling(m_ui->stepsViewWidget->currentIndex().row(), 0));

	m_ui->addStepButton->setEnabled(isGesture);
	m_ui->removeStepButton->setEnabled(isGesture && item);
}

MouseProfile MouseProfileDialog::getProfile() const
{
	MouseProfile profile;
	profile.title = m_ui->titleLineEdit->text();
	profile.description = m_ui->descriptionLineEdit->text();
	profile.version = m_ui->versionLineEdit->text();
	profile.author = m_ui->authorLineEdit->text();
	profile.isModified = m_isModified;

	for (int i = 0; i < m_ui->gesturesViewWidget->getSourceModel()->rowCount(); ++i)
	{
		QStandardItem *contextItem(m_ui->gesturesViewWidget->getSourceModel()->item(i, 0));

		if (contextItem && contextItem->rowCount() > 0)
		{
			QHash<QString, int> gestures;

			for (int j = 0; j < contextItem->rowCount(); ++j)
			{
				if (!contextItem->child(j, 0) || !contextItem->child(j, 1))
				{
					continue;
				}

				const QStringList steps(contextItem->child(j, 0)->data(Qt::DisplayRole).toString().split(QLatin1String(", "), QString::SkipEmptyParts));
				const int action(contextItem->child(j, 1)->data(Qt::UserRole).toInt());

				if (!steps.isEmpty() && action >= 0)
				{
					gestures[steps.join(QLatin1Char(','))] = action;
				}
			}

			if (gestures.count() > 0)
			{
				profile.gestures[contextItem->data(Qt::UserRole).toString()] = gestures;
			}
		}
	}

	return profile;
}

}
