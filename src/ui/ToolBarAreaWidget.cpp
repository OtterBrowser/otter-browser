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

#include "ToolBarAreaWidget.h"
#include "MainWindow.h"
#include "TabBarWidget.h"
#include "ToolBarWidget.h"
#include "../core/ToolBarsManager.h"

#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

ToolBarDropAreaWidget::ToolBarDropAreaWidget(ToolBarAreaWidget *parent) : QWidget(parent),
	m_area(parent)
{
}

void ToolBarDropAreaWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	const bool isHorizontal = (m_area->getArea() == Qt::TopToolBarArea || m_area->getArea() == Qt::BottomToolBarArea);
	int lineWidth = (isHorizontal ? (height() / 4) : (width() / 4));
	int lineOffset = (isHorizontal ? (height() / 2) : (width() / 2));
	QPainter painter(this);
	painter.setPen(QPen(palette().text(), lineWidth, Qt::DotLine));

	if (isHorizontal)
	{
		painter.drawLine(0, lineOffset, width(), lineOffset);
	}
	else
	{
		painter.drawLine(lineOffset, 0, lineOffset, height());
	}

	painter.setPen(QPen(palette().text(), (lineWidth * 3), Qt::SolidLine, Qt::RoundCap));

	if (isHorizontal)
	{
		painter.drawPoint(0, lineOffset);
		painter.drawPoint(width(), lineOffset);
	}
	else
	{
		painter.drawPoint(lineOffset, 0);
		painter.drawPoint(lineOffset, height());
	}
}

QSize ToolBarDropAreaWidget::sizeHint() const
{
	return QSize(10, 10).expandedTo(QApplication::globalStrut());
}

ToolBarAreaWidget::ToolBarAreaWidget(Qt::ToolBarArea area, MainWindow *parent) : QWidget(parent),
	m_mainWindow(parent),
	m_tabBarToolBar(NULL),
	m_area(area)
{
	QBoxLayout::Direction direction = QBoxLayout::BottomToTop;

	if (area == Qt::BottomToolBarArea)
	{
		direction = QBoxLayout::TopToBottom;
	}
	else if (area == Qt::LeftToolBarArea)
	{
		direction = QBoxLayout::RightToLeft;
	}
	else if (area == Qt::RightToolBarArea)
	{
		direction = QBoxLayout::LeftToRight;
	}

	m_layout = new QBoxLayout(direction, this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(0);

	setLayout(m_layout);

	const QVector<ToolBarDefinition> toolBarDefinitions = ToolBarsManager::getToolBarDefinitions();

	for (int i = 0; i < toolBarDefinitions.count(); ++i)
	{
		if (toolBarDefinitions.at(i).location == m_area)
		{
			toolBarAdded(toolBarDefinitions.at(i).identifier);
		}
	}

	connect(m_mainWindow, SIGNAL(controlsHiddenChanged(bool)), this, SLOT(controlsHiddenChanged(bool)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarAdded(int)), this, SLOT(toolBarAdded(int)));
}

void ToolBarAreaWidget::controlsHiddenChanged(bool hidden)
{
	if (m_tabBarToolBar)
	{
		const QList<ToolBarWidget*> toolBars = findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly);

		if (hidden)
		{
			if (m_area == Qt::LeftToolBarArea || m_area == Qt::RightToolBarArea)
			{
				m_tabBarToolBar->setMaximumWidth(1);
			}
			else
			{
				m_tabBarToolBar->setMaximumHeight(1);
			}

			m_tabBarToolBar->installEventFilter(this);

			for (int i = 0; i < toolBars.count(); ++i)
			{
				if (toolBars.at(i) != m_tabBarToolBar)
				{
					toolBars.at(i)->hide();
				}
			}
		}
		else
		{
			if (m_area == Qt::LeftToolBarArea || m_area == Qt::RightToolBarArea)
			{
				m_tabBarToolBar->setMaximumWidth(QWIDGETSIZE_MAX);
			}
			else
			{
				m_tabBarToolBar->setMaximumHeight(QWIDGETSIZE_MAX);
			}

			m_tabBarToolBar->removeEventFilter(this);

			for (int i = 0; i < toolBars.count(); ++i)
			{
				if (toolBars.at(i) != m_tabBarToolBar)
				{
					toolBars.at(i)->show();
				}
			}
		}
	}
	else
	{
		setVisible(!hidden);
	}
}

void ToolBarAreaWidget::toolBarAdded(int identifier)
{
	const ToolBarDefinition definition = ToolBarsManager::getToolBarDefinition(identifier);

	if (definition.location != m_area)
	{
		return;
	}

	ToolBarWidget *toolBar = new ToolBarWidget(identifier, NULL, this);
	toolBar->setOrientation((m_area == Qt::TopToolBarArea || m_area == Qt::BottomToolBarArea) ? Qt::Horizontal : Qt::Vertical);

	if (definition.row < 0)
	{
		m_layout->addWidget(toolBar);
	}
	else
	{
		m_layout->insertWidget(definition.row, toolBar);
	}

	if (identifier == ToolBarsManager::TabBar)
	{
		m_tabBarToolBar = toolBar;

		m_mainWindow->setTabBar(toolBar->findChild<TabBarWidget*>());
	}
}

Qt::ToolBarArea ToolBarAreaWidget::getArea() const
{
	return m_area;
}

bool ToolBarAreaWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_tabBarToolBar && event->type() == QEvent::Enter)
	{
		if (m_area == Qt::LeftToolBarArea || m_area == Qt::RightToolBarArea)
		{
			m_tabBarToolBar->setMaximumWidth(QWIDGETSIZE_MAX);
		}
		else
		{
			m_tabBarToolBar->setMaximumHeight(QWIDGETSIZE_MAX);
		}
	}

	if (object == m_tabBarToolBar && event->type() == QEvent::Leave)
	{
		if (m_area == Qt::LeftToolBarArea || m_area == Qt::RightToolBarArea)
		{
			m_tabBarToolBar->setMaximumWidth(1);
		}
		else
		{
			m_tabBarToolBar->setMaximumHeight(1);
		}
	}

	return QWidget::eventFilter(object, event);
}

}
