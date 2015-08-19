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
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionTitleBar>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

MdiWidget::MdiWidget(QWidget *parent) : QMdiArea(parent)
{
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

bool MdiWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
	{
		 return QAbstractScrollArea::eventFilter(object, event);
	}

	return QMdiArea::eventFilter(object, event);
}

MdiWindow::MdiWindow(Window *window, MdiWidget *parent) : QMdiSubWindow(parent, Qt::SubWindow),
	m_wasMaximized(false)
{
	setWidget(window);

	parent->addSubWindow(this);

	connect(window, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}

void MdiWindow::storeState()
{
	m_wasMaximized = isMaximized();
}

void MdiWindow::restoreState()
{
	if (m_wasMaximized)
	{
		setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
		showMaximized();
	}
	else
	{
		showNormal();
	}

	SessionsManager::markSessionModified();
}

void MdiWindow::changeEvent(QEvent *event)
{
	QMdiSubWindow::changeEvent(event);

	if (event->type() == QEvent::WindowStateChange)
	{
		SessionsManager::markSessionModified();
	}
}

void MdiWindow::closeEvent(QCloseEvent *event)
{
	Window *window = qobject_cast<Window*>(widget());

	if (window)
	{
		window->close();
	}

	event->ignore();
}

void MdiWindow::moveEvent(QMoveEvent *event)
{
	QMdiSubWindow::moveEvent(event);

	SessionsManager::markSessionModified();
}

void MdiWindow::resizeEvent(QResizeEvent *event)
{
	QMdiSubWindow::resizeEvent(event);

	SessionsManager::markSessionModified();
}

void MdiWindow::mouseReleaseEvent(QMouseEvent *event)
{
	QStyleOptionTitleBar option;
	option.initFrom(this);
	option.titleBarFlags = windowFlags();
	option.titleBarState = windowState();
	option.subControls = QStyle::SC_All;
	option.activeSubControls = QStyle::SC_None;

	if (!isMinimized())
	{
		option.rect.setHeight(height() - widget()->height());
	}

	if (!isMaximized() && style()->subControlRect(QStyle::CC_TitleBar, &option, QStyle::SC_TitleBarMaxButton, this).contains(event->pos()))
	{
		setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
		showMaximized();

		SessionsManager::markSessionModified();
	}
	else if (!isMinimized() && style()->subControlRect(QStyle::CC_TitleBar, &option, QStyle::SC_TitleBarMinButton, this).contains(event->pos()))
	{
		const QList<QMdiSubWindow*> subWindows = mdiArea()->subWindowList();
		int activeSubWindows = 0;

		for (int i = 0; i < subWindows.count(); ++i)
		{
			if (!subWindows.at(i)->isMinimized())
			{
				++activeSubWindows;
			}
		}

		storeState();
		setWindowFlags(Qt::SubWindow);
		showMinimized();

		if (activeSubWindows == 1)
		{
			mdiArea()->setActiveSubWindow(NULL);
		}
		else if (activeSubWindows > 1)
		{
			ActionsManager::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, mdiArea());
		}

		SessionsManager::markSessionModified();
	}
	else if (isMinimized())
	{
		restoreState();
	}

	QMdiSubWindow::mouseReleaseEvent(event);
}

void MdiWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
	QStyleOptionTitleBar option;
	option.initFrom(this);
	option.titleBarFlags = windowFlags();
	option.titleBarState = windowState();
	option.subControls = QStyle::SC_All;
	option.activeSubControls = QStyle::SC_None;

	if (!isMinimized())
	{
		option.rect.setHeight(height() - widget()->height());
	}

	if (style()->subControlRect(QStyle::CC_TitleBar, &option, QStyle::SC_TitleBarLabel, this).contains(event->pos()))
	{
		setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
		showMaximized();
	}
}

WorkspaceWidget::WorkspaceWidget(QWidget *parent) : QWidget(parent),
	m_mdi(NULL),
	m_activeWindow(NULL),
	m_restoreTimer(0),
	m_isRestored(false)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	optionChanged(QLatin1String("Interface/NewTabOpeningAction"), SettingsManager::getValue(QLatin1String("Interface/NewTabOpeningAction")));

	if (!m_mdi)
	{
		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	}
}

void WorkspaceWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_restoreTimer)
	{
		killTimer(m_restoreTimer);

		m_restoreTimer = 0;

		connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));
	}
}

void WorkspaceWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (!m_mdi && m_activeWindow)
	{
		m_activeWindow->resize(size());
	}
}

void WorkspaceWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (!m_mdi && option == QLatin1String("Interface/NewTabOpeningAction") && value.toString() != QLatin1String("maximizeTab"))
	{
		createMdi();
	}
}

void WorkspaceWidget::createMdi()
{
	if (m_mdi)
	{
		return;
	}

	disconnect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));

	Window *activeWindow = m_activeWindow;

	m_mdi = new MdiWidget(this);
	m_mdi->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);

	layout()->addWidget(m_mdi);

	const bool wasRestored = m_isRestored;

	if (wasRestored)
	{
		m_isRestored = false;
	}

	QList<Window*> windows = findChildren<Window*>();

	for (int i = 0; i < windows.count(); ++i)
	{
		windows.at(i)->setVisible(true);

		addWindow(windows.at(i));
	}

	if (wasRestored)
	{
		setActiveWindow(activeWindow, true);
		markRestored();
	}
}

void WorkspaceWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (!m_mdi)
	{
		createMdi();
	}

	switch (identifier)
	{
		case ActionsManager::MaximizeTabAction:
			if (m_mdi->activeSubWindow())
			{
				MdiWindow *activeSubWindow = qobject_cast<MdiWindow*>(m_mdi->activeSubWindow());

				if (activeSubWindow)
				{
					activeSubWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
					activeSubWindow->showMaximized();
					activeSubWindow->storeState();
				}
			}

			break;
		case ActionsManager::MinimizeTabAction:
			if (m_mdi->activeSubWindow())
			{
				MdiWindow *activeSubWindow = qobject_cast<MdiWindow*>(m_mdi->activeSubWindow());

				if (activeSubWindow)
				{
					const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();
					int activeSubWindows = 0;

					for (int i = 0; i < subWindows.count(); ++i)
					{
						if (!subWindows.at(i)->isMinimized())
						{
							++activeSubWindows;
						}
					}

					activeSubWindow->storeState();
					activeSubWindow->setWindowFlags(Qt::SubWindow);
					activeSubWindow->showMinimized();

					if (activeSubWindows == 1)
					{
						setActiveWindow(NULL);
					}
					else if (activeSubWindows > 1)
					{
						ActionsManager::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, this);
					}
				}
			}

			break;
		case ActionsManager::RestoreTabAction:
			if (m_mdi->activeSubWindow())
			{
				MdiWindow *activeSubWindow = qobject_cast<MdiWindow*>(m_mdi->activeSubWindow());

				if (activeSubWindow)
				{
					activeSubWindow->setWindowFlags(Qt::SubWindow);
					activeSubWindow->showNormal();
					activeSubWindow->storeState();
				}
			}

			break;
		case ActionsManager::AlwaysOnTopTabAction:
			if (m_mdi->activeSubWindow())
			{
				if (parameters.value(QLatin1String("isChecked")).toBool())
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
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow = qobject_cast<MdiWindow*>(subWindows.at(i));

					if (subWindow)
					{
						subWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
						subWindow->showMaximized();
						subWindow->storeState();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::MinimizeAllAction:
			{
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow = qobject_cast<MdiWindow*>(subWindows.at(i));

					if (subWindow)
					{
						subWindow->storeState();
						subWindow->setWindowFlags(Qt::SubWindow);
						subWindow->showMinimized();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				setActiveWindow(NULL);
			}

			break;
		case ActionsManager::RestoreAllAction:
			{
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows = m_mdi->subWindowList();

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow = qobject_cast<MdiWindow*>(subWindows.at(i));

					if (subWindow)
					{
						subWindow->setWindowFlags(Qt::SubWindow);
						subWindow->showNormal();
						subWindow->storeState();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::CascadeAllAction:
			triggerAction(ActionsManager::RestoreAllAction);

			m_mdi->cascadeSubWindows();

			break;
		case ActionsManager::TileAllAction:
			triggerAction(ActionsManager::RestoreAllAction);

			m_mdi->tileSubWindows();

			break;
		default:
			break;
	}

	SessionsManager::markSessionModified();
}

void WorkspaceWidget::markRestored()
{
	m_isRestored = true;

	if (m_mdi)
	{
		m_restoreTimer = startTimer(250);
	}
}

void WorkspaceWidget::addWindow(Window *window, const QRect &geometry, WindowState state, bool isAlwaysOnTop)
{
	if (window)
	{
		if (!m_mdi && (state != MaximizedWindowState || geometry.isValid()))
		{
			createMdi();
		}

		if (m_mdi)
		{
			disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));

			QMdiSubWindow *activeWindow = m_mdi->activeSubWindow();
			MdiWindow *mdiWindow = new MdiWindow(window, m_mdi);
			Action *closeAction = new Action(ActionsManager::CloseTabAction);
			closeAction->setEnabled(true);
			closeAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Close"));
			closeAction->setIcon(QIcon());

			QMenu *menu = new QMenu(mdiWindow);
			menu->addAction(closeAction);
			menu->addAction(ActionsManager::getAction(ActionsManager::RestoreTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::MinimizeTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::MaximizeTabAction, this));
			menu->addAction(ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this));
			menu->addSeparator();

			QMenu *arrangeMenu = menu->addMenu(tr("Arrange"));
			arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
			arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
			arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
			arrangeMenu->addSeparator();
			arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
			arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));

			mdiWindow->show();
			mdiWindow->lower();
			mdiWindow->setSystemMenu(menu);

			switch (state)
			{
				case MaximizedWindowState:
					mdiWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
					mdiWindow->showMaximized();

					break;
				case MinimizedWindowState:
					mdiWindow->showMinimized();

					break;
				default:
					mdiWindow->showNormal();

					break;
			}

			if (isAlwaysOnTop)
			{
				mdiWindow->setWindowFlags(mdiWindow->windowFlags() | Qt::WindowStaysOnTopHint);
			}

			if (geometry.isValid())
			{
				mdiWindow->setGeometry(geometry);
			}

			if (activeWindow)
			{
				m_mdi->setActiveSubWindow(activeWindow);
			}

			if (m_isRestored)
			{
				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeSubWindowChanged(QMdiSubWindow*)));
			}

			connect(closeAction, SIGNAL(triggered()), window, SLOT(close()));
			connect(mdiWindow, SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates)), this, SLOT(updateActions()));
			connect(window, SIGNAL(destroyed()), this, SLOT(updateActions()));
		}
		else
		{
			window->hide();
			window->setParent(this);
			window->move(0, 0);
		}

		updateActions();
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
	const int subWindowsCount = (m_mdi ? subWindows.count() : findChildren<Window*>().count());
	int maximizedSubWindows = (m_mdi ? 0 : subWindowsCount);
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

	ActionsManager::getAction(ActionsManager::MaximizeAllAction, this)->setEnabled(maximizedSubWindows < subWindowsCount);
	ActionsManager::getAction(ActionsManager::MinimizeAllAction, this)->setEnabled(minimizedSubWindows < subWindowsCount);
	ActionsManager::getAction(ActionsManager::RestoreAllAction, this)->setEnabled(restoredSubWindows < subWindowsCount);
	ActionsManager::getAction(ActionsManager::CascadeAllAction, this)->setEnabled(subWindowsCount > 0);
	ActionsManager::getAction(ActionsManager::TileAllAction, this)->setEnabled(subWindowsCount > 0);

	QMdiSubWindow *activeSubWindow = (m_mdi ? m_mdi->activeSubWindow() : NULL);

	ActionsManager::getAction(ActionsManager::MaximizeTabAction, this)->setEnabled(activeSubWindow && !activeSubWindow->windowState().testFlag(Qt::WindowMaximized));
	ActionsManager::getAction(ActionsManager::MinimizeTabAction, this)->setEnabled(activeSubWindow && !activeSubWindow->windowState().testFlag(Qt::WindowMinimized));
	ActionsManager::getAction(ActionsManager::RestoreTabAction, this)->setEnabled(!m_mdi || (activeSubWindow && (activeSubWindow->windowState().testFlag(Qt::WindowMaximized) || activeSubWindow->windowState().testFlag(Qt::WindowMinimized))));
	ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this)->setEnabled(activeSubWindow);
	ActionsManager::getAction(ActionsManager::AlwaysOnTopTabAction, this)->setChecked(activeSubWindow && activeSubWindow->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
}

void WorkspaceWidget::setActiveWindow(Window *window, bool force)
{
	if (!force && !m_isRestored && window && window->isMinimized())
	{
		return;
	}

	if (force || window != m_activeWindow)
	{
		if (m_mdi)
		{
			MdiWindow *subWindow = NULL;

			if (window)
			{
				subWindow = qobject_cast<MdiWindow*>(window->parentWidget());

				if (subWindow && subWindow->isMinimized())
				{
					subWindow->restoreState();
				}
			}

			m_mdi->setActiveSubWindow(subWindow);
		}
		else
		{
			if (window)
			{
				window->resize(size());
				window->show();
			}

			if (m_activeWindow && m_activeWindow != window)
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

}
