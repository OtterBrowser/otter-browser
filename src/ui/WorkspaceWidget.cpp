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

#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

MdiWidget::MdiWidget(QWidget *parent) : QMdiArea(parent)
{
}

bool MdiWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
	{
		 return QAbstractScrollArea::eventFilter(object, event);
	}

	return QMdiArea::eventFilter(object, event);
}

void MdiWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
	menu.addSeparator();
	menu.addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
	menu.addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));
	menu.exec(event->globalPos());
}

WorkspaceWidget::WorkspaceWidget(QWidget *parent) : QWidget(parent),
	m_mdi(NULL),
	m_activeWindow(NULL)
{
	if (SettingsManager::getValue(QLatin1String("Interface/EnableMdi")).toBool())
	{
		m_mdi = new MdiWidget(this);
		m_mdi->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);

		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		layout->addWidget(m_mdi);

		connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));
	}
}

void WorkspaceWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_activeWindow && !m_mdi)
	{
		m_activeWindow->resize(size());
	}
}

void WorkspaceWidget::triggerAction(int identifier, bool checked)
{
	Q_UNUSED(checked)

	if (!m_mdi)
	{
		return;
	}

	switch (identifier)
	{
		case ActionsManager::MaximizeAllAction:
			for (int i = 0; i < m_mdi->subWindowList().count(); ++i)
			{
				m_mdi->subWindowList().at(i)->showMaximized();
			}

			break;
		case ActionsManager::MinimizeAllAction:
			for (int i = 0; i < m_mdi->subWindowList().count(); ++i)
			{
				m_mdi->subWindowList().at(i)->showMinimized();
			}

			break;
		case ActionsManager::RestoreAllAction:
			for (int i = 0; i < m_mdi->subWindowList().count(); ++i)
			{
				m_mdi->subWindowList().at(i)->showNormal();
			}

			break;
		case ActionsManager::CascadeAllAction:
			m_mdi->cascadeSubWindows();

			break;
		case ActionsManager::TileAllAction:
			m_mdi->tileSubWindows();

			break;
		default:
			break;
	}
}

void WorkspaceWidget::addWindow(Window *window)
{
	if (window)
	{
		if (m_mdi)
		{
			QMdiSubWindow *subWindow = m_mdi->addSubWindow(window, Qt::SubWindow);
			subWindow->showNormal();
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
		if (m_mdi)
		{
			if (window->parentWidget())
			{
				m_mdi->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(window->parentWidget()));
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
