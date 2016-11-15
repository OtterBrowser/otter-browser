/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../ui/WorkspaceWidget.h"
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

void WindowsManager::triggerAction(int identifier, const QVariantMap &parameters)
{
	Window *window(nullptr);

	if (parameters.contains(QLatin1String("window")))
	{
		window = getWindowByIdentifier(parameters[QLatin1String("window")].toULongLong());
	}
	else
	{
		window = m_mainWindow->getWorkspace()->getActiveWindow();
	}

	switch (identifier)
	{
		case ActionsManager::NewTabAction:
			open(QUrl(), NewTabOpen);

			break;
		case ActionsManager::NewTabPrivateAction:
			open(QUrl(), (NewTabOpen | PrivateOpen));

			break;
		case ActionsManager::CloneTabAction:
			if (window && window->canClone())
			{
				addWindow(window->clone(true, m_mainWindow->getWorkspace()));
			}

			break;
		case ActionsManager::PinTabAction:
			if (window)
			{
				window->setPinned(!window->isPinned());
			}

			break;
		case ActionsManager::DetachTabAction:
			if (window && m_mainWindow->getTabBar()->count() > 1)
			{
				OpenHints hints(NewWindowOpen);

				if (window->isPrivate())
				{
					hints |= PrivateOpen;
				}

				window->getContentsWidget()->setParent(nullptr);

				Window *newWindow(openWindow(window->getContentsWidget(), hints));

				if (newWindow && window->isPinned())
				{
					newWindow->setPinned(true);
				}

				m_mainWindow->getTabBar()->removeTab(getWindowIndex(window->getIdentifier()));

				Action *closePrivateTabsAction(m_mainWindow->getAction(ActionsManager::ClosePrivateTabsAction));

				if (closePrivateTabsAction->isEnabled() && getWindowCount(true) == 0)
				{
					closePrivateTabsAction->setEnabled(false);
				}

				m_windows.remove(window->getIdentifier());

				emit windowRemoved(window->getIdentifier());
			}

			break;
		case ActionsManager::CloseTabAction:
			if (window)
			{
				close(getWindowIndex(window->getIdentifier()));
			}

			break;
		case ActionsManager::CloseOtherTabsAction:
			if (window)
			{
				const int index(getWindowIndex(window->getIdentifier()));

				if (index >= 0)
				{
					closeOther(index);
				}
			}

			break;
		case ActionsManager::ClosePrivateTabsAction:
			m_mainWindow->getAction(ActionsManager::ClosePrivateTabsAction)->setEnabled(false);

			for (int i = (m_mainWindow->getTabBar()->count() - 1); i > 0; --i)
			{
				if (getWindowByIndex(i)->isPrivate())
				{
					close(i);
				}
			}

			break;
		case ActionsManager::OpenUrlAction:
			{
				QUrl url;
				OpenHints hints(DefaultOpen);

				if (parameters.contains(QLatin1String("url")))
				{
					url = ((parameters[QLatin1String("url")].type() == QVariant::Url) ? parameters[QLatin1String("url")].toUrl() : QUrl::fromUserInput(parameters[QLatin1String("url")].toString()));
				}

				if (parameters.contains(QLatin1String("hints")))
				{
					hints = static_cast<OpenHints>(parameters[QLatin1String("hints")].toInt());
				}

				open(url, hints);
			}

			break;
		case ActionsManager::ReopenTabAction:
			restore();

			break;
		case ActionsManager::StopAllAction:
			for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
			{
				getWindowByIndex(i)->triggerAction(ActionsManager::StopAction);
			}

			break;
		case ActionsManager::ReloadAllAction:
			for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
			{
				getWindowByIndex(i)->triggerAction(ActionsManager::ReloadAction);
			}

			break;
		case ActionsManager::ActivatePreviouslyUsedTabAction:
		case ActionsManager::ActivateLeastRecentlyUsedTabAction:
			{
				QMultiMap<qint64, quint64> map;
				const bool includeMinimized(parameters.contains(QLatin1String("includeMinimized")));

				for (int i = 0; i < getWindowCount(); ++i)
				{
					Window *window(getWindowByIndex(i));

					if (window && (includeMinimized || !window->isMinimized()))
					{
						map.insert(window->getLastActivity().toMSecsSinceEpoch(), window->getIdentifier());
					}
				}

				const QList<quint64> list(map.values());

				if (list.count() > 1)
				{
					setActiveWindowByIdentifier((identifier == ActionsManager::ActivatePreviouslyUsedTabAction) ? list.at(list.count() - 2) : list.first());
				}
			}

			break;
		case ActionsManager::ActivateTabAction:
			if (window)
			{
				setActiveWindowByIdentifier(window->getIdentifier());
			}

			break;
		case ActionsManager::ActivateTabOnLeftAction:
			m_mainWindow->getTabBar()->activateTabOnLeft();

			break;
		case ActionsManager::ActivateTabOnRightAction:
			m_mainWindow->getTabBar()->activateTabOnRight();

			break;
		case ActionsManager::BookmarkAllOpenPagesAction:
			for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
			{
				Window *window(getWindowByIndex(i));

				if (window && !Utils::isUrlEmpty(window->getUrl()))
				{
					BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, window->getUrl(), window->getTitle());
				}
			}

			break;
		case ActionsManager::OpenBookmarkAction:
			{
				OpenHints hints(DefaultOpen);

				if (parameters.contains(QLatin1String("hints")))
				{
					hints = static_cast<OpenHints>(parameters[QLatin1String("hints")].toInt());
				}

				open(BookmarksManager::getBookmark(parameters.value(QLatin1String("bookmark")).toULongLong()), hints);
			}

			break;
		default:
			if (identifier == ActionsManager::PasteAndGoAction && (!window || window->getType() != QLatin1String("web")))
			{
				window = new Window(m_isPrivate);

				addWindow(window, NewTabOpen);

				window->triggerAction(ActionsManager::PasteAndGoAction);
			}
			else if (window)
			{
				window->triggerAction(identifier, parameters);
			}

			break;
	}
}

void WindowsManager::open(const QUrl &url, OpenHints hints)
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (hints == NewTabOpen && !url.isEmpty() && window && Utils::isUrlEmpty(window->getUrl()))
	{
		hints = CurrentTabOpen;
	}

	if (hints == DefaultOpen && url.scheme() == QLatin1String("about") && !url.path().isEmpty() && url.path() != QLatin1String("blank") && url.path() != QLatin1String("start") && (!window || !Utils::isUrlEmpty(window->getUrl())))
	{
		hints = NewTabOpen;
	}

	if (hints == DefaultOpen && url.scheme() != QLatin1String("javascript") && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(SettingsManager::Browser_ReuseCurrentTabOption).toBool()))
	{
		hints = CurrentTabOpen;
	}

	if (m_isPrivate)
	{
		hints |= PrivateOpen;
	}

	if (hints.testFlag(NewWindowOpen))
	{
		Application::openWindow(hints.testFlag(PrivateOpen), hints.testFlag(BackgroundOpen), url);
	}
	else if (hints.testFlag(CurrentTabOpen) && window && window->getType() == QLatin1String("web"))
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

	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (hints == DefaultOpen && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(SettingsManager::Browser_ReuseCurrentTabOption).toBool()))
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
				const QList<QUrl> urls(bookmark->getUrls());
				bool canOpen(true);

				if (urls.count() > 1 && SettingsManager::getValue(SettingsManager::Choices_WarnOpenBookmarkFolderOption).toBool())
				{
					QMessageBox messageBox;
					messageBox.setWindowTitle(tr("Question"));
					messageBox.setText(tr("You are about to open %n bookmark(s).", "", urls.count()));
					messageBox.setInformativeText(tr("Do you want to continue?"));
					messageBox.setIcon(QMessageBox::Question);
					messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
					messageBox.setDefaultButton(QMessageBox::Yes);
					messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

					if (messageBox.exec() == QMessageBox::Cancel)
					{
						canOpen = false;
					}

					SettingsManager::setValue(SettingsManager::Choices_WarnOpenBookmarkFolderOption, !messageBox.checkBox()->isChecked());
				}

				if (urls.isEmpty() || !canOpen)
				{
					return;
				}

				open(urls.at(0), hints);

				for (int i = 1; i < urls.count(); ++i)
				{
					open(urls.at(i), ((hints == DefaultOpen || hints.testFlag(CurrentTabOpen)) ? NewTabOpen : hints));
				}
			}

			break;
		default:
			break;
	}
}

void WindowsManager::openTab(const QUrl &url, OpenHints hints)
{
	Window *window(new Window(hints.testFlag(PrivateOpen)));

	addWindow(window, hints);

	window->setUrl(((url.isEmpty() && SettingsManager::getValue(SettingsManager::StartPage_EnableStartPageOption).toBool()) ? QUrl(QLatin1String("about:start")) : url), false);
}

void WindowsManager::search(const QString &query, const QString &searchEngine, OpenHints hints)
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (hints == NewTabOpen && window && Utils::isUrlEmpty(window->getUrl()))
	{
		hints = CurrentTabOpen;
	}

	if (hints == DefaultOpen && ((window && Utils::isUrlEmpty(window->getUrl())) || SettingsManager::getValue(SettingsManager::Browser_ReuseCurrentTabOption).toBool()))
	{
		hints = CurrentTabOpen;
	}

	if (window && hints.testFlag(CurrentTabOpen) && window->getType() == QLatin1String("web"))
	{
		window->search(query, searchEngine);

		return;
	}

	if (window && window->canClone())
	{
		window = window->clone(false, m_mainWindow->getWorkspace());

		addWindow(window, hints);
	}
	else
	{
		open(QUrl(), hints);

		window = m_mainWindow->getWorkspace()->getActiveWindow();
	}

	if (window)
	{
		window->search(query, searchEngine);
	}
}

void WindowsManager::close(int index)
{
	if (index < 0 || index >= m_mainWindow->getTabBar()->count())
	{
		return;
	}

	Window *window(getWindowByIndex(index));

	if (window && !window->isPinned())
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
	int index = session.index;

	if (session.windows.isEmpty())
	{
		m_isRestored = true;

		if (SettingsManager::getValue(SettingsManager::Interface_LastTabClosingActionOption).toString() != QLatin1String("doNothing"))
		{
			open();
		}
		else
		{
			m_mainWindow->setCurrentWindow(nullptr);
		}
	}
	else
	{
		for (int i = 0; i < session.windows.count(); ++i)
		{
			Window *window(new Window(m_isPrivate));
			window->setSession(session.windows.at(i));

			if (index < 0 && session.windows.at(i).state != MinimizedWindowState)
			{
				index = i;
			}

			addWindow(window, DefaultOpen, -1, session.windows.at(i).geometry, session.windows.at(i).state, session.windows.at(i).isAlwaysOnTop);
		}
	}

	m_isRestored = true;

	connect(SessionsManager::getInstance(), SIGNAL(requestedRemoveStoredUrl(QString)), this, SLOT(removeStoredUrl(QString)));
	connect(m_mainWindow->getTabBar(), SIGNAL(currentChanged(int)), this, SLOT(setActiveWindowByIndex(int)));

	setActiveWindowByIndex(index);

	m_mainWindow->getWorkspace()->markRestored();
}

void WindowsManager::restore(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	const ClosedWindow closedWindow(m_closedWindows.at(index));
	int windowIndex(-1);

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
		const int previousIndex(getWindowIndex(closedWindow.previousWindow));

		if (previousIndex >= 0)
		{
			windowIndex = (previousIndex + 1);
		}
		else
		{
			const int nextIndex(getWindowIndex(closedWindow.nextWindow));

			if (nextIndex >= 0)
			{
				windowIndex = qMax(0, (nextIndex - 1));
			}
		}
	}

	Window *window(new Window(m_isPrivate));
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

void WindowsManager::addWindow(Window *window, OpenHints hints, int index, const QRect &geometry, WindowState state, bool isAlwaysOnTop)
{
	if (!window)
	{
		return;
	}

	m_windows[window->getIdentifier()] = window;

	if (window->isPrivate())
	{
		m_mainWindow->getAction(ActionsManager::ClosePrivateTabsAction)->setEnabled(true);
	}

	window->setControlsHidden(m_mainWindow->isFullScreen());

	if (index < 0)
	{
		index = ((!hints.testFlag(EndOpen) && SettingsManager::getValue(SettingsManager::TabBar_OpenNextToActiveOption).toBool()) ? (m_mainWindow->getTabBar()->currentIndex() + 1) : m_mainWindow->getTabBar()->count());
	}

	if (!window->isPinned())
	{
		const int offset(m_mainWindow->getTabBar()->getPinnedTabsAmount());

		if (index < offset)
		{
			index = offset;
		}
	}

	const QString newTabOpeningAction(SettingsManager::getValue(SettingsManager::Interface_NewTabOpeningActionOption).toString());

	if (m_isRestored && newTabOpeningAction == QLatin1String("maximizeTab"))
	{
		state = MaximizedWindowState;
	}

	m_mainWindow->getTabBar()->addTab(index, window);
	m_mainWindow->getWorkspace()->addWindow(window, geometry, state, isAlwaysOnTop);
	m_mainWindow->getAction(ActionsManager::CloseTabAction)->setEnabled(!window->isPinned());

	if (!hints.testFlag(BackgroundOpen) || m_mainWindow->getTabBar()->count() < 2)
	{
		m_mainWindow->getTabBar()->setCurrentIndex(index);

		if (m_isRestored)
		{
			setActiveWindowByIndex(index);
		}
	}

	if (m_isRestored)
	{
		if (newTabOpeningAction == QLatin1String("cascadeAll"))
		{
			ActionsManager::triggerAction(ActionsManager::CascadeAllAction, this);
		}
		else if (newTabOpeningAction == QLatin1String("tileAll"))
		{
			ActionsManager::triggerAction(ActionsManager::TileAllAction, this);
		}
	}

	connect(m_mainWindow, SIGNAL(controlsHiddenChanged(bool)), window, SLOT(setControlsHidden(bool)));
	connect(window, &Window::needsAttention, [&]()
	{
		QApplication::alert(m_mainWindow);
	});
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), this, SLOT(open(QUrl,WindowsManager::OpenHints)));
	connect(window, SIGNAL(requestedOpenBookmark(BookmarksItem*,WindowsManager::OpenHints)), this, SLOT(open(BookmarksItem*,WindowsManager::OpenHints)));
	connect(window, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), this, SLOT(search(QString,QString,WindowsManager::OpenHints)));
	connect(window, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
	connect(window, SIGNAL(requestedEditBookmark(QUrl)), this, SIGNAL(requestedEditBookmark(QUrl)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*,WindowsManager::OpenHints)), this, SLOT(openWindow(ContentsWidget*,WindowsManager::OpenHints)));
	connect(window, SIGNAL(requestedCloseWindow(Window*)), this, SLOT(handleWindowClose(Window*)));
	connect(window, SIGNAL(isPinnedChanged(bool)), this, SLOT(handleWindowIsPinnedChanged(bool)));

	emit windowAdded(window->getIdentifier());
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
	const int index(window ? getWindowIndex(window->getIdentifier()) : -1);

	if (index < 0)
	{
		return;
	}

	if (window && !window->isPrivate())
	{
		const WindowHistoryInformation history(window->getContentsWidget()->getHistory());

		if (!Utils::isUrlEmpty(window->getUrl()) || history.entries.count() > 1)
		{
			Window *nextWindow(getWindowByIndex(index + 1));
			Window *previousWindow((index > 0) ? getWindowByIndex(index - 1) : nullptr);

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

	const QString lastTabClosingAction(SettingsManager::getValue(SettingsManager::Interface_LastTabClosingActionOption).toString());

	if (m_mainWindow->getTabBar()->count() == 1)
	{
		if (lastTabClosingAction == QLatin1String("closeWindow") || (lastTabClosingAction == QLatin1String("closeWindowIfNotLast") && SessionsManager::getWindows().count() > 1))
		{
			m_mainWindow->triggerAction(ActionsManager::CloseWindowAction);

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
			m_mainWindow->getAction(ActionsManager::CloseTabAction)->setEnabled(false);
			m_mainWindow->setCurrentWindow(nullptr);

			emit windowTitleChanged(QString());
		}
	}

	m_mainWindow->getTabBar()->removeTab(index);

	Action *closePrivateTabsAction(m_mainWindow->getAction(ActionsManager::ClosePrivateTabsAction));

	if (closePrivateTabsAction->isEnabled() && getWindowCount(true) == 0)
	{
		closePrivateTabsAction->setEnabled(false);
	}

	emit windowRemoved(window->getIdentifier());

	m_windows.remove(window->getIdentifier());

	if (m_mainWindow->getTabBar()->count() < 1 && lastTabClosingAction == QLatin1String("openTab"))
	{
		open();
	}
}

void WindowsManager::handleWindowIsPinnedChanged(bool isPinned)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window && window == m_mainWindow->getWorkspace()->getActiveWindow())
	{
		m_mainWindow->getAction(ActionsManager::CloseTabAction)->setEnabled(!isPinned);
	}
}

void WindowsManager::setOption(int identifier, const QVariant &value)
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (window)
	{
		window->getContentsWidget()->setOption(identifier, value);
	}
}

void WindowsManager::setZoom(int zoom)
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (window)
	{
		window->getContentsWidget()->setZoom(zoom);
	}
}

void WindowsManager::setActiveWindowByIndex(int index)
{
	if (!m_isRestored || index >= m_mainWindow->getTabBar()->count())
	{
		return;
	}

	if (index != m_mainWindow->getTabBar()->currentIndex())
	{
		m_mainWindow->getTabBar()->setCurrentIndex(index);

		return;
	}

	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	if (window)
	{
		disconnect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
		disconnect(window, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
		disconnect(window, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	}

	setStatusMessage(QString());

	window = getWindowByIndex(index);

	m_mainWindow->getAction(ActionsManager::CloseTabAction)->setEnabled(window && !window->isPinned());
	m_mainWindow->setCurrentWindow(window);

	if (window)
	{
		m_mainWindow->getWorkspace()->setActiveWindow(window);

		window->setFocus();
		window->markActive();

		setStatusMessage(window->getContentsWidget()->getStatusMessage());

		emit windowTitleChanged(window->getContentsWidget()->getTitle());
		emit zoomChanged(window->getContentsWidget()->getZoom());
		emit canZoomChanged(window->getContentsWidget()->canZoom());

		connect(window, SIGNAL(statusMessageChanged(QString)), this, SLOT(setStatusMessage(QString)));
		connect(window, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
		connect(window, SIGNAL(canZoomChanged(bool)), this, SIGNAL(canZoomChanged(bool)));
	}

	m_mainWindow->getAction(ActionsManager::CloneTabAction)->setEnabled(window && window->canClone());

	emit currentWindowChanged(window ? window->getIdentifier() : 0);
}

void WindowsManager::setActiveWindowByIdentifier(quint64 identifier)
{
	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window(getWindowByIndex(i));

		if (window && window->getIdentifier() == identifier)
		{
			setActiveWindowByIndex(i);

			break;
		}
	}
}

void WindowsManager::setTitle(const QString &title)
{
	QString text(title.isEmpty() ? tr("Empty") : title);
	Window *window(qobject_cast<Window*>(sender()));

	if (!window)
	{
		return;
	}

	const int index(getWindowIndex(window->getIdentifier()));

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
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	return (window ? window->getContentsWidget()->getAction(identifier) : nullptr);
}

Window* WindowsManager::openWindow(ContentsWidget *widget, OpenHints hints)
{
	if (!widget)
	{
		return nullptr;
	}

	Window *window(nullptr);

	if (hints.testFlag(NewWindowOpen))
	{
		MainWindow *mainWindow(Application::createWindow((widget->isPrivate() ? Application::PrivateFlag : Application::NoFlags), hints.testFlag(BackgroundOpen)));

		if (mainWindow)
		{
			window = mainWindow->getWindowsManager()->openWindow(widget, (hints.testFlag(PrivateOpen) ? PrivateOpen : DefaultOpen));

			mainWindow->getWindowsManager()->closeOther();
		}
	}
	else
	{
		window = new Window((widget->isPrivate() || hints.testFlag(PrivateOpen)), widget);

		addWindow(window, hints);
	}

	return window;
}

Window* WindowsManager::getWindowByIndex(int index) const
{
	if (index == -1)
	{
		index = m_mainWindow->getTabBar()->currentIndex();
	}

	return m_mainWindow->getTabBar()->getWindow(index);
}

Window* WindowsManager::getWindowByIdentifier(quint64 identifier) const
{
	return (m_windows.contains(identifier) ? m_windows[identifier] : nullptr);
}

QVariant WindowsManager::getOption(int identifier) const
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	return (window ? window->getContentsWidget()->getOption(identifier) : QVariant());
}

QString WindowsManager::getTitle() const
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	return (window ? window->getTitle() : tr("Empty"));
}

QUrl WindowsManager::getUrl() const
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	return (window ? window->getUrl() : QUrl());
}

SessionMainWindow WindowsManager::getSession() const
{
	SessionMainWindow session;
	session.index = m_mainWindow->getTabBar()->currentIndex();

	for (int i = 0; i < m_mainWindow->getTabBar()->count(); ++i)
	{
		Window *window(getWindowByIndex(i));

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

WindowsManager::OpenHints WindowsManager::calculateOpenHints(OpenHints hints, Qt::MouseButton button, int modifiers)
{
	const bool useNewTab(!hints.testFlag(NewWindowOpen) && SettingsManager::getValue(SettingsManager::Browser_OpenLinksInNewTabOption).toBool());
	const Qt::KeyboardModifiers keyboardModifiers((modifiers == -1) ? QGuiApplication::keyboardModifiers() : static_cast<Qt::KeyboardModifiers>(modifiers));

	if (button == Qt::MiddleButton && keyboardModifiers.testFlag(Qt::AltModifier))
	{
		return ((useNewTab ? NewTabOpen : NewWindowOpen) | BackgroundOpen | EndOpen);
	}

	if (keyboardModifiers.testFlag(Qt::ControlModifier) || button == Qt::MiddleButton)
	{
		return ((useNewTab ? NewTabOpen : NewWindowOpen) | BackgroundOpen);
	}

	if (keyboardModifiers.testFlag(Qt::ShiftModifier))
	{
		return (useNewTab ? NewTabOpen : NewWindowOpen);
	}

	if (hints.testFlag(NewTabOpen) && !hints.testFlag(NewWindowOpen))
	{
		return (useNewTab ? NewTabOpen : NewWindowOpen);
	}

	if (SettingsManager::getValue(SettingsManager::Browser_ReuseCurrentTabOption).toBool())
	{
		return CurrentTabOpen;
	}

	return hints;
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
		Window *window(m_mainWindow->getTabBar()->getWindow(i));

		if (window && window->getIdentifier() == identifier)
		{
			return i;
		}
	}

	return -1;
}

int WindowsManager::getZoom() const
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

	return (window ? window->getContentsWidget()->getZoom() : 100);
}

bool WindowsManager::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

		if (window)
		{
			emit windowTitleChanged(window->getTitle().isEmpty() ? tr("Empty") : window->getTitle());
		}
	}

	return QObject::event(event);
}

bool WindowsManager::canZoom() const
{
	Window *window(m_mainWindow->getWorkspace()->getActiveWindow());

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
		Window *window(getWindowByIndex(i));

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
