/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Piotr Wójcik <chocimier@tlen.pl>
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
#include "../../core/ThemesManager.h"

#include "ui_MouseProfileDialog.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

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
		const QString name(widget->getActionIdentifier());

		model->setData(index, definition.getText(true), Qt::DisplayRole);
		model->setData(index, QStringLiteral("%1 (%2)").arg(definition.getText(true)).arg(name), Qt::ToolTipRole);
		model->setData(index, widget->getActionIdentifier(), MouseProfileDialog::IdentifierRole);
		model->setData(index, name, MouseProfileDialog::NameRole);

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
	widget->setActionIdentifier(index.data(MouseProfileDialog::IdentifierRole).toInt());
	widget->setFocus();

	return widget;
}

MouseProfileDialog::MouseProfileDialog(const QString &profile, const QHash<QString, MouseProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profiles[profile]),
	m_ui(new Ui::MouseProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *gesturesModel(new QStandardItemModel(this));
	const QVector<QPair<GesturesManager::GesturesContext, QString> > contexts({{GesturesManager::GesturesContext::GenericContext, tr("Generic")}, {GesturesManager::GesturesContext::LinkContext, tr("Link")}, {GesturesManager::GesturesContext::ContentEditableContext, tr("Editable Content")}, {GesturesManager::GesturesContext::TabHandleContext, tr("Tab Handle")}, {GesturesManager::GesturesContext::ActiveTabHandleContext, tr("Tab Handle of Active Tab")}, {GesturesManager::GesturesContext::NoTabHandleContext, tr("Empty Area of Tab Bar")}, {GesturesManager::GesturesContext::ToolBarContext, tr("Any Toolbar")}});

	for (int i = 0; i < contexts.count(); ++i)
	{
		QStandardItem *item(new QStandardItem(contexts.at(i).second));
		item->setData(contexts.at(i).first, ContextRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		const QHash<int, QVector<MouseProfile::Gesture> > definitions(m_profile.getDefinitions());

		if (definitions.contains(contexts.at(i).first))
		{
			const QVector<MouseProfile::Gesture> &gestures(definitions[contexts.at(i).first]);

			for (int j = 0; j < gestures.count(); ++j)
			{
				const ActionsManager::ActionDefinition action(ActionsManager::getActionDefinition(gestures.at(j).action));
				const QString name(ActionsManager::getActionName(gestures.at(j).action));
				const QString parameters(gestures.at(j).parameters.isEmpty() ? QString() : QJsonDocument(QJsonObject::fromVariantMap(gestures.at(j).parameters)).toJson(QJsonDocument::Compact));
				QString steps;

				for (int k = 0; k < gestures.at(j).steps.count(); ++k)
				{
					if (k > 0)
					{
						steps += QLatin1String(", ");
					}

					steps += gestures.at(j).steps.at(k).toString();
				}

				QList<QStandardItem*> items({new QStandardItem(action.getText(true)), new QStandardItem(parameters), new QStandardItem(steps)});
				items[0]->setData(QColor(Qt::transparent), Qt::DecorationRole);
				items[0]->setData(action.identifier, IdentifierRole);
				items[0]->setData(name, NameRole);
				items[0]->setData(gestures.at(j).parameters, ParametersRole);
				items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsEditable);
				items[0]->setToolTip(QStringLiteral("%1 (%2)").arg(action.getText(true)).arg(name));
				items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
				items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
				items[2]->setToolTip(parameters);

				if (!action.defaultState.icon.isNull())
				{
					items[0]->setIcon(action.defaultState.icon);
				}

				item->appendRow(items);
			}
		}

		QList<QStandardItem*> items({item, new QStandardItem(), new QStandardItem()});
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		gesturesModel->appendRow(items);
	}

	gesturesModel->setHorizontalHeaderLabels({tr("Context and Action"), tr("Parameters"), tr("Steps")});
	gesturesModel->sort(0);

	QStandardItemModel *stepsModel(new QStandardItemModel(this));
	stepsModel->setHorizontalHeaderLabels({tr("Step")});

	m_ui->gesturesViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->gesturesViewWidget->setModel(gesturesModel);
	m_ui->gesturesViewWidget->setItemDelegateForColumn(0, new GestureActionDelegate(this));
	m_ui->gesturesViewWidget->setFilterRoles({Qt::DisplayRole, NameRole});
	m_ui->gesturesViewWidget->setModified(m_profile.isModified());
	m_ui->stepsViewWidget->setModel(stepsModel);
	m_ui->moveDownStepsButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-down")));
	m_ui->moveUpStepsButton->setIcon(ThemesManager::createIcon(QLatin1String("arrow-up")));
	m_ui->titleLineEditWidget->setText(m_profile.getTitle());
	m_ui->descriptionLineEditWidget->setText(m_profile.getDescription());
	m_ui->versionLineEditWidget->setText(m_profile.getVersion());
	m_ui->authorLineEditWidget->setText(m_profile.getAuthor());

	connect(m_ui->titleLineEditWidget, &QLineEdit::textChanged, m_ui->gesturesViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->descriptionLineEditWidget, &QLineEdit::textChanged, m_ui->gesturesViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->versionLineEditWidget, &QLineEdit::textChanged, m_ui->gesturesViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->authorLineEditWidget, &QLineEdit::textChanged, m_ui->gesturesViewWidget, &ItemViewWidget::markAsModified);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->gesturesViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->gesturesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &MouseProfileDialog::updateGesturesActions);
	connect(m_ui->addGestureButton, &QPushButton::clicked, this, &MouseProfileDialog::addGesture);
	connect(m_ui->removeGestureButton, &QPushButton::clicked, this, &MouseProfileDialog::removeGesture);
	connect(m_ui->stepsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &MouseProfileDialog::updateStepsActions);
	connect(m_ui->stepsViewWidget, &ItemViewWidget::canMoveRowDownChanged, m_ui->moveDownStepsButton, &QToolButton::setEnabled);
	connect(m_ui->stepsViewWidget, &ItemViewWidget::canMoveRowUpChanged, m_ui->moveUpStepsButton, &QToolButton::setEnabled);
	connect(m_ui->addStepButton, &QPushButton::clicked, this, &MouseProfileDialog::addStep);
	connect(m_ui->removeStepButton, &QPushButton::clicked, this, &MouseProfileDialog::removeStep);
	connect(m_ui->moveDownStepsButton, &QToolButton::clicked, m_ui->stepsViewWidget, &ItemViewWidget::moveDownRow);
	connect(m_ui->moveUpStepsButton, &QToolButton::clicked, m_ui->stepsViewWidget, &ItemViewWidget::moveUpRow);
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
		QList<QStandardItem*> items({new QStandardItem(tr("Select Action")), new QStandardItem(), new QStandardItem()});
		items[0]->setData(QColor(Qt::transparent), Qt::DecorationRole);
		items[0]->setData(-1, IdentifierRole);
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsEditable);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		item->appendRow(items);

		m_ui->gesturesViewWidget->setCurrentIndex(items[0]->index());
	}
}

void MouseProfileDialog::removeGesture()
{
	QStandardItem *item(m_ui->gesturesViewWidget->getSourceModel()->itemFromIndex(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0)));

	if (item && item->flags().testFlag(Qt::ItemNeverHasChildren))
	{
		item->parent()->removeRow(item->row());
	}
}

void MouseProfileDialog::addStep()
{
	m_ui->stepsViewWidget->insertRow();
}

void MouseProfileDialog::removeStep()
{
	m_ui->stepsViewWidget->removeRow();
}

void MouseProfileDialog::updateGesturesActions()
{
	const QModelIndex index(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0));
	const bool isGesture(index.flags().testFlag(Qt::ItemNeverHasChildren));

	m_ui->gesturesViewWidget->setCurrentIndex(index.sibling(index.row(), 0));
	m_ui->stepsViewWidget->getSourceModel()->removeRows(0, m_ui->stepsViewWidget->getSourceModel()->rowCount());
	m_ui->addGestureButton->setEnabled(index.isValid());
	m_ui->removeGestureButton->setEnabled(isGesture);

	if (isGesture)
	{
		const QStringList steps(index.sibling(index.row(), 2).data(Qt::DisplayRole).toString().split(QLatin1String(", ")));

		for (int i = 0; i < steps.count(); ++i)
		{
			QStandardItem *item(new QStandardItem(steps.at(i)));
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			m_ui->stepsViewWidget->getSourceModel()->appendRow(item);
		}
	}

	updateStepsActions();
}

void MouseProfileDialog::updateStepsActions()
{
	QStandardItem *item(m_ui->gesturesViewWidget->getSourceModel()->itemFromIndex(m_ui->gesturesViewWidget->currentIndex().sibling(m_ui->gesturesViewWidget->currentIndex().row(), 0)));
	const bool isGesture(item && item->flags().testFlag(Qt::ItemNeverHasChildren));

	item = m_ui->stepsViewWidget->getSourceModel()->itemFromIndex(m_ui->stepsViewWidget->currentIndex().sibling(m_ui->stepsViewWidget->currentIndex().row(), 0));

	m_ui->addStepButton->setEnabled(isGesture);
	m_ui->removeStepButton->setEnabled(isGesture && item);

	if (isGesture && item)
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

		m_ui->gesturesViewWidget->setData(item->index().sibling(item->index().row(), 2), steps.join(QLatin1String(", ")), Qt::DisplayRole);
	}
}

MouseProfile MouseProfileDialog::getProfile() const
{
	MouseProfile profile(m_profile);
	profile.setTitle(m_ui->titleLineEditWidget->text());
	profile.setDescription(m_ui->descriptionLineEditWidget->text());
	profile.setVersion(m_ui->versionLineEditWidget->text());
	profile.setAuthor(m_ui->authorLineEditWidget->text());

	QHash<int, QVector<MouseProfile::Gesture> > definitions;

	for (int i = 0; i < m_ui->gesturesViewWidget->getRowCount(); ++i)
	{
		const QModelIndex contextIndex(m_ui->gesturesViewWidget->getIndex(i, 0));
		const int gestureAmount(m_ui->gesturesViewWidget->getRowCount(contextIndex));

		if (gestureAmount > 0)
		{
			QVector<MouseProfile::Gesture> gestures;
			gestures.reserve(gestureAmount);

			for (int j = 0; j < gestureAmount; ++j)
			{
				const QModelIndex actionIndex(contextIndex.child(j, 0));
				const QStringList steps(actionIndex.sibling(actionIndex.row(), 2).data(Qt::DisplayRole).toString().split(QLatin1String(", "), QString::SkipEmptyParts));
				const int action(actionIndex.data(IdentifierRole).toInt());

				if (!steps.isEmpty() && action >= 0)
				{
					MouseProfile::Gesture gesture;
					gesture.action = action;
					gesture.parameters = actionIndex.data(ParametersRole).toMap();

					for (int k = 0; k < steps.count(); ++k)
					{
						gesture.steps.append(MouseProfile::Gesture::Step::fromString(steps.at(k)));
					}

					gestures.append(gesture);
				}
			}

			gestures.squeeze();

			if (gestures.count() > 0)
			{
				definitions[static_cast<GesturesManager::GesturesContext>(contextIndex.data(ContextRole).toInt())] = gestures;
			}
		}
	}

	profile.setDefinitions(definitions);
	profile.setModified(m_ui->gesturesViewWidget->isModified());

	return profile;
}

bool MouseProfileDialog::isModified() const
{
	return m_ui->gesturesViewWidget->isModified();
}

}
