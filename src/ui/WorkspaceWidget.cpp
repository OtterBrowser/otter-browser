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

#include "WorkspaceWidget.h"
#include "Window.h"

namespace Otter
{

WorkspaceWidget::WorkspaceWidget(QWidget *parent) : QWidget(parent),
	m_activeWindow(NULL)
{
}

void WorkspaceWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_activeWindow)
	{
		m_activeWindow->resize(size());
	}
}

void WorkspaceWidget::addWindow(Window *window)
{
	if (window)
	{
		window->hide();
		window->setParent(this);
		window->move(0, 0);
	}
}

void WorkspaceWidget::setActiveWindow(Window *window)
{
	if (window != m_activeWindow)
	{
		if (window)
		{
			window->resize(size());
			window->show();
		}

		if (m_activeWindow)
		{
			m_activeWindow->hide();
		}

		m_activeWindow = window;
	}
}

Window* WorkspaceWidget::getActiveWindow()
{
	return m_activeWindow.data();
}

}
