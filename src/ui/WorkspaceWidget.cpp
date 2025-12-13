/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionTitleBar>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

MdiWidget::MdiWidget(QWidget *parent) : QMdiArea(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);
}

bool MdiWidget::eventFilter(QObject *object, QEvent *event)
{
	switch (event->type())
	{
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			return QAbstractScrollArea::eventFilter(object, event);
		default:
			break;
	}

	return QMdiArea::eventFilter(object, event);
}

MdiWindow::MdiWindow(Window *window, MdiWidget *parent) : QMdiSubWindow(parent, Qt::SubWindow),
	m_window(window),
	m_wasMaximized(false)
{
	setWidget(window);

	parent->addSubWindow(this);

	connect(window, &Window::destroyed, this, &MdiWindow::deleteLater);
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

	SessionsManager::markSessionAsModified();
}

void MdiWindow::changeEvent(QEvent *event)
{
	QMdiSubWindow::changeEvent(event);

	if (event->type() == QEvent::WindowStateChange)
	{
		SessionsManager::markSessionAsModified();
	}
}

void MdiWindow::closeEvent(QCloseEvent *event)
{
	m_window->requestClose();

	event->ignore();
}

void MdiWindow::moveEvent(QMoveEvent *event)
{
	QMdiSubWindow::moveEvent(event);

	SessionsManager::markSessionAsModified();
}

void MdiWindow::resizeEvent(QResizeEvent *event)
{
	QMdiSubWindow::resizeEvent(event);

	SessionsManager::markSessionAsModified();
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
	option.titleBarState = static_cast<int>(windowState());
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

		SessionsManager::markSessionAsModified();
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
				mainWindow->setActiveWindowByIdentifier(0);
			}
			else
			{
				mdiArea()->setActiveSubWindow(nullptr);
			}
		}
		else if (activeSubWindows > 1)
		{
			Application::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, {}, mdiArea());
		}

		SessionsManager::markSessionAsModified();
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
	option.titleBarState = static_cast<int>(windowState());
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

Window* MdiWindow::getWindow() const
{
	return m_window;
}

WorkspaceWidget::WorkspaceWidget(MainWindow *parent) : QWidget(parent),
	m_mainWindow(parent),
	m_mdi(nullptr),
	m_activeWindow(nullptr),
	m_peekedWindow(nullptr),
	m_restoreTimer(0),
	m_isRestored(false)
{
	QVBoxLayout *layout(new QVBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	handleOptionChanged(SettingsManager::Interface_NewTabOpeningActionOption, SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption));

	if (!m_mdi)
	{
		connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &WorkspaceWidget::handleOptionChanged);
	}
}

void WorkspaceWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_restoreTimer)
	{
		killTimer(m_restoreTimer);

		m_restoreTimer = 0;

		connect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);
	}
}

void WorkspaceWidget::paintEvent(QPaintEvent *event)
{
	if (!m_mdi && !m_activeWindow)
	{
		QPainter painter(this);
		painter.fillRect(rect(), palette().brush(QPalette::Dark));
	}
	else
	{
		QWidget::paintEvent(event);
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

void WorkspaceWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape && m_mainWindow->isFullScreen())
	{
		m_mainWindow->triggerAction(ActionsManager::FullScreenAction);
	}
}

void WorkspaceWidget::createMdi()
{
	if (m_mdi)
	{
		return;
	}

	disconnect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &WorkspaceWidget::handleOptionChanged);

	Window *activeWindow(m_activeWindow);

	m_mdi = new MdiWidget(this);

	layout()->addWidget(m_mdi);

	const bool wasRestored(m_isRestored);

	if (wasRestored)
	{
		m_isRestored = false;
	}

	const QList<Window*> windows(findChildren<Window*>());

	for (int i = 0; i < windows.count(); ++i)
	{
		Window *window(windows.at(i));
		window->setVisible(true);

		addWindow(window);
	}

	if (wasRestored)
	{
		setActiveWindow(activeWindow, true);
		markAsRestored();
	}

	connect(m_mdi, &MdiWidget::customContextMenuRequested, this, [&](const QPoint &position)
	{
		ActionExecutor::Object executor(m_mainWindow, m_mainWindow);
		QMenu menu(this);
		QMenu *arrangeMenu(menu.addMenu(tr("Arrange")));
		arrangeMenu->addAction(new Action(ActionsManager::RestoreTabAction, {}, executor, arrangeMenu));
		arrangeMenu->addSeparator();
		arrangeMenu->addAction(new Action(ActionsManager::RestoreAllAction, {}, executor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::MaximizeAllAction, {}, executor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::MinimizeAllAction, {}, executor, arrangeMenu));
		arrangeMenu->addSeparator();
		arrangeMenu->addAction(new Action(ActionsManager::CascadeAllAction, {}, executor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::TileAllAction, {}, executor, arrangeMenu));

		menu.addMenu(new Menu(Menu::ToolBarsMenu, &menu));
		menu.exec(m_mdi->mapToGlobal(position));
	});
}

void WorkspaceWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	const bool hasSpecifiedWindow(parameters.contains(QLatin1String("tab")));
	Window *window(hasSpecifiedWindow ? m_mainWindow->getWindowByIdentifier(parameters[QLatin1String("tab")].toULongLong()) : nullptr);

	if (identifier == ActionsManager::PeekTabAction)
	{
		if (m_peekedWindow == window)
		{
			m_peekedWindow = nullptr;

			setActiveWindow(m_activeWindow, true);
		}
		else
		{
			m_peekedWindow = window;

			setActiveWindow((window ? window : m_activeWindow.data()), true);
		}

		return;
	}

	if (!m_mdi)
	{
		createMdi();
	}

	MdiWindow *subWindow(nullptr);

	if (!hasSpecifiedWindow)
	{
		subWindow = qobject_cast<MdiWindow*>(m_mdi->currentSubWindow());
	}
	else if (window)
	{
		subWindow = qobject_cast<MdiWindow*>(window->parentWidget());
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
				const int activeSubWindows(m_mainWindow->getWindowCount() - getWindowCount(Qt::WindowMinimized));

				if (activeSubWindows == 1)
				{
					m_mainWindow->setActiveWindowByIdentifier(0);
				}
				else if (subWindow == m_mdi->currentSubWindow() && activeSubWindows > 1)
				{
					Application::triggerAction(ActionsManager::ActivatePreviouslyUsedTabAction, {}, m_mainWindow, trigger);
				}

				subWindow->storeState();
				subWindow->setWindowFlags(Qt::SubWindow);
				subWindow->showMinimized();
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
				if (parameters.value(QLatin1String("isChecked"), !m_mainWindow->getActionState(identifier, parameters).isChecked).toBool())
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
				disconnect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *mdiWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (mdiWindow)
					{
						mdiWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
						mdiWindow->showMaximized();
						mdiWindow->storeState();
					}
				}

				connect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::MinimizeAllAction:
			{
				disconnect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *mdiWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (mdiWindow)
					{
						mdiWindow->storeState();
						mdiWindow->setWindowFlags(Qt::SubWindow);
						mdiWindow->showMinimized();
					}
				}

				connect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				m_mainWindow->setActiveWindowByIdentifier(0);
			}

			break;
		case ActionsManager::RestoreAllAction:
			{
				disconnect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				const QList<QMdiSubWindow*> subWindows(m_mdi->subWindowList());

				for (int i = 0; i < subWindows.count(); ++i)
				{
					MdiWindow *mdiWindow(qobject_cast<MdiWindow*>(subWindows.at(i)));

					if (mdiWindow)
					{
						mdiWindow->setWindowFlags(Qt::SubWindow);
						mdiWindow->showNormal();
						mdiWindow->storeState();
					}
				}

				connect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

				setActiveWindow(m_activeWindow, true);
			}

			break;
		case ActionsManager::CascadeAllAction:
			triggerAction(ActionsManager::RestoreAllAction, {}, trigger);

			m_mdi->cascadeSubWindows();

			break;
		case ActionsManager::TileAllAction:
			triggerAction(ActionsManager::RestoreAllAction, {}, trigger);

			m_mdi->tileSubWindows();

			break;
		default:
			break;
	}

	SessionsManager::markSessionAsModified();
}

void WorkspaceWidget::markAsRestored()
{
	m_isRestored = true;

	if (m_mdi)
	{
		m_restoreTimer = startTimer(250);
	}
}

void WorkspaceWidget::addWindow(Window *window, const Session::Window::State &state, bool isAlwaysOnTop)
{
	if (!window)
	{
		return;
	}

	if (!m_mdi && (state.state != Qt::WindowMaximized || state.geometry.isValid()))
	{
		createMdi();
	}

	if (m_mdi)
	{
		disconnect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);

		ActionExecutor::Object mainWindowExecutor(m_mainWindow, m_mainWindow);
		ActionExecutor::Object windowExecutor(window, window);
		QMdiSubWindow *activeWindow(m_mdi->currentSubWindow());
		MdiWindow *mdiWindow(new MdiWindow(window, m_mdi));
		QMenu *menu(new QMenu(mdiWindow));
		Action *closeAction(new Action(ActionsManager::CloseTabAction, {}, windowExecutor, menu));
		closeAction->setIconOverride(QIcon());
		closeAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Close"));

		menu->addAction(closeAction);
		menu->addAction(new Action(ActionsManager::RestoreTabAction, {}, windowExecutor, menu));
		menu->addAction(new Action(ActionsManager::MinimizeTabAction, {}, windowExecutor, menu));
		menu->addAction(new Action(ActionsManager::MaximizeTabAction, {}, windowExecutor, menu));
		menu->addAction(new Action(ActionsManager::AlwaysOnTopTabAction, {}, windowExecutor, menu));
		menu->addSeparator();

		QMenu *arrangeMenu(menu->addMenu(tr("Arrange")));
		arrangeMenu->addAction(new Action(ActionsManager::RestoreAllAction, {}, mainWindowExecutor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::MaximizeAllAction, {}, mainWindowExecutor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::MinimizeAllAction, {}, mainWindowExecutor, arrangeMenu));
		arrangeMenu->addSeparator();
		arrangeMenu->addAction(new Action(ActionsManager::CascadeAllAction, {}, mainWindowExecutor, arrangeMenu));
		arrangeMenu->addAction(new Action(ActionsManager::TileAllAction, {}, mainWindowExecutor, arrangeMenu));

		mdiWindow->show();
		mdiWindow->lower();
		mdiWindow->setSystemMenu(menu);

		switch (state.state)
		{
			case Qt::WindowMaximized:
				mdiWindow->setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
				mdiWindow->showMaximized();

				break;
			case Qt::WindowMinimized:
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

		if (state.geometry.isValid())
		{
			mdiWindow->setGeometry(state.geometry);
		}

		if (activeWindow)
		{
			const bool wasMinimized(mdiWindow->isMinimized());

			if (wasMinimized)
			{
				mdiWindow->showNormal();
			}

			m_mdi->setActiveSubWindow(activeWindow);

			if (wasMinimized)
			{
				mdiWindow->showMinimized();
			}
		}

		if (m_isRestored)
		{
			connect(m_mdi, &MdiWidget::subWindowActivated, this, &WorkspaceWidget::handleActiveSubWindowChanged);
		}

		connect(mdiWindow, &MdiWindow::windowStateChanged, this, &WorkspaceWidget::notifyActionsStateChanged);
		connect(window, &Window::destroyed, this, &WorkspaceWidget::notifyActionsStateChanged);
	}
	else
	{
		window->hide();
		window->move(0, 0);
	}

	if (m_isRestored)
	{
		notifyActionsStateChanged();
	}
}

void WorkspaceWidget::handleActiveSubWindowChanged(QMdiSubWindow *subWindow)
{
	if (subWindow)
	{
		const Window *window(qobject_cast<Window*>(subWindow->widget()));

		if (window && window->isActive())
		{
			m_mainWindow->setActiveWindowByIdentifier(window->getIdentifier());
		}
	}
	else
	{
		notifyActionsStateChanged();
	}
}

void WorkspaceWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	if (!m_mdi && identifier == SettingsManager::Interface_NewTabOpeningActionOption && value.toString() != QLatin1String("maximizeTab"))
	{
		createMdi();
	}
}

void WorkspaceWidget::notifyActionsStateChanged()
{
	if (!m_mainWindow->isAboutToClose())
	{
		emit arbitraryActionsStateChanged({ActionsManager::MaximizeAllAction, ActionsManager::MinimizeAllAction, ActionsManager::RestoreAllAction, ActionsManager::CascadeAllAction, ActionsManager::TileAllAction});
	}
}

void WorkspaceWidget::setActiveWindow(Window *window, bool force)
{
	if (!force && (window == m_activeWindow || (!m_isRestored && window && window->isMinimized())))
	{
		return;
	}

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

				QTimer::singleShot(0, subWindow, static_cast<void(MdiWindow::*)()>(&MdiWindow::setFocus));
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

	if (window != m_peekedWindow)
	{
		m_activeWindow = window;
	}
}

Window* WorkspaceWidget::getActiveWindow() const
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
			QMdiSubWindow *window(windows.at(i));

			if (!window->windowState().testFlag(Qt::WindowMinimized) && !window->windowState().testFlag(Qt::WindowMaximized))
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
