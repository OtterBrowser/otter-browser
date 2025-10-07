/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2019 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionsContentsWidget.h"
#include "../../../core/ItemModel.h"
#include "../../../core/ThemesManager.h"

#include "ui_ActionsContentsWidget.h"

namespace Otter
{

ActionsContentsWidget::ActionsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::ActionsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->actionsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->actionsViewWidget->setModel(m_model);
	m_ui->actionsViewWidget->setFilterRoles({Qt::DisplayRole, ActionRole, ParametersRole});

	populateActions();

	connect(m_ui->actionsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ActionsContentsWidget::updateActions);
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::setFilterString);
	connect(ActionsManager::getInstance(), &ActionsManager::shortcutsChanged, this, &ActionsContentsWidget::populateActions);
}

ActionsContentsWidget::~ActionsContentsWidget()
{
	delete m_ui;
}

void ActionsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Action"), tr("Shortcuts"), tr("Gestures")});
	}
}

void ActionsContentsWidget::print(QPrinter *printer)
{
	m_ui->actionsViewWidget->render(printer);
}

void ActionsContentsWidget::populateActions()
{
	m_model->clear();
	m_model->setHorizontalHeaderLabels({tr("Action"), tr("Shortcuts"), tr("Gestures")});

	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		const ActionsManager::ActionDefinition definition(definitions.at(i));

		if (definition.flags.testFlag(ActionsManager::ActionDefinition::IsDeprecatedFlag) || definition.flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters))
		{
			continue;
		}

		const QString actionName(ActionsManager::getActionName(definition.identifier));
		QList<QStandardItem*> items({new QStandardItem(definition.getText(true)), new QStandardItem(QKeySequence::listToString(ActionsManager::getActionShortcuts(definition.identifier).toList(), QKeySequence::NativeText)), new QStandardItem()});
		items[0]->setData(ItemModel::createDecoration(definition.defaultState.icon), Qt::DecorationRole);
		items[0]->setData(definition.identifier, IdentifierRole);
		items[0]->setData(actionName, ActionRole);
		items[0]->setToolTip(QStringLiteral("%1 (%2)").arg(items[0]->text(), actionName));
		items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
		items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
		items[2]->setFlags(items[2]->flags() | Qt::ItemNeverHasChildren);

		m_model->appendRow(items);
	}
}

void ActionsContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->actionsViewWidget->getCurrentIndex().sibling(m_ui->actionsViewWidget->getCurrentRow(), 0));

	if (index.isValid())
	{
		m_ui->actionLabelWidget->setText(index.data(Qt::DisplayRole).toString());
		m_ui->identifierLabelWidget->setText(index.data(ActionRole).toString());
		m_ui->parametersLabelWidget->setText(index.data(ParametersRole).toString());
		m_ui->shortcutsLabelWidget->setText(index.sibling(index.row(), 1).data(Qt::DisplayRole).toString());
		m_ui->gesturesLabelWidget->setText(index.sibling(index.row(), 2).data(Qt::DisplayRole).toString());
	}
	else
	{
		m_ui->actionLabelWidget->setText({});
		m_ui->identifierLabelWidget->setText({});
		m_ui->parametersLabelWidget->setText({});
		m_ui->shortcutsLabelWidget->setText({});
		m_ui->gesturesLabelWidget->setText({});
	}
}

QString ActionsContentsWidget::getTitle() const
{
	return tr("Actions");
}

QLatin1String ActionsContentsWidget::getType() const
{
	return QLatin1String("actions");
}

QUrl ActionsContentsWidget::getUrl() const
{
	return {QLatin1String("about:actions")};
}

QIcon ActionsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("edit-cut"), false);
}

}
