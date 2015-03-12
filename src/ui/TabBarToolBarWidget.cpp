/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabBarToolBarWidget.h"
#include "ActionWidget.h"
#include "Menu.h"
#include "TabBarWidget.h"
#include "toolbars/MenuActionWidget.h"
#include "../core/ActionsManager.h"
#include "../core/Utils.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

namespace Otter
{

TabBarToolBarWidget::TabBarToolBarWidget(QMainWindow *parent) : QToolBar(parent),
	m_window(parent),
	m_widget(new QWidget(this)),
	m_tabBar(new TabBarWidget(m_widget)),
	m_newTabButton(new ActionWidget(Action::NewTabAction, NULL, m_widget))
{
	setObjectName(QLatin1String("tabBarToolBar"));
	setStyleSheet(QLatin1String("QToolBar {padding:0;}"));
	setAllowedAreas(Qt::AllToolBarAreas);
	setFloatable(false);

	QAction *closedWindowsAction = new QAction(Utils::getIcon(QLatin1String("user-trash")), tr("Closed Tabs"), this);
	Menu *closedWindowsMenu = new Menu(m_widget);
	closedWindowsMenu->setRole(ClosedWindowsMenu);
	closedWindowsAction->setMenu(closedWindowsMenu);
	closedWindowsAction->setEnabled(false);

	QToolButton *closedWindowsMenuButton = new QToolButton(m_widget);
	closedWindowsMenuButton->setDefaultAction(closedWindowsAction);
	closedWindowsMenuButton->setAutoRaise(true);
	closedWindowsMenuButton->setPopupMode(QToolButton::InstantPopup);

	QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, m_widget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(3);
	layout->addSpacing(3);
	layout->addWidget(new MenuActionWidget(m_widget));
	layout->addWidget(m_tabBar, 1);
	layout->addWidget(m_newTabButton, 0, Qt::AlignCenter);
	layout->addStretch();
	layout->addWidget(closedWindowsMenuButton, 0, Qt::AlignCenter);
	layout->addSpacing(3);

	m_widget->setLayout(layout);
	m_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	addWidget(m_widget);
	updateOrientation();

	connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(updateOrientation()));
	connect(this, SIGNAL(topLevelChanged(bool)), m_tabBar, SLOT(setIsMoved(bool)));
	connect(m_tabBar, SIGNAL(layoutChanged()), this, SLOT(updateLayout()));
}

void TabBarToolBarWidget::resizeEvent(QResizeEvent *event)
{
	QToolBar::resizeEvent(event);

	updateLayout();
}

void TabBarToolBarWidget::updateLayout()
{
	QBoxLayout *layout = dynamic_cast<QBoxLayout*>(m_widget->layout());

	if (!layout)
	{
		return;
	}

	const int spacing = 3;
	int size = ((isMovable() ? style()->pixelMetric(QStyle::PM_ToolBarHandleExtent) : 0) + spacing);
	const bool isHorizontal = (m_tabBar->shape() == QTabBar::RoundedNorth || m_tabBar->shape() == QTabBar::RoundedSouth);

	for (int i = 0; i < layout->count(); ++i)
	{
		if (layout->itemAt(i)->widget() && layout->itemAt(i)->widget()->isVisible() && layout->itemAt(i)->widget() != m_tabBar)
		{
			size += (isHorizontal ? layout->itemAt(i)->geometry().width() : layout->itemAt(i)->geometry().height());
		}

		size += spacing;
	}

	if (isHorizontal)
	{
		m_tabBar->setMaximumSize(qMax(0, qMin((m_tabBar->count() * 250), (width() - size))), QWIDGETSIZE_MAX);
	}
	else
	{
		m_tabBar->setMaximumSize(QWIDGETSIZE_MAX, qMax(0, qMin((m_tabBar->count() * 250), (height() - size))));
	}
}

void TabBarToolBarWidget::updateOrientation()
{
	QBoxLayout *layout = qobject_cast<QBoxLayout*>(m_widget->layout());
	const Qt::ToolBarArea area = m_window->toolBarArea(this);

	m_tabBar->setOrientation(area);

	if (layout)
	{
		if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea)
		{
			layout->setDirection(QBoxLayout::TopToBottom);

			m_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		}
		else
		{
			layout->setDirection(QBoxLayout::LeftToRight);

			m_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		}
	}

	updateLayout();
}

TabBarWidget* TabBarToolBarWidget::getTabBar()
{
	return m_tabBar;
}

}
