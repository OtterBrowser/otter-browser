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
#include "MainWindow.h"
#include "Window.h"
#include "../core/SettingsManager.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

WorkspaceWidget::WorkspaceWidget(QWidget *parent) : QWidget(parent),
	m_mdiArea(),
	m_activeWindow(NULL)
{
	if (SettingsManager::getValue(QLatin1String("Interface/EnableMdi")).toBool())
	{
		m_mdiArea = new QMdiArea(this);

		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		layout->addWidget(m_mdiArea);

		connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));
	}
}

void WorkspaceWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_activeWindow && !m_mdiArea)
	{
		m_activeWindow->resize(size());
	}
}

void WorkspaceWidget::addWindow(Window *window)
{
	if (window)
	{
		if (m_mdiArea)
		{
			QMdiSubWindow *subWindow = m_mdiArea->addSubWindow(window, Qt::SubWindow);
			subWindow->setSystemMenu(NULL);
			subWindow->installEventFilter(this);

			connect(window, SIGNAL(destroyed()), subWindow, SLOT(deleteLater()));
		}
		else
		{
			window->hide();
			window->setParent(this);
			window->move(0, 0);
		}
	}
}

void WorkspaceWidget::activeSubWindowChanged(QMdiSubWindow *subWindow)
{
	if (subWindow)
	{
		Window *window = qobject_cast<Window*>(subWindow->widget());

		if (window)
		{
			MainWindow::findMainWindow(this)->getWindowsManager()->setActiveWindowByIdentifier(window->getIdentifier());
		}
	}
}

void WorkspaceWidget::setActiveWindow(Window *window)
{
	if (window != m_activeWindow)
	{
		if (m_mdiArea)
		{
			if (window->parentWidget())
			{
				m_mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(window->parentWidget()));
			}
		}
		else
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
		}

		m_activeWindow = window;
	}
}

Window* WorkspaceWidget::getActiveWindow()
{
	return m_activeWindow.data();
}

bool WorkspaceWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::Close)
	{
		QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow*>(object);

		if (subWindow)
		{
			Window *window = qobject_cast<Window*>(subWindow->widget());

			if (window)
			{
				window->close();
			}

			event->ignore();

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
