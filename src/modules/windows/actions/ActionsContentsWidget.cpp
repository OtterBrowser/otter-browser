/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

	m_model->setHorizontalHeaderLabels({tr("Action")});

	const QVector<ActionsManager::ActionDefinition> definitions(ActionsManager::getActionDefinitions());

	for (int i = 0; i < definitions.count(); ++i)
	{
		if (!definitions.at(i).flags.testFlag(ActionsManager::ActionDefinition::IsDeprecatedFlag) && !definitions.at(i).flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters))
		{
			QStandardItem *item(new QStandardItem(definitions.at(i).getText(true)));
			item->setData(QColor(Qt::transparent), Qt::DecorationRole);
			item->setData(definitions.at(i).identifier, Qt::UserRole);
			item->setToolTip(QStringLiteral("%1 (%2)").arg(item->text()).arg(ActionsManager::getActionName(definitions.at(i).identifier)));
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			if (!definitions.at(i).defaultState.icon.isNull())
			{
				item->setIcon(definitions.at(i).defaultState.icon);
			}

			m_model->appendRow(item);
		}
	}

	m_ui->actionsViewWidget->setModel(m_model);

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::setFilterString);
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

		m_model->setHorizontalHeaderLabels({tr("Action")});
	}
}

void ActionsContentsWidget::print(QPrinter *printer)
{
	m_ui->actionsViewWidget->render(printer);
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
	return QUrl(QLatin1String("about:actions"));
}

QIcon ActionsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("edit-cut"), false);
}

}
