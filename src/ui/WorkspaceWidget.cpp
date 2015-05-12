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
#include <QtGui/QWindowStateChangeEvent>
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
	m_activeWindow(NULL),
	m_ignoreStateChange(false)
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
	if (!m_mdi)
	{
		return;
	}

	switch (identifier)
	{
		case ActionsManager::MaximizeTabAction:
			if (m_mdi->activeSubWindow())
			{
				m_mdi->activeSubWindow()->showMaximized();
			}

			break;
		case ActionsManager::MinimizeTabAction:
			if (m_mdi->activeSubWindow())
			{
				m_mdi->activeSubWindow()->showMinimized();
			}

			break;
		case ActionsManager::RestoreTabAction:
			if (m_mdi->activeSubWindow())
			{
				m_mdi->activeSubWindow()->showNormal();
			}

			break;
		case ActionsManager::AlwaysOnTopTabAction:
			if (m_mdi->activeSubWindow())
			{
				if (checked)
				{
					m_mdi->activeSubWindow()->setWindowFlags(m_mdi->activeSubWindow()->windowFlags() | Qt::WindowStaysOnTopHint);
				}
				else
				{
					m_mdi->activeSubWindow()->setWindowFlags(m_mdi->activeSubWindow()->windowFlags() & ~Qt::WindowStaysOnTopHint);
				}
			}

			break;
		case ActionsManager::MaximizeAllAction:
			{
				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					subWindows.at(i)->showMaximized();
				}
			}

			break;
		case ActionsManager::MinimizeAllAction:
			{
				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					subWindows.at(i)->showMinimized();
				}
			}

			break;
		case ActionsManager::RestoreAllAction:
			{
				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					subWindows.at(i)->showNormal();
				}
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

void WorkspaceWidget::addWindow(Window *window, const QRect &geometry, WindowState state, bool isAlwaysOnTop)
{
	if (window)
	{
		if (m_mdi)
		{
			QMdiSubWindow *subWindow = m_mdi->addSubWindow(window, Qt::SubWindow);
			subWindow->installEventFilter(this);

			Action *closeAction = new Action(ActionsManager::CloseTabAction);
			closeAction->setEnabled(true);
			closeAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Close"));
			closeAction->setIcon(QIcon());

			QMenu *menu = new QMenu(subWindow);
			menu->addAction(closeAction);
			menu->addAction(ActionsManager::getAction(ActionsManager::RestoreTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::MinimizeTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::MaximizeTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this));
			menu->addSeparator();

			QMenu *arrangementMenu = menu->addMenu(tr("Arrangement"));
			arrangementMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
			arrangementMenu->addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
			arrangementMenu->addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
			arrangementMenu->addSeparator();
			arrangementMenu->addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
			arrangementMenu->addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));

			subWindow->show();
			subWindow->setSystemMenu(menu);

			if (isAlwaysOnTop)
			{
				subWindow->setWindowFlags(subWindow->windowFlags() | Qt::WindowStaysOnTopHint);
			}

			if (geometry.isValid())
			{
				subWindow->setGeometry(geometry);
			}

			switch (state)
			{
				case MaximizedWindowState:
					subWindow->showMaximized();

					break;
				case MinimizedWindowState:
					subWindow->showMinimized();

					break;
				default:
					subWindow->showNormal();

					break;
			}

			updateActions();

			connect(closeAction, SIGNAL(triggered()), window, SLOT(close()));
			connect(subWindow, SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates)), this, SLOT(updateActions()));
			connect(window, SIGNAL(destroyed()), subWindow, SLOT(deleteLater()));
			connect(window, SIGNAL(destroyed()), this, SLOT(updateActions()));
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
	else
	{
		updateActions();
	}
}

void WorkspaceWidget::updateActions()
{
	const QList<QMdiSubWindow*> subWindows = (m_mdi ? m_mdi->subWindowList() : QList<QMdiSubWindow*>());
	int maximizedSubWindows = 0;
	int minimizedSubWindows = 0;
	int restoredSubWindows = 0;

	for (int i = 0; i < subWindows.count(); ++i)
	{
		const Qt::WindowStates states = subWindows.at(i)->windowState();

		if (states.testFlag(Qt::WindowMaximized))
		{
			++maximizedSubWindows;
		}
		else if (states.testFlag(Qt::WindowMinimized))
		{
			++minimizedSubWindows;
		}
		else
		{
			++restoredSubWindows;
		}
	}

	ActionsManager::getAction(ActionsManager::MaximizeAllAction, this)->setEnabled(maximizedSubWindows < subWindows.count());
	ActionsManager::getAction(ActionsManager::MinimizeAllAction, this)->setEnabled(minimizedSubWindows < subWindows.count());
	ActionsManager::getAction(ActionsManager::RestoreAllAction, this)->setEnabled(restoredSubWindows < subWindows.count());
	ActionsManager::getAction(ActionsManager::CascadeAllAction, this)->setEnabled(subWindows.count() > 0);
	ActionsManager::getAction(ActionsManager::TileAllAction, this)->setEnabled(subWindows.count() > 0);

	QMdiSubWindow *activeSubWindow = (m_mdi ? m_mdi->activeSubWindow() : NULL);

	ActionsManager::getAction(ActionsManager::MaximizeTabAction, this)->setEnabled(activeSubWindow && !activeSubWindow->windowState().testFlag(Qt::WindowMaximized));
	ActionsManager::getAction(ActionsManager::MinimizeTabAction, this)->setEnabled(activeSubWindow && !activeSubWindow->windowState().testFlag(Qt::WindowMinimized));
	ActionsManager::getAction(ActionsManager::RestoreTabAction, this)->setEnabled(activeSubWindow && (activeSubWindow->windowState().testFlag(Qt::WindowMaximized) || activeSubWindow->windowState().testFlag(Qt::WindowMinimized)));
	ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this)->setEnabled(activeSubWindow);
	ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this)->setChecked(activeSubWindow && activeSubWindow->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
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
	else if (event->type() == QEvent::WindowStateChange)
	{
		if (m_ignoreStateChange)
		{
			m_ignoreStateChange = false;
		}
		else
		{
			QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow*>(object);
			QWindowStateChangeEvent *windowStateChangeEvent = dynamic_cast<QWindowStateChangeEvent*>(event);

			if (subWindow && windowStateChangeEvent)
			{
				m_ignoreStateChange = true;

				const bool isActive = subWindow->isActiveWindow();
				const bool isMinimized = subWindow->isMinimized();
				const bool isStayOnTop = subWindow->windowFlags().testFlag(Qt::WindowStaysOnTopHint);

				if (!windowStateChangeEvent->oldState().testFlag(Qt::WindowMaximized) && subWindow->windowState().testFlag(Qt::WindowMaximized))
				{
					Qt::WindowFlags flags = (Qt::SubWindow | Qt::CustomizeWindowHint);

					if (isStayOnTop)
					{
						flags |= Qt::WindowStaysOnTopHint;
					}

					subWindow->setWindowFlags(flags);
				}
				else if (windowStateChangeEvent->oldState().testFlag(Qt::WindowMaximized) && !subWindow->windowState().testFlag(Qt::WindowMaximized))
				{
					Qt::WindowFlags flags = Qt::SubWindow;

					if (isStayOnTop)
					{
						flags |= Qt::WindowStaysOnTopHint;
					}

					subWindow->setWindowFlags(flags);

					if (isMinimized)
					{
						subWindow->showMinimized();
					}
				}

				if (isActive)
				{
					m_mdi->setActiveSubWindow(NULL);
					m_mdi->setActiveSubWindow(subWindow);
				}
			}
		}

		SessionsManager::markSessionModified();
	}
	else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
	{
		SessionsManager::markSessionModified();
	}

	return QObject::eventFilter(object, event);
}

}
