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
	m_ui(new Ui::ActionsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->actionsViewWidget->setViewMode(ItemViewWidget::TreeView);

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
