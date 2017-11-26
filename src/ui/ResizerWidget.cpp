/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ResizerWidget.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>

namespace Otter
{

ResizerWidget::ResizerWidget(QWidget *widget, QWidget *parent) : QWidget(parent),
	m_widget(widget),
	m_direction(LeftToRightDirection),
	m_isMoving(false)
{
	setDirection(LeftToRightDirection);
}

void ResizerWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::StyleChange)
	{
		setDirection(m_direction);
	}
}

void ResizerWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	QStyleOption option;
	option.rect = contentsRect();
	option.palette = palette();
	option.state = (QStyle::State_Enabled | ((m_direction == TopToBottomDirection || m_direction == BottomToTopDirection) ? QStyle::State_None : QStyle::State_Horizontal));

	if (underMouse())
	{
		option.state |= QStyle::State_MouseOver;
	}

	if (m_isMoving)
	{
		option.state |= QStyle::State_Sunken;
	}

	style()->drawControl(QStyle::CE_Splitter, &option, &painter);
}

void ResizerWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragStartPosition = event->pos();

		event->accept();
	}

	QWidget::mousePressEvent(event);
}

void ResizerWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_isMoving && ((event->pos() - m_dragStartPosition).manhattanLength() > QApplication::startDragDistance()))
	{
		m_isMoving = true;
	}

	if (!m_widget)
	{
		return;
	}

	const bool isFlipped(m_direction == RightToLeftDirection || m_direction == BottomToTopDirection);
	const bool isVertical(m_direction == TopToBottomDirection || m_direction == BottomToTopDirection);
	const QPoint positionDifference(isFlipped ? (m_dragStartPosition - event->pos()) : (event->pos() - m_dragStartPosition));
	int sizeDifference(0);

	if (isVertical && positionDifference.y() != 0)
	{
		sizeDifference = positionDifference.y();
	}
	else if (positionDifference.x() != 0)
	{
		sizeDifference = positionDifference.x();
	}

	if (sizeDifference == 0)
	{
		return;
	}

	const int oldSize((m_direction == TopToBottomDirection || m_direction == BottomToTopDirection) ? m_widget->height() : m_widget->width());
	int newSize(oldSize + sizeDifference);

	m_dragStartPosition = event->pos();

	if (newSize < 0)
	{
		sizeDifference = -oldSize;

		newSize = 0;
	}

	if (sizeDifference == 0)
	{
		return;
	}

	if (isVertical)
	{
		m_widget->setFixedHeight(newSize);

		if (isFlipped)
		{
			m_dragStartPosition += QPoint(0, sizeDifference);
		}
		else
		{
			m_dragStartPosition -= QPoint(0, sizeDifference);
		}
	}
	else
	{
		m_widget->setFixedWidth(newSize);

		if (isFlipped)
		{
			m_dragStartPosition += QPoint(sizeDifference, 0);
		}
		else
		{
			m_dragStartPosition -= QPoint(sizeDifference, 0);
		}
	}

	emit resized(newSize);
}

void ResizerWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_isMoving && event->button() == Qt::LeftButton)
	{
		m_dragStartPosition = {};
		m_isMoving = false;
	}

	emit finished();

	QWidget::mouseReleaseEvent(event);
}

void ResizerWidget::setDirection(ResizeDirection direction)
{
	m_direction = direction;

	const int size(style()->pixelMetric(QStyle::PM_SplitterWidth));

	setMinimumSize(size, size);

	if (direction == TopToBottomDirection || direction == BottomToTopDirection)
	{
		setCursor(Qt::SizeVerCursor);
		setMaximumSize(QWIDGETSIZE_MAX, size);
	}
	else
	{
		setCursor(Qt::SizeHorCursor);
		setMaximumSize(size, QWIDGETSIZE_MAX);
	}
}

}
