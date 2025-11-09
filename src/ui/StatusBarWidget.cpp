/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StatusBarWidget.h"
#include "MainWindow.h"
#include "ToolBarWidget.h"

#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

StatusBarWidget::StatusBarWidget(MainWindow *parent) : QStatusBar(parent),
	m_toolBar(new ToolBarWidget(ToolBarsManager::StatusBar, nullptr, this))
{
	setFixedHeight(m_toolBar->getIconSize());
	setSizeGripEnabled(false);
	setStyleSheet(QLatin1String("padding:1px;"));

	connect(m_toolBar, &ToolBarWidget::iconSizeChanged, this, [&](int iconSize)
	{
		setFixedHeight(iconSize);
	});
}

void StatusBarWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOption option;
	option.initFrom(this);

	style()->drawPrimitive(QStyle::PE_PanelStatusBar, &option, &painter, this);
}

void StatusBarWidget::resizeEvent(QResizeEvent *event)
{
	QStatusBar::resizeEvent(event);

	m_toolBar->setFixedSize(size());
	m_toolBar->move(0, 0);
}

}
