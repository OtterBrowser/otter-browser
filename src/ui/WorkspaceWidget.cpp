/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WorkspaceWidget.h"
#include "Action.h"
#include "MainWindow.h"
#include "Menu.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/SettingsManager.h"

#include <QtCore/QTimer>
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
	m_window(window),
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
	m_window->close();

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

void MdiWindow::focusInEvent(QFocusEvent *event)
{
	QMdiSubWindow::focusInEvent(event);

	m_window->setFocus(Qt::ActiveWindowFocusReason);
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
		const QList<QMdiSubWindow*> subWindows(mdiArea()->subWindowList());
		int activeSubWindows(0);

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
			MainWindow *mainWindow(MainWindow::findMainWindow(mdiArea()));

			if (mainWindow)
			{
				mainWindow->setActiveWindowByIndex(-1);
			}
			else
			{
				mdiArea()->setActiveSubWindow(nullptr);
			}
		}
		else if (activeSubWindows > 1)
		{
			Application::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, QVariantMap(), mdiArea());
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

WorkspaceWidget::WorkspaceWidget(MainWindow *parent) : QWidget(parent),
	m_mainWindow(parent),
	m_mdi(nullptr),
	m_activeWindow(nullptr),
	m_restoreTimer(0),
	m_isRestored(false)
{
	QVBoxLayout *layout(new QVBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	handleOptionChanged(SettingsManager::Interface_NewTabOpeningActionOption, SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption));

	if (!m_mdi)
	{
		connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	}
}

void WorkspaceWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_restoreTimer)
	{
		killTimer(m_restoreTimer);

		m_restoreTimer = 0;

		connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));
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

void WorkspaceWidget::createMdi()
{
	if (m_mdi)
	{
		return;
	}

	disconnect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));

	Window *activeWindow(m_activeWindow);

	m_mdi = new MdiWidget(this);
	m_mdi->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);
	m_mdi->setContextMenuPolicy(Qt::CustomContextMenu);

	layout()->addWidget(m_mdi);

	const bool wasRestored(m_isRestored);

	if (wasRestored)
	{
		m_isRestored = false;
	}

	const QList<Window*> windows(findChildren<Window*>());

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

	connect(m_mdi, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

void WorkspaceWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (!m_mdi)
	{
		createMdi();
	}

	MdiWindow *subWindow(nullptr);

	if (parameters.contains(QLatin1String("window")))
	{
		Window *window(m_mainWindow->getWindowByIdentifier(parameters[QLatin1String("window")].toULongLong()));

		if (window)
		{
			subWindow = qobject_cast<MdiWindow*>(window->parentWidget());
		}
	}
	else
	{
		subWindow = qobject_cast<MdiWindow*>(m_mdi->currentSubWindow());
	}

	switch (identifier)
	{
		case ActionsManager::MaximizeTabAction:
			if (subWindow)
			{
				subWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
				subWindow->showMaximized();
				subWindow->storeState();

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::MinimizeTabAction:
			if (subWindow)
			{
				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());
				int activeSubWindows(0);
				const bool wasActive(subWindow == m_mdi->currentSubWindow());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					if (!subWindows.at(i)->isMinimized())
					{
						++activeSubWindows;
					}
				}

				subWindow->storeState();
				subWindow->setWindowFlags(Qt::SubWindow);
				subWindow->showMinimized();

				if (activeSubWindows == 1)
				{
					m_mainWindow->setActiveWindowByIndex(-1);
				}
				else if (wasActive && activeSubWindows > 1)
				{
					Application::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, QVariantMap(), m_mainWindow);
				}
			}

			break;
		case ActionsManager::RestoreTabAction:
			if (subWindow)
			{
				subWindow->setWindowFlags(Qt::SubWindow);
				subWindow->showNormal();
				subWindow->storeState();

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::AlwaysOnTopTabAction:
			if (subWindow)
			{
				if (Action::calculateCheckedState(parameters, m_mainWindow->getAction(ActionsManager::AlwaysOnTopTabAction)))
				{
					subWindow->setWindowFlags(subWindow->windowFlags() | Qt::WindowStaysOnTopHint);
				}
				else
				{
					subWindow->setWindowFlags(subWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
				}
			}

			break;
		case ActionsManager::MaximizeAllAction:
			{
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (subWindow)
					{
						subWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
						subWindow->showMaximized();
						subWindow->storeState();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::MinimizeAllAction:
			{
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (subWindow)
					{
						subWindow->storeState();
						subWindow->setWindowFlags(Qt::SubWindow);
						subWindow->showMinimized();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

				m_mainWindow->setActiveWindowByIndex(-1);
			}

			break;
		case ActionsManager::RestoreAllAction:
			{
				disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *subWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (subWindow)
					{
						subWindow->setWindowFlags(Qt::SubWindow);
						subWindow->showNormal();
						subWindow->storeState();
					}
				}

				connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

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
	if (!window)
	{
		return;
	}

	if (!m_mdi && (state != MaximizedWindowState || geometry.isValid()))
	{
		createMdi();
	}

	if (m_mdi)
	{
		disconnect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));

		QMdiSubWindow *activeWindow(m_mdi->currentSubWindow());
		MdiWindow *mdiWindow(new MdiWindow(window, m_mdi));
		Action *closeAction(new Action(ActionsManager::CloseTabAction));
		closeAction->setEnabled(true);
		closeAction->setOverrideText(QT_TRANSLATE_NOOP("actions", "Close"));
		closeAction->setIcon(QIcon());

		QMenu *menu(new QMenu(mdiWindow));
		menu->addAction(closeAction);
		menu->addAction(m_mainWindow->getAction(ActionsManager::RestoreTabAction));
		menu->addAction(m_mainWindow->getAction(ActionsManager::MinimizeTabAction));
		menu->addAction(m_mainWindow->getAction(ActionsManager::MaximizeTabAction));
		menu->addAction(m_mainWindow->getAction(ActionsManager::AlwaysOnTopTabAction));
		menu->addSeparator();

		QMenu *arrangeMenu(menu->addMenu(tr("Arrange")));
		arrangeMenu->addAction(m_mainWindow->getAction(ActionsManager::RestoreAllAction));
		arrangeMenu->addAction(m_mainWindow->getAction(ActionsManager::MaximizeAllAction));
		arrangeMenu->addAction(m_mainWindow->getAction(ActionsManager::MinimizeAllAction));
		arrangeMenu->addSeparator();
		arrangeMenu->addAction(m_mainWindow->getAction(ActionsManager::CascadeAllAction));
		arrangeMenu->addAction(m_mainWindow->getAction(ActionsManager::TileAllAction));

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
			connect(m_mdi, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(handleActiveSubWindowChanged(QMdiSubWindow*)));
		}

		connect(closeAction, SIGNAL(triggered()), window, SLOT(close()));
		connect(mdiWindow, SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates)), this, SLOT(updateActions()));
		connect(window, SIGNAL(destroyed()), this, SLOT(updateActions()));
	}
	else
	{
		window->hide();
		window->move(0, 0);
	}

	updateActions();
}

void WorkspaceWidget::handleActiveSubWindowChanged(QMdiSubWindow *subWindow)
{
	if (subWindow)
	{
		Window *window(qobject_cast<Window*>(subWindow->widget()));

		if (window)
		{
			m_mainWindow->setActiveWindowByIdentifier(window->getIdentifier());
		}
	}
	else
	{
		updateActions();
	}
}

void WorkspaceWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	if (!m_mdi && identifier == SettingsManager::Interface_NewTabOpeningActionOption && value.toString() != QLatin1String("maximizeTab"))
	{
		createMdi();
	}
}

void WorkspaceWidget::updateActions()
{
	const Qt::WindowStates state((m_mdi && m_mdi->currentSubWindow()) ? m_mdi->currentSubWindow()->windowState() : Qt::WindowNoState);
	const int windowCount(m_mainWindow->getWindowCount());

	m_mainWindow->getAction(ActionsManager::MaximizeTabAction)->setEnabled(!state.testFlag(Qt::WindowMaximized));
	m_mainWindow->getAction(ActionsManager::MinimizeTabAction)->setEnabled(!state.testFlag(Qt::WindowMinimized));
	m_mainWindow->getAction(ActionsManager::RestoreTabAction)->setEnabled(state.testFlag(Qt::WindowMaximized) || state.testFlag(Qt::WindowMinimized));
	m_mainWindow->getAction(ActionsManager::AlwaysOnTopTabAction)->setChecked(m_mdi && m_mdi->currentSubWindow() && m_mdi->currentSubWindow()->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
	m_mainWindow->getAction(ActionsManager::MaximizeAllAction)->setEnabled(getWindowCount(Qt::WindowMaximized) != windowCount);
	m_mainWindow->getAction(ActionsManager::MinimizeAllAction)->setEnabled(getWindowCount(Qt::WindowMinimized) != windowCount);
	m_mainWindow->getAction(ActionsManager::RestoreAllAction)->setEnabled(getWindowCount(Qt::WindowNoState) != windowCount);
	m_mainWindow->getAction(ActionsManager::CascadeAllAction)->setEnabled(windowCount > 0);
	m_mainWindow->getAction(ActionsManager::TileAllAction)->setEnabled(windowCount > 0);
}

void WorkspaceWidget::showContextMenu(const QPoint &position)
{
	QMenu menu(this);
	QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreTabAction, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::RestoreAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MaximizeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::MinimizeAllAction, this));
	arrangeMenu->addSeparator();
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::CascadeAllAction, this));
	arrangeMenu->addAction(ActionsManager::getAction(ActionsManager::TileAllAction, this));

	menu.addMenu(new Menu(Menu::ToolBarsMenuRole, &menu))->setText(tr("Toolbars"));
	menu.exec(m_mdi->mapToGlobal(position));
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
			MdiWindow *subWindow(nullptr);

			if (window)
			{
				subWindow = qobject_cast<MdiWindow*>(window->parentWidget());

				if (subWindow)
				{
					if (subWindow->isMinimized())
					{
						subWindow->restoreState();
					}

					if (!m_activeWindow)
					{
						subWindow->raise();
					}

					QTimer::singleShot(0, subWindow, SLOT(setFocus()));
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

int WorkspaceWidget::getWindowCount(Qt::WindowState state) const
{
	if (!m_mdi)
	{
		switch (state)
		{
			case Qt::WindowNoState:
				return m_mainWindow->getWindowCount();
			case Qt::WindowActive:
				return (m_activeWindow ? 1 : 0);
			default:
				break;
		}

		return 0;
	}

	const QList<QMdiSubWindow*> windows(m_mdi->subWindowList());
	int amount(0);

	if (state == Qt::WindowNoState)
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			if (!windows.at(i)->windowState().testFlag(Qt::WindowMinimized) && !windows.at(i)->windowState().testFlag(Qt::WindowMaximized))
			{
				++amount;
			}
		}
	}
	else
	{
		for (int i = 0; i < windows.count(); ++i)
		{
			if (windows.at(i)->windowState().testFlag(state))
			{
				++amount;
			}
		}
	}

	return amount;
}

}
