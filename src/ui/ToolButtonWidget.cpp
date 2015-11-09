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

#include "ToolButtonWidget.h"
#include "ToolBarWidget.h"

#include <QtCore/QEvent>
#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ToolButtonWidget::ToolButtonWidget(QWidget *parent) : QToolButton(parent)
{
	setAutoRaise(true);
	setContextMenuPolicy(Qt::NoContextMenu);

	ToolBarWidget *toolBar = qobject_cast<ToolBarWidget*>(parent);

	if (toolBar)
	{
		setIconSize(toolBar->iconSize());
		setMaximumButtonSize(toolBar->getMaximumButtonSize());
		setToolButtonStyle(toolBar->toolButtonStyle());

		connect(toolBar, SIGNAL(iconSizeChanged(QSize)), this, SLOT(setIconSize(QSize)));
		connect(toolBar, SIGNAL(maximumButtonSizeChanged(int)), this, SLOT(setMaximumButtonSize(int)));
		connect(toolBar, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)), this, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
	}
}

void ToolButtonWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionToolButton option;
	initStyleOption(&option);

	option.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight, (option.rect.width() - (option.fontMetrics.width(QLatin1Char(' ')) * 2) - ((toolButtonStyle() == Qt::ToolButtonTextBesideIcon) ? iconSize().width() : 0)));

	painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

void ToolButtonWidget::setMaximumButtonSize(int size)
{
	if (size > 0)
	{
		setMaximumSize(size, size);
	}
	else
	{
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	}
}

bool ToolButtonWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ContextMenu && contextMenuPolicy() == Qt::NoContextMenu)
	{
		return false;
	}

	return QToolButton::event(event);
}

}
