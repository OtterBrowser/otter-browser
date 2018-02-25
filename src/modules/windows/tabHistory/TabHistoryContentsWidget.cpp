/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabHistoryContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Window.h"

#include "ui_TabHistoryContentsWidget.h"

namespace Otter
{

TabHistoryContentsWidget::TabHistoryContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_window(nullptr),
	m_ui(new Ui::TabHistoryContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->informationViewWidget->setViewMode(ItemViewWidget::TreeViewMode);

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->informationViewWidget, &ItemViewWidget::setFilterString);
}

TabHistoryContentsWidget::~TabHistoryContentsWidget()
{
	delete m_ui;
}

void TabHistoryContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

QString TabHistoryContentsWidget::getTitle() const
{
	return tr("Tab History");
}

QLatin1String TabHistoryContentsWidget::getType() const
{
	return QLatin1String("tabHistory");
}

QUrl TabHistoryContentsWidget::getUrl() const
{
	return {};
}

QIcon TabHistoryContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("view-history"), false);
}

}
