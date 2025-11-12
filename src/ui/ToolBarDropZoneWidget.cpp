/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolBarDropZoneWidget.h"
#include "MainWindow.h"
#include "ToolBarWidget.h"

#include <QtGui/QDropEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

namespace Otter
{

ToolBarDropZoneWidget::ToolBarDropZoneWidget(MainWindow *parent) : QToolBar(parent),
	m_mainWindow(parent),
	m_isDropTarget(false)
{
	setAcceptDrops(true);
}

void ToolBarDropZoneWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);

	if (!m_isDropTarget)
	{
		painter.setOpacity(0.25);
	}

	QStyleOptionRubberBand rubberBandOption;
	rubberBandOption.initFrom(this);

	style()->drawControl(QStyle::CE_RubberBand, &rubberBandOption, &painter);
}

void ToolBarDropZoneWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (canDrop(event))
	{
		event->accept();

		m_isDropTarget = true;

		update();
	}
	else
	{
		event->ignore();
	}
}

void ToolBarDropZoneWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (canDrop(event))
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ToolBarDropZoneWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	QWidget::dragLeaveEvent(event);

	m_isDropTarget = false;

	update();
}

void ToolBarDropZoneWidget::dropEvent(QDropEvent *event)
{
	if (!canDrop(event))
	{
		event->ignore();

		m_mainWindow->endToolBarDragging();

		return;
	}

	event->accept();

	m_isDropTarget = false;

	update();

	const QList<ToolBarWidget*> toolBars(m_mainWindow->findChildren<ToolBarWidget*>());
	const int draggedIdentifier(event->mimeData()->property("x-toolbar-identifier").toInt());

	for (int i = 0; i < toolBars.count(); ++i)
	{
		ToolBarWidget *toolBar(toolBars.at(i));

		if (toolBar->getIdentifier() == draggedIdentifier)
		{
			m_mainWindow->insertToolBar(this, toolBar);
			m_mainWindow->insertToolBarBreak(this);

			toolBar->setArea(m_mainWindow->toolBarArea(this));

			break;
		}
	}

	m_mainWindow->endToolBarDragging();
}

QSize ToolBarDropZoneWidget::sizeHint() const
{
	if ((orientation() == Qt::Horizontal))
	{
		return {QToolBar::sizeHint().width(), 5};
	}

	return {5, QToolBar::sizeHint().height()};
}

bool ToolBarDropZoneWidget::canDrop(QDropEvent *event)
{
	return (!event->mimeData()->property("x-toolbar-identifier").isNull() && m_mainWindow->toolBarArea(this) != Qt::NoToolBarArea);
}

}
