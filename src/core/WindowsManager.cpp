/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "WindowsManager.h"
#include "Application.h"
#include "BookmarksManager.h"
#include "SettingsManager.h"
#include "Utils.h"
#include "../ui/ContentsWidget.h"
#include "../ui/MainWindow.h"
#include "../ui/MdiWidget.h"
#include "../ui/TabBarWidget.h"

#include <QtGui/QStatusTipEvent>
#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>

namespace Otter
{

WindowsManager::WindowsManager(bool isPrivate, MainWindow *parent) : QObject(parent),
	m_mainWindow(parent),
	m_isPrivate(isPrivate),
	m_isRestored(false)
{
}

void WindowsManager::triggerAction(int identifier, bool checked)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	switch (identifier)
	{
		case Action::NewTabAction:
			open(QUrl(), NewTabOpen);

			break;
		case Action::NewTabPrivateAction:
			open(QUrl(), NewPrivateTabOpen);

			break;
		case Action::CloneTabAction:
			cloneWindow(m_mainWindow->getTabBar()->currentIndex());

			break;
		case Action::PinTabAction:
			{
				const int index = m_mainWindow->getTabBar()->currentIndex();

				pinWindow(index, !m_mainWindow->getTabBar()->getTabProperty(index, QLatin1String("isPinned"), false).toBool());
			}

			break;
		case Action::DetachTabAction:
			if (m_mainWindow->getTabBar()->count() > 1)
			{
				detachWindow(m_mainWindow->getTabBar()->currentIndex());
			}

			break;
		case Action::CloseTabAction:
			close(m_mainWindow->getTabBar()->currentIndex());

			break;
		case Action::CloseOtherTabsAction:
			closeOther(m_mainWindow->getTabBar()->currentIndex());

			break;
		case Action::ClosePrivateTabsAction:
			m_mainWindow->getActionsManager()->getAction(Action::ClosePrivateTabsAction)->setEnabled(false);

			for (int i = (m_mainWindow->getTabBar()->count() - 1); i > 0; --i)
			{
				if (getWindowByIndex(i)->isPrivate())
				{
					close(i);
				}
			}

			break;
		case Action::ReopenTabAction:
			restore();

			break;
		case Action::ReloadAllAction:
			for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
			{
				getWindowByIndex(i)->triggerAction(Action::ReloadAction);
			}

			break;
		case Action::ActivateTabOnLeftAction:
			m_mainWindow->getTabBar()->activateTabOnLeft();

			break;
		case Action::ActivateTabOnRightAction:
			m_mainWindow->getTabBar()->activateTabOnRight();

			break;
		default:
			if (identifier == Action::PasteAndGoAction && (!window || window->getType() != QLatin1String("web")))
			{
				window = new Window(m_isPrivate, NULL, m_mainWindow->getMdi());

				addWindow(window, NewTabOpen);

				window->triggerAction(Action::PasteAndGoAction);
			}
			else if (window)
			{
				window->triggerAction(identifier, checked);
			}
	}
}

void WindowsManager::open(const QUrl &url, OpenHints hints)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (hints == NewTabOpen && !url.isEmpty() && window && Utils::isUrlEmpty(window->getUrl()))
	{
		hints = CurrentTabOpen;
	}

	if (hints == DefaultOpen && url.scheme() == QLatin1String("about") && !url.path().isEmpty() && url.path() != QLatin1String("blank") && url.path() != QLatin1String("start") && (!window || !Utils::isUrlEmpty(window->getUrl())))
	{
		hints = NewTabOpen;
	}

	if (hints == DefaultOpen && url.scheme() != QLatin1String("javascript") && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool()))
	{
		hints = CurrentTabOpen;
	}

	if (m_isPrivate)
	{
		hints |= PrivateOpen;
	}

	if (hints & NewWindowOpen)
	{
		emit requestedNewWindow((hints & PrivateOpen), (hints & BackgroundOpen), url);
	}
	else if ((hints & CurrentTabOpen) && window && window->getType() == QLatin1String("web"))
	{
		if (window->isPrivate() == hints.testFlag(PrivateOpen))
		{
			window->getContentsWidget()->setHistory(WindowHistoryInformation());
			window->setUrl(url, false);
		}
		else
		{
			close(m_mainWindow->getTabBar()->currentIndex());
			openTab(url, hints);
		}
	}
	else
	{
		openTab(url, hints);
	}
}

void WindowsManager::open(BookmarksItem *bookmark, OpenHints hints)
{
	if (!bookmark)
	{
		return;
	}

	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (hints == DefaultOpen && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool()))
	{
		hints = CurrentTabOpen;
	}

	switch (static_cast<BookmarksModel::BookmarkType>(bookmark->data(BookmarksModel::TypeRole).toInt()))
	{
		case BookmarksModel::UrlBookmark:
			open(QUrl(bookmark->data(BookmarksModel::UrlRole).toUrl()), hints);

			break;
		case BookmarksModel::RootBookmark:
		case BookmarksModel::FolderBookmark:
			{
				const QList<QUrl> urls = bookmark->getUrls();
				bool canOpen = true;

				if (urls.count() > 1 && SettingsManager::getValue(QLatin1String("Choices/WarnOpenBookmarkFolder")).toBool())
				{
					QMessageBox messageBox;
					messageBox.setWindowTitle(tr("Question"));
					messageBox.setText(tr("You are about to open %n bookmarks.", "", urls.count()));
					messageBox.setInformativeText(tr("Do you want to continue?"));
					messageBox.setIcon(QMessageBox::Question);
					messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
					messageBox.setDefaultButton(QMessageBox::Yes);
					messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

					if (messageBox.exec() == QMessageBox::Cancel)
					{
						canOpen = false;
					}

					SettingsManager::setValue(QLatin1String("Choices/WarnOpenBookmarkFolder"), !messageBox.checkBox()->isChecked());
				}

				if (!canOpen)
				{
					return;
				}

				open(urls.at(0), hints);

				for (int i = 1; i < urls.count(); ++i)
				{
					open(urls.at(i), ((hints == DefaultOpen || (hints & CurrentTabOpen)) ? NewTabOpen : hints));
				}
			}

			break;
		default:
			break;
	}
}

void WindowsManager::openTab(const QUrl &url, OpenHints hints)
{
	Window *window = new Window((hints & PrivateOpen), NULL, m_mainWindow->getMdi());

	addWindow(window, hints);

	window->setUrl(url, false);
}

void WindowsManager::search(const QString &query, const QString &engine, OpenHints hints)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (hints == NewTabOpen && window && Utils::isUrlEmpty(window->getUrl()))
	{
		hints = CurrentTabOpen;
	}

	if (hints == DefaultOpen && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool()))
	{
		hints = CurrentTabOpen;
	}

	if (window && (hints & CurrentTabOpen) && window->getType() == QLatin1String("web"))
	{
		window->search(query, engine);

		return;
	}

	if (window && window->canClone())
	{
		window = window->clone(false, m_mainWindow->getMdi());

		addWindow(window, hints);
	}
	else
	{
		open(QUrl(), hints);

		window = m_mainWindow->getMdi()->getActiveWindow();
	}

	if (window)
	{
		window->search(query, engine);
	}
}

void WindowsManager::close(int index)
{
	if (index < 0 || index >= m_mainWindow->getTabBar()->count() || m_mainWindow->getTabBar()->getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		return;
	}

	Window *window = getWindowByIndex(index);

	if (window)
	{
		window->close();
	}
}

void WindowsManager::closeOther(int index)
{
	if (index < 0)
	{
		index = m_mainWindow->getTabBar()->currentIndex();
	}

	if (index < 0 || index >= m_mainWindow->getTabBar()->count())
	{
		return;
	}

	for (int i = (m_mainWindow->getTabBar()->count() - 1); i > index; --i)
	{
		close(i);
	}

	for (int i = (index - 1); i >= 0; --i)
	{
		close(i);
	}
}

void WindowsManager::closeAll()
{
	for (int i = (m_mainWindow->getTabBar()->count() - 1); i >= 0; --i)
	{
		close(i);
	}
}

void WindowsManager::restore(const SessionMainWindow &session)
{
	if (session.windows.isEmpty())
	{
		if (SettingsManager::getValue(QLatin1String("TabBar/LastTabClosingAction")).toString() != QLatin1String("doNothing"))
		{
			open();
		}
		else
		{
			m_mainWindow->getActionsManager()->setCurrentWindow(NULL);
		}
	}
	else
	{
		for (int i = 0; i < session.windows.count(); ++i)
		{
			Window *window = new Window(m_isPrivate, NULL, m_mainWindow->getMdi());
			window->setSession(session.windows.at(i));

			addWindow(window);
		}
	}

	m_isRestored = true;

	connect(SessionsManager::getInstance(), SIGNAL(requestedRemoveStoredUrl(QString)), this, SLOT(removeStoredUrl(QString)));
	connect(m_mainWindow->getTabBar(), SIGNAL(currentChanged(int)), this, SLOT(setActiveWindowByIndex(int)));
	connect(m_mainWindow->getTabBar(), SIGNAL(requestedClone(int)), this, SLOT(cloneWindow(int)));
	connect(m_mainWindow->getTabBar(), SIGNAL(requestedDetach(int)), this, SLOT(detachWindow(int)));
	connect(m_mainWindow->getTabBar(), SIGNAL(requestedPin(int,bool)), this, SLOT(pinWindow(int,bool)));
	connect(m_mainWindow->getTabBar(), SIGNAL(requestedClose(int)), this, SLOT(close(int)));
	connect(m_mainWindow->getTabBar(), SIGNAL(requestedCloseOther(int)), this, SLOT(closeOther(int)));

	setActiveWindowByIndex(session.index);
}

void WindowsManager::restore(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	const ClosedWindow closedWindow = m_closedWindows.at(index);
	int windowIndex = -1;

	if (closedWindow.previousWindow == 0)
	{
		windowIndex = 0;
	}
	else if (closedWindow.nextWindow == 0)
	{
		windowIndex = m_mainWindow->getTabBar()->count();
	}
	else
	{
		const int previousIndex = getWindowIndex(closedWindow.previousWindow);

		if (previousIndex >= 0)
		{
			windowIndex = (previousIndex + 1);
		}
		else
		{
			const int nextIndex = getWindowIndex(closedWindow.nextWindow);

			if (nextIndex >= 0)
			{
				windowIndex = qMax(0, (nextIndex - 1));
			}
		}
	}

	Window *window = new Window(m_isPrivate, NULL, m_mainWindow->getMdi());
	window->setSession(closedWindow.window);

	m_closedWindows.removeAt(index);

	if (m_closedWindows.isEmpty() && SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}

	addWindow(window, DefaultOpen, windowIndex);
}

void WindowsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	if (SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}
}

void WindowsManager::addWindow(Window *window, OpenHints hints, int index)
{
	if (!window)
	{
		return;
	}

	if (window->isPrivate())
	{
		m_mainWindow->getActionsManager()->getAction(Action::ClosePrivateTabsAction)->setEnabled(true);
	}

	window->setControlsHidden(m_mainWindow->isFullScreen());

	if (index < 0)
	{
		index = ((!(hints & EndOpen) && SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool()) ? (m_mainWindow->getTabBar()->currentIndex() + 1) : m_mainWindow->getTabBar()->count());
	}

	if (!window->isPinned())
	{
		const int offset = m_mainWindow->getTabBar()->getPinnedTabsAmount();

		if (index < offset)
		{
			index = offset;
		}
	}

	m_mainWindow->getTabBar()->addTab(index, window);
	m_mainWindow->getMdi()->addWindow(window);

	if (!m_mainWindow->getActionsManager()->getAction(Action::CloseTabAction)->isEnabled())
	{
		m_mainWindow->getActionsManager()->getAction(Action::CloseTabAction)->setEnabled(true);
	}

	if (!(hints & BackgroundOpen))
	{
		m_mainWindow->getTabBar()->setCurrentIndex(index);

		if (m_isRestored)
		{
			setActiveWindowByIndex(index);
		}
	}

	connect(m_mainWindow, SIGNAL(controlsHiddenChanged(bool)), window, SLOT(setControlsHidden(bool)));
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(open(QUrl,OpenHints)));
	connect(window, SIGNAL(requestedOpenBookmark(BookmarksItem*,OpenHints)), this, SLOT(open(BookmarksItem*,OpenHints)));
	connect(window, SIGNAL(requestedSearch(QString,QString,OpenHints)), this, SLOT(search(QString,QString,OpenHints)));
	connect(window, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)), this, SLOT(openWindow(ContentsWidget*,OpenHints)));
	connect(window, SIGNAL(requestedCloseWindow(Window*)), this, SLOT(handleWindowClose(Window*)));

	emit windowAdded(window->getIdentifier());
}

void WindowsManager::openWindow(ContentsWidget *widget, OpenHints hints)
{
	if (!widget)
	{
		return;
	}

	if (hints & NewWindowOpen)
	{
		MainWindow *mainWindow = Application::getInstance()->createWindow(widget->isPrivate(), (hints & BackgroundOpen));

		if (mainWindow)
		{
			mainWindow->getWindowsManager()->openWindow(widget);
			mainWindow->getWindowsManager()->closeOther();
		}
	}
	else
	{
		addWindow(new Window(widget->isPrivate(), widget, m_mainWindow->getMdi()), hints);
	}
}

void WindowsManager::cloneWindow(int index)
{
	Window *window = getWindowByIndex(index);

	if (window && window->canClone())
	{
		addWindow(window->clone(true, m_mainWindow->getMdi()));
	}
}

void WindowsManager::detachWindow(int index)
{
	Window *window = getWindowByIndex(index);

	if (!window)
	{
		return;
	}

	MainWindow *mainWindow = Application::getInstance()->createWindow(window->isPrivate(), true);

	if (mainWindow)
	{
		window->getContentsWidget()->setParent(NULL);

		mainWindow->getWindowsManager()->openWindow(window->getContentsWidget());
		mainWindow->getWindowsManager()->closeOther();

		m_mainWindow->getTabBar()->removeTab(index);

		Action *closePrivateTabsAction = m_mainWindow->getActionsManager()->getAction(Action::ClosePrivateTabsAction);

		if (closePrivateTabsAction->isEnabled() && getWindowCount(true) == 0)
		{
			closePrivateTabsAction->setEnabled(false);
		}

		emit windowRemoved(window->getIdentifier());
	}
}

void WindowsManager::pinWindow(int index, bool pin)
{
	m_mainWindow->getTabBar()->setTabProperty(index, QLatin1String("isPinned"), pin);
}

void WindowsManager::removeStoredUrl(const QString &url)
{
	for (int i = (m_closedWindows.count() - 1); i >= 0; --i)
	{
		if (url == m_closedWindows.at(i).window.getUrl())
		{
			m_closedWindows.removeAt(i);

			break;
		}
	}

	if (m_closedWindows.isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}
}

void WindowsManager::handleWindowClose(Window *window)
{
	const int index = (window ? getWindowIndex(window->getIdentifier()) : -1);

	if (index < 0)
	{
		return;
	}

	if (window && !window->isPrivate())
	{
		const WindowHistoryInformation history = window->getContentsWidget()->getHistory();

		if (!Utils::isUrlEmpty(window->getUrl()) || history.entries.count() > 1)
		{
			Window *nextWindow = getWindowByIndex(index + 1);
			Window *previousWindow = ((index > 0) ? getWindowByIndex(index - 1) : NULL);

			ClosedWindow closedWindow;
			closedWindow.window = window->getSession();
			closedWindow.nextWindow = (nextWindow ? nextWindow->getIdentifier() : 0);
			closedWindow.previousWindow = (previousWindow ? previousWindow->getIdentifier() : 0);

			if (window->getType() != QLatin1String("web"))
			{
				removeStoredUrl(closedWindow.window.getUrl());
			}

			m_closedWindows.prepend(closedWindow);

			emit closedWindowsAvailableChanged(true);
		}
	}

	const QString lastTabClosingAction = SettingsManager::getValue(QLatin1String("TabBar/LastTabClosingAction")).toString();

	if (m_mainWindow->getTabBar()->count() == 1)
	{
		if (lastTabClosingAction == QLatin1String("closeWindow"))
		{
			m_mainWindow->getActionsManager()->triggerAction(Action::CloseWindowAction);

			return;
		}

		if (lastTabClosingAction == QLatin1String("openTab"))
		{
			window = getWindowByIndex(0);

			if (window)
			{
				window->clear();

				return;
			}
		}
		else
		{
			m_mainWindow->getActionsManager()->getAction(Action::CloseTabAction)->setEnabled(false);;
			m_mainWindow->getActionsManager()->setCurrentWindow(NULL);

			emit windowTitleChanged(QString());
		}
	}

	m_mainWindow->getTabBar()->removeTab(index);

	Action *closePrivateTabsAction = m_mainWindow->getActionsManager()->getAction(Action::ClosePrivateTabsAction);

	if (closePrivateTabsAction->isEnabled() && getWindowCount(true) == 0)
	{
		closePrivateTabsAction->setEnabled(false);
	}

	emit windowRemoved(window->getIdentifier());

	if (m_mainWindow->getTabBar()->count() < 1 && lastTabClosingAction != QLatin1String("doNothing"))
	{
		open();
	}
}

void WindowsManager::setOption(const QString &key, const QVariant &value)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (window)
	{
		window->setOption(key, value);
	}
}

void WindowsManager::setZoom(int zoom)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (window)
	{
		window->getContentsWidget()->setZoom(zoom);
	}
}

void WindowsManager::setActiveWindowByIndex(int index)
{
	if (index < 0 || index >= m_mainWindow->getTabBar()->count())
	{
		index = 0;
	}

	if (index != m_mainWindow->getTabBar()->currentIndex())
	{
		m_mainWindow->getTabBar()->setCurrentIndex(index);

		return;
	}

	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	if (window)
	{
		disconnect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
		disconnect(window, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
		disconnect(window, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	}

	setStatusMessage(QString());

	window = getWindowByIndex(index);

	m_mainWindow->getActionsManager()->setCurrentWindow(window);

	if (window)
	{
		m_mainWindow->getMdi()->setActiveWindow(window);

		window->setFocus();
		window->markActive();

		setStatusMessage(window->getContentsWidget()->getStatusMessage());

		emit canZoomChanged(window->getContentsWidget()->canZoom());
		emit zoomChanged(window->getContentsWidget()->getZoom());
		emit windowTitleChanged(window->getContentsWidget()->getTitle());

		connect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
		connect(window, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
		connect(window, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
	}

	m_mainWindow->getActionsManager()->getAction(Action::CloneTabAction)->setEnabled(window && window->canClone());

	emit currentWindowChanged(window ? window->getIdentifier() : -1);
}

void WindowsManager::setActiveWindowByIdentifier(quint64 identifier)
{
	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window = getWindowByIndex(i);

		if (window && window->getIdentifier() == identifier)
		{
			setActiveWindowByIndex(i);

			break;
		}
	}
}

void WindowsManager::setTitle(const QString &title)
{
	QString text = (title.isEmpty() ? tr("Empty") : title);
	Window *window = qobject_cast<Window*>(sender());

	if (!window)
	{
		return;
	}

	const int index = getWindowIndex(window->getIdentifier());

	if (index == m_mainWindow->getTabBar()->currentIndex())
	{
		emit windowTitleChanged(text);
	}

	m_mainWindow->getTabBar()->setTabText(index, text.replace(QLatin1Char('&'), QLatin1String("&&")));
}

void WindowsManager::setStatusMessage(const QString &message)
{
	QStatusTipEvent event(message);

	QApplication::sendEvent(m_mainWindow, &event);
}

Action* WindowsManager::getAction(int identifier)
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getContentsWidget()->getAction(identifier) : NULL);
}

Window* WindowsManager::getWindowByIndex(int index) const
{
	return ((index >= m_mainWindow->getTabBar()->count()) ? NULL : qvariant_cast<Window*>(m_mainWindow->getTabBar()->tabData(index)));
}

Window* WindowsManager::getWindowByIdentifier(quint64 identifier) const
{
	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window = getWindowByIndex(i);

		if (window && window->getIdentifier() == identifier)
		{
			return window;
		}
	}

	return NULL;
}

QVariant WindowsManager::getOption(const QString &key) const
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getOption(key) : QVariant());
}

QString WindowsManager::getTitle() const
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getTitle() : tr("Empty"));
}

QUrl WindowsManager::getUrl() const
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getUrl() : QUrl());
}

SessionMainWindow WindowsManager::getSession() const
{
	SessionMainWindow session;
	session.index = m_mainWindow->getTabBar()->currentIndex();

	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window = getWindowByIndex(i);

		if (window && !window->isPrivate())
		{
			session.windows.append(window->getSession());
		}
		else if (i < session.index)
		{
			--session.index;
		}
	}

	return session;
}

QList<ClosedWindow> WindowsManager::getClosedWindows() const
{
	return m_closedWindows;
}

OpenHints WindowsManager::calculateOpenHints(Qt::KeyboardModifiers modifiers, Qt::MouseButton button, OpenHints hints)
{
	const bool useNewTab = SettingsManager::getValue(QLatin1String("Browser/OpenLinksInNewTab")).toBool();

	if (modifiers == Qt::NoModifier)
	{
		modifiers = QGuiApplication::keyboardModifiers();
	}

	if (button == Qt::MiddleButton && modifiers.testFlag(Qt::AltModifier))
	{
		return (useNewTab ? NewBackgroundTabEndOpen : NewBackgroundWindowEndOpen);
	}

	if (modifiers.testFlag(Qt::ControlModifier) || button == Qt::MiddleButton)
	{
		return (useNewTab ? NewBackgroundTabOpen : NewBackgroundWindowOpen);
	}

	if (modifiers.testFlag(Qt::ShiftModifier))
	{
		return (useNewTab ? NewTabOpen : NewWindowOpen);
	}

	if (hints != DefaultOpen)
	{
		return hints;
	}

	if (SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool())
	{
		return CurrentTabOpen;
	}

	return (useNewTab ? NewTabOpen : NewWindowOpen);
}

int WindowsManager::getWindowCount(bool onlyPrivate) const
{
	if (!onlyPrivate || isPrivate())
	{
		return m_mainWindow->getTabBar()->count();
	}

	int amount = 0;

	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		if (getWindowByIndex(i)->isPrivate())
		{
			++amount;
		}
	}

	return amount;
}

int WindowsManager::getWindowIndex(quint64 identifier) const
{
	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window = qvariant_cast<Window*>(m_mainWindow->getTabBar()->tabData(i));

		if (window && identifier == window->getIdentifier())
		{
			return i;
		}
	}

	return -1;
}

int WindowsManager::getZoom() const
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getContentsWidget()->getZoom() : 100);
}

bool WindowsManager::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
		{
			Window *window = getWindowByIndex(i);

			if (window)
			{
				QString text = (window->getTitle().isEmpty() ? tr("Empty") : window->getTitle());

				if (i == m_mainWindow->getTabBar()->currentIndex())
				{
					emit windowTitleChanged(text);
				}

				m_mainWindow->getTabBar()->setTabText(i, text.replace(QLatin1Char('&'), QLatin1String("&&")));
			}
		}
	}

	return QObject::event(event);
}

bool WindowsManager::canZoom() const
{
	Window *window = m_mainWindow->getMdi()->getActiveWindow();

	return (window ? window->getContentsWidget()->canZoom() : false);
}

bool WindowsManager::isPrivate() const
{
	return m_isPrivate;
}

bool WindowsManager::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window = getWindowByIndex(i);

		if (window && window->getUrl() == url)
		{
			if (activate)
			{
				setActiveWindowByIndex(i);
			}

			return true;
		}
	}

	return false;
}

}
