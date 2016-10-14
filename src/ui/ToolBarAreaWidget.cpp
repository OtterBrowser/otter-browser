/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

ToolBarAreaWidget::ToolBarAreaWidget(Qt::ToolBarArea area, MainWindow *parent) : QWidget(parent),
	m_mainWindow(parent),
	m_area(area),
	m_dropRow(-1)
{
	QBoxLayout::Direction direction(QBoxLayout::BottomToTop);

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
	setAcceptDrops(true);

	QVector<ToolBarsManager::ToolBarDefinition> toolBarDefinitions(ToolBarsManager::getToolBarDefinitions());

	std::sort(toolBarDefinitions.begin(), toolBarDefinitions.end(), [&](const ToolBarsManager::ToolBarDefinition &first, const ToolBarsManager::ToolBarDefinition &second)
	{
		return (first.row < second.row);
	});

	for (int i = 0; i < toolBarDefinitions.count(); ++i)
	{
		if (toolBarDefinitions.at(i).location == m_area)
		{
			toolBarAdded(toolBarDefinitions.at(i).identifier);
		}
	}

	connect(m_mainWindow, SIGNAL(requestedToolBarsActivation(Qt::ToolBarAreas)), this, SLOT(activateToolBars(Qt::ToolBarAreas)));
	connect(m_mainWindow, SIGNAL(controlsHiddenChanged(bool)), this, SLOT(setControlsHidden(bool)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarAdded(int)), this, SLOT(toolBarAdded(int)));
	connect(ToolBarsManager::getInstance(), SIGNAL(toolBarModified(int)), this, SLOT(toolBarModified(int)));
}

void ToolBarAreaWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	if (m_dropRow < 0)
	{
		return;
	}

	QPainter painter(this);
	painter.setPen(QPen(palette().text(), 3, Qt::DotLine));

	const bool isHorizontal(m_area == Qt::TopToolBarArea || m_area == Qt::BottomToolBarArea);
	int lineOffset(4);

	for (int i = 0; i < m_layout->count(); ++i)
	{
		if (i >= m_dropRow)
		{
			break;
		}

		QWidget *widget(m_layout->itemAt(i)->widget());

		if (widget)
		{
			if (isHorizontal)
			{
				lineOffset += widget->height();
			}
			else
			{
				lineOffset += widget->width();
			}
		}
	}

	if (m_area == Qt::TopToolBarArea)
	{
		lineOffset = (height() - lineOffset);
	}
	else if (m_area == Qt::LeftToolBarArea)
	{
		lineOffset = (width() - lineOffset);
	}

	if (isHorizontal)
	{
		painter.drawLine(0, lineOffset, width(), lineOffset);
	}
	else
	{
		painter.drawLine(lineOffset, 0, lineOffset, height());
	}

	painter.setPen(QPen(palette().text(), 9, Qt::SolidLine, Qt::RoundCap));

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

void ToolBarAreaWidget::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	if (m_mainWindow->areControlsHidden())
	{
		const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));

		for (int i = 0; i < toolBars.count(); ++i)
		{
			ToolBarsManager::ToolBarVisibility visibility(ToolBarsManager::getToolBarDefinition(toolBars.at(i)->getIdentifier()).fullScreenVisibility);

			if (visibility == ToolBarsManager::OnHoverVisibleToolBar)
			{
				toolBars.at(i)->hide();
			}
		}
	}
}

void ToolBarAreaWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat(QLatin1String("x-toolbar-identifier")))
	{
		event->accept();

		updateDropRow(event->pos());
	}
	else
	{
		event->ignore();
	}
}

void ToolBarAreaWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasFormat(QLatin1String("x-toolbar-identifier")))
	{
		event->accept();

		updateDropRow(event->pos());
	}
	else
	{
		event->ignore();
	}
}

void ToolBarAreaWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	QWidget::dragLeaveEvent(event);

	m_dropRow = -1;

	update();
}

void ToolBarAreaWidget::dropEvent(QDropEvent *event)
{
	if (!event->mimeData()->hasFormat(QLatin1String("x-toolbar-identifier")))
	{
		event->ignore();

		return;
	}

	event->accept();

	updateDropRow(event->pos());

	const int draggedIdentifier(QString(event->mimeData()->data(QLatin1String("x-toolbar-identifier"))).toInt());
	QVector<int> identifiers;
	identifiers.reserve(m_layout->count() + 1);

	for (int i = 0; i < m_layout->count(); ++i)
	{
		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(m_layout->itemAt(i)->widget()));

		if (i == m_dropRow)
		{
			identifiers.append(draggedIdentifier);
		}

		if (toolBar && toolBar->getIdentifier() != draggedIdentifier)
		{
			identifiers.append(toolBar->getIdentifier());
		}
	}

	if (m_layout->count() == m_dropRow)
	{
		identifiers.append(draggedIdentifier);
	}

	for (int i = 0; i < identifiers.count(); ++i)
	{
		ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifiers.at(i)));
		definition.location = m_area;
		definition.row = i;

		ToolBarsManager::setToolBar(definition);
	}

	m_dropRow = -1;

	update();

	m_mainWindow->endToolBarDragging();
}

void ToolBarAreaWidget::startToolBarDragging()
{
	m_mainWindow->startToolBarDragging();
}

void ToolBarAreaWidget::endToolBarDragging()
{
	m_mainWindow->endToolBarDragging();
}

void ToolBarAreaWidget::setControlsHidden(bool areHidden)
{
	const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));
	bool showToolBar(false);

	for (int i = 0; i < toolBars.count(); ++i)
	{
		ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(toolBars.at(i)->getIdentifier()));
		ToolBarsManager::ToolBarVisibility visibility(areHidden ? definition.fullScreenVisibility : definition.normalVisibility);

		if (visibility == ToolBarsManager::AlwaysVisibleToolBar)
		{
			toolBars.at(i)->show();
		}
		else if (visibility == ToolBarsManager::AlwaysHiddenToolBar || visibility == ToolBarsManager::OnHoverVisibleToolBar)
		{
			toolBars.at(i)->hide();
		}

		if (visibility != ToolBarsManager::AlwaysHiddenToolBar)
		{
			showToolBar = true;
		}
	}

	setVisible(showToolBar);
}

void ToolBarAreaWidget::activateToolBars(Qt::ToolBarAreas areas)
{
	if (!areas.testFlag(m_area) || isHidden())
	{
		return;
	}

	const QList<ToolBarWidget*> toolBars(findChildren<ToolBarWidget*>(QString(), Qt::FindDirectChildrenOnly));

	for (int i = 0; i < toolBars.count(); ++i)
	{
		ToolBarsManager::ToolBarVisibility visibility(ToolBarsManager::getToolBarDefinition(toolBars.at(i)->getIdentifier()).fullScreenVisibility);

		if (visibility == ToolBarsManager::OnHoverVisibleToolBar)
		{
			toolBars.at(i)->show();
		}
	}
}

void ToolBarAreaWidget::insertToolBar(ToolBarWidget *toolBar)
{
	m_layout->insertWidget(ToolBarsManager::getToolBarDefinition(toolBar->getIdentifier()).row, toolBar);
}

void ToolBarAreaWidget::toolBarAdded(int identifier)
{
	const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(identifier));

	if (definition.location != m_area)
	{
		return;
	}

	ToolBarWidget *toolBar(new ToolBarWidget(identifier, NULL, this));

	if (definition.row < 0)
	{
		m_layout->addWidget(toolBar);
	}
	else
	{
		m_layout->addWidget(toolBar);
	}

	if (identifier == ToolBarsManager::TabBar)
	{
		m_mainWindow->setTabBar(toolBar->findChild<TabBarWidget*>());
	}

	updateToolBarsOrder();
}

void ToolBarAreaWidget::toolBarModified(int identifier)
{
	for (int i = 0; i < m_layout->count(); ++i)
	{
		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(m_layout->itemAt(i)->widget()));

		if (toolBar && toolBar->getIdentifier() == identifier)
		{
			const ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(toolBar->getIdentifier()));

			if (toolBar->getArea() != definition.location)
			{
				m_layout->removeWidget(toolBar);

				m_mainWindow->moveToolBar(toolBar, definition.location);

				updateToolBarsOrder();
			}
			else if (m_layout->indexOf(toolBar) != definition.row)
			{
				m_layout->removeWidget(toolBar);
				m_layout->insertWidget(definition.row, toolBar);

				updateToolBarsOrder();
			}
		}
	}
}

void ToolBarAreaWidget::updateDropRow(const QPoint &position)
{
	int row = 0;

	for (int i = 0; i < m_layout->count(); ++i)
	{
		QWidget *widget(m_layout->itemAt(i)->widget());

		if (widget && widget->geometry().contains(position))
		{
			row = i;

			const QPoint center(widget->geometry().center());

			switch (m_area)
			{
				case Qt::BottomToolBarArea:
					if (position.y() > center.y())
					{
						++row;
					}

					break;
				case Qt::LeftToolBarArea:
					if (position.x() < center.x())
					{
						++row;
					}

					break;
				case Qt::RightToolBarArea:
					if (position.x() > center.x())
					{
						++row;
					}

					break;
				default:
					if (position.y() < center.y())
					{
						++row;
					}

					break;
			}

			break;
		}
	}

	if (row != m_dropRow)
	{
		m_dropRow = row;

		update();
	}
}

void ToolBarAreaWidget::updateToolBarsOrder()
{
	for (int i = 0; i < m_layout->count(); ++i)
	{
		ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(m_layout->itemAt(i)->widget()));

		if (toolBar)
		{
			ToolBarsManager::ToolBarDefinition definition(ToolBarsManager::getToolBarDefinition(toolBar->getIdentifier()));

			if (definition.row != i)
			{
				definition.row = i;

				ToolBarsManager::setToolBar(definition);
			}
		}
	}
}

Qt::ToolBarArea ToolBarAreaWidget::getArea() const
{
	return m_area;
}

}
