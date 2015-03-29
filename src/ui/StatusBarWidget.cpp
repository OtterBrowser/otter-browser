/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "StatusBarWidget.h"
#include "MainWindow.h"
#include "ToolBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtWidgets/QSizeGrip>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

StatusBarWidget::StatusBarWidget(MainWindow *parent) : QStatusBar(parent),
	m_toolBar(new ToolBarWidget(QLatin1String("StatusBar"), NULL, parent))
{
	m_toolBar->setParent(this);

	setFixedHeight(ToolBarsManager::getToolBarDefinition(QLatin1String("StatusBar")).iconSize);

	QTimer::singleShot(100, this, SLOT(updateSize()));

	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(QString)), this, SLOT(toolBarModified(QString)));
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

	updateSize();
}

void StatusBarWidget::toolBarModified(const QString &identifier)
{
	if (identifier == QLatin1String("StatusBar"))
	{
		setFixedHeight(ToolBarsManager::getToolBarDefinition(QLatin1String("StatusBar")).iconSize);
	}
}

void StatusBarWidget::updateSize()
{
	QSizeGrip *sizeGrip = findChild<QSizeGrip*>();
	const int offset = (sizeGrip ? sizeGrip->width() : 0);

	m_toolBar->setFixedSize((width() - offset), ToolBarsManager::getToolBarDefinition(QLatin1String("StatusBar")).iconSize);
	m_toolBar->move(((layoutDirection() == Qt::LeftToRight) ? 0 : offset), 0);
}

}
