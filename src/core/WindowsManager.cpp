/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SettingsManager.h"
#include "../ui/ContentsWidget.h"
#include "../ui/MainWindow.h"
#include "../ui/StatusBarWidget.h"
#include "../ui/TabBarWidget.h"

#include <QtGui/QPainter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QAction>
#include <QtWidgets/QMdiSubWindow>

namespace Otter
{

WindowsManager::WindowsManager(QMdiArea *area, TabBarWidget *tabBar, StatusBarWidget *statusBar, bool privateSession) : QObject(area),
	m_area(area),
	m_tabBar(tabBar),
	m_statusBar(statusBar),
	m_currentWindow(-1),
	m_printedWindow(-1),
	m_privateSession(privateSession)
{
	connect(SessionsManager::getInstance(), SIGNAL(requestedRemoveStoredUrl(QString)), this, SLOT(removeStoredUrl(QString)));
	connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentWindow(int)));
	connect(m_tabBar, SIGNAL(requestedClone(int)), this, SLOT(cloneWindow(int)));
	connect(m_tabBar, SIGNAL(requestedDetach(int)), this, SLOT(detachWindow(int)));
	connect(m_tabBar, SIGNAL(requestedPin(int,bool)), this, SLOT(pinWindow(int,bool)));
	connect(m_tabBar, SIGNAL(requestedClose(int)), this, SLOT(closeWindow(int)));
	connect(m_tabBar, SIGNAL(requestedCloseOther(int)), this, SLOT(closeOther(int)));
}

void WindowsManager::open(const QUrl &url, bool privateWindow, bool background, bool newWindow)
{
	if (newWindow)
	{
		requestedNewWindow(privateWindow, background, url);

		return;
	}

	Window *window = NULL;

	privateWindow = (m_privateSession || privateWindow);

	if (!url.isEmpty())
	{
		window = getWindow(getCurrentWindow());

		if (window && window->getType() == QLatin1String("web") && window->getUrl().scheme() == QLatin1String("about") && (window->getUrl().path() == QLatin1String("blank") || window->getUrl().path().isEmpty()))
		{
			if (window->isPrivate() == privateWindow)
			{
				window->getContentsWidget()->setHistory(WindowHistoryInformation());
				window->setUrl(url);

				return;
			}

			closeWindow(getCurrentWindow());
		}
	}

	window = new Window(privateWindow, NULL, m_area);
	window->setUrl(url);

	addWindow(window, background);
}

void WindowsManager::search(const QString &query, const QString &engine)
{
	Window *window = getWindow();

	if (window && window->canClone())
	{
		window = window->clone(m_area);
		window->getContentsWidget()->setHistory(WindowHistoryInformation());

		addWindow(window);
	}
	else
	{
		open();

		window = getWindow();
	}

	if (window)
	{
		window->search(query, engine);
	}
}

void WindowsManager::close(int index)
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	closeWindow(index);
}

void WindowsManager::closeAll()
{
	for (int i = (m_tabBar->count() - 1); i >= 0; --i)
	{
		Window *window = getWindow(i);

		if (window)
		{
			window->close();
		}
	}
}

void WindowsManager::closeOther(int index)
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	if (index < 0 || index >= m_tabBar->count())
	{
		return;
	}

	for (int i = (m_tabBar->count() - 1); i > index; --i)
	{
		closeWindow(i);
	}

	for (int i = (index - 1); i >= 0; --i)
	{
		closeWindow(i);
	}
}

void WindowsManager::restore(const QList<SessionWindow> &windows)
{
	if (windows.isEmpty())
	{
		open();

		return;
	}

	m_closedWindows = windows;

	for (int i = 0; i < windows.count(); ++i)
	{
		restore(0);
	}
}

void WindowsManager::restore(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	const SessionWindow entry = m_closedWindows.at(index);

	m_closedWindows.removeAt(index);

	WindowHistoryInformation history;
	history.index = entry.index;
	history.entries = entry.history;

	Window *window = new Window(m_privateSession, NULL, m_area);
	window->setUrl(entry.url());
	window->getContentsWidget()->setHistory(history);
	window->setSearchEngine(entry.searchEngine);
	window->setPinned(entry.pinned);
	window->getContentsWidget()->setZoom(entry.zoom());

	if (m_closedWindows.isEmpty() && SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}

	addWindow(window);
}

void WindowsManager::print(int index)
{
	Window *window = getWindow(index);

	if (!window)
	{
		return;
	}

	QPrinter printer;
	QPrintDialog printDialog(&printer, m_area);
	printDialog.setWindowTitle(tr("Print Page"));

	if (printDialog.exec() != QDialog::Accepted)
	{
		return;
	}

	window->getContentsWidget()->print(&printer);
}

void WindowsManager::printPreview(int index)
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	if (index < 0 || index >= m_tabBar->count())
	{
		return;
	}

	m_printedWindow = index;

	QPrintPreviewDialog prinPreviewtDialog(m_area);
	prinPreviewtDialog.setWindowTitle(tr("Print Preview"));

	connect(&prinPreviewtDialog, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printPreview(QPrinter*)));

	prinPreviewtDialog.exec();

	m_printedWindow = -1;
}

void WindowsManager::printPreview(QPrinter *printer)
{
	Window *window = getWindow(m_printedWindow);

	if (window)
	{
		window->getContentsWidget()->print(printer);
	}
}

void WindowsManager::triggerAction(WindowAction action, bool checked)
{
	Window *window = getWindow(getCurrentWindow());

	if (window)
	{
		window->getContentsWidget()->triggerAction(action, checked);
	}
}

void WindowsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	if (SessionsManager::getClosedWindows().isEmpty())
	{
		emit closedWindowsAvailableChanged(false);
	}
}

void WindowsManager::addWindow(Window *window, bool background)
{
	if (!window)
	{
		return;
	}

	window->setParent(m_area);

	QMdiSubWindow *mdiWindow = m_area->addSubWindow(window, (Qt::SubWindow | Qt::CustomizeWindowHint));

	if (background)
	{
		mdiWindow->hide();
	}
	else
	{
		mdiWindow->showMaximized();
	}

	const int index = (SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool() ? (m_tabBar->currentIndex() + 1) : m_tabBar->count());

	m_tabBar->insertTab(index, window->getContentsWidget()->getTitle());
	m_tabBar->setTabData(index, QVariant::fromValue(window));

	if (!background)
	{
		m_tabBar->setCurrentIndex(index);

		setCurrentWindow(index);
	}

	connect(window, SIGNAL(requestedCloseWindow(Window*)), this, SLOT(closeWindow(Window*)));
	connect(window, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString)));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)), this, SLOT(open(QUrl,bool,bool,bool)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*)), this, SLOT(addWindow(ContentsWidget*)));
	connect(window, SIGNAL(requestedSearch(QString,QString)), this, SLOT(search(QString,QString)));
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(iconChanged(QIcon)), m_tabBar, SLOT(updateTabs()));
	connect(window, SIGNAL(loadingChanged(bool)), m_tabBar, SLOT(updateTabs()));
	connect(m_tabBar->tabButton(index, QTabBar::LeftSide), SIGNAL(destroyed()), window, SLOT(deleteLater()));

	emit windowAdded(index);
}

void WindowsManager::addWindow(ContentsWidget *widget, bool detached)
{
	if (widget)
	{
		addWindow(new Window(widget->isPrivate(), widget, m_area));

		if (detached)
		{
			closeOther();
		}
	}
}

void WindowsManager::cloneWindow(int index)
{
	Window *window = getWindow(index);

	if (window && window->canClone())
	{
		addWindow(window->clone(m_area));
	}
}

void WindowsManager::detachWindow(int index)
{
	Window *window = getWindow(index);

	if (!window)
	{
		return;
	}

	MainWindow *mainWindow = Application::getInstance()->createWindow(m_privateSession, false);

	if (mainWindow)
	{
		window->getContentsWidget()->setParent(NULL);

		mainWindow->getWindowsManager()->addWindow(window->getContentsWidget(), true);

		m_tabBar->removeTab(index);

		emit windowRemoved(index);
	}
}

void WindowsManager::pinWindow(int index, bool pin)
{
	int offset = 0;

	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		if (!m_tabBar->getTabProperty(i, QLatin1String("isPinned"), false).toBool())
		{
			break;
		}

		++offset;
	}

	if (!pin)
	{
		m_tabBar->setTabProperty(index, QLatin1String("isPinned"), false);
		m_tabBar->setTabText(index, m_tabBar->getTabProperty(index, QLatin1String("title"), tr("(Untitled)")).toString());
		m_tabBar->moveTab(index, offset);
		m_tabBar->updateTabs();

		return;
	}

	m_tabBar->setTabProperty(index, QLatin1String("isPinned"), true);
	m_tabBar->setTabText(index, QString());
	m_tabBar->moveTab(index, offset);
	m_tabBar->updateTabs();
}

void WindowsManager::closeWindow(int index)
{
	if (index < 0 || index >= m_tabBar->count() || m_tabBar->getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		return;
	}

	Window *window = getWindow(index);

	if (window)
	{
		window->close();
	}
}

void WindowsManager::closeWindow(Window *window)
{
	const int index = getWindowIndex(window);

	if (index < 0)
	{
		return;
	}

	if (window && !window->isPrivate())
	{
		const WindowHistoryInformation history = window->getContentsWidget()->getHistory();

		if (!history.entries.isEmpty())
		{
			SessionWindow information;
			information.searchEngine = window->getSearchEngine();
			information.history = history.entries;
			information.group = 0;
			information.index = history.index;
			information.pinned = window->isPinned();

			if (window->getType() != QLatin1String("web"))
			{
				removeStoredUrl(information.url());
			}

			m_closedWindows.prepend(information);

			emit closedWindowsAvailableChanged(true);
		}
	}

	if (m_tabBar->count() == 1)
	{
		window = getWindow(0);

		if (window && window->getType() == QLatin1String("web"))
		{
			window->setUrl(QUrl(QLatin1String("about:blank")));
			window->getContentsWidget()->setHistory(WindowHistoryInformation());

			return;
		}
	}

	if (index < m_currentWindow)
	{
		--m_currentWindow;
	}

	m_tabBar->removeTab(index);

	emit windowRemoved(index);

	if (m_tabBar->count() < 1)
	{
		open();
	}
}

void WindowsManager::removeStoredUrl(const QString &url)
{
	for (int i = (m_closedWindows.count() - 1); i >= 0; --i)
	{
		if (url == m_closedWindows.at(i).url())
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

void WindowsManager::setDefaultTextEncoding(const QString &encoding)
{
	Window *window = getWindow(getCurrentWindow());

	if (window)
	{
		window->setDefaultTextEncoding(encoding);
	}
}

void WindowsManager::setZoom(int zoom)
{
	Window *window = getWindow(getCurrentWindow());

	if (window)
	{
		window->getContentsWidget()->setZoom(zoom);
	}
}

void WindowsManager::setCurrentWindow(int index)
{
	if (index < 0 || index >= m_tabBar->count())
	{
		index = 0;
	}

	if (index != m_tabBar->currentIndex())
	{
		m_tabBar->setCurrentIndex(index);

		return;
	}

	Window *window = getWindow(m_currentWindow);

	if (window)
	{
		if (window->parentWidget())
		{
			window->parentWidget()->showMinimized();
			window->parentWidget()->hide();
		}

		if (window->getContentsWidget()->getUndoStack())
		{
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		}

		disconnect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		disconnect(window, SIGNAL(statusMessageChanged(QString,int)), m_statusBar, SLOT(showMessage(QString,int)));
		disconnect(window, SIGNAL(zoomChanged(int)), m_statusBar, SLOT(setZoom(int)));
		disconnect(window, SIGNAL(canZoomChanged(bool)), m_statusBar, SLOT(setZoomEnabled(bool)));
		disconnect(m_statusBar, SIGNAL(requestedZoomChange(int)), window->getContentsWidget(), SLOT(setZoom(int)));
	}

	m_statusBar->clearMessage();

	m_currentWindow = index;

	window = getWindow(index);

	if (window)
	{
		if (window->parentWidget())
		{
			window->parentWidget()->showMaximized();

			m_area->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(window->parentWidget()));
		}

		m_statusBar->setZoom(window->getContentsWidget()->getZoom());
		m_statusBar->setZoomEnabled(window->getContentsWidget()->canZoom());

		emit windowTitleChanged(QString("%1 - Otter").arg(window->getContentsWidget()->getTitle()));

		if (window->getContentsWidget()->getUndoStack())
		{
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		}

		connect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		connect(window, SIGNAL(statusMessageChanged(QString,int)), m_statusBar, SLOT(showMessage(QString,int)));
		connect(window, SIGNAL(zoomChanged(int)), m_statusBar, SLOT(setZoom(int)));
		connect(window, SIGNAL(canZoomChanged(bool)), m_statusBar, SLOT(setZoomEnabled(bool)));
		connect(m_statusBar, SIGNAL(requestedZoomChange(int)), window->getContentsWidget(), SLOT(setZoom(int)));
	}

	emit actionsChanged();
	emit currentWindowChanged(index);
}

void WindowsManager::setTitle(const QString &title)
{
	const QString text = (title.isEmpty() ? tr("Empty") : title);
	const int index = getWindowIndex(qobject_cast<Window*>(sender()));

	if (!m_tabBar->getTabProperty(index, QLatin1String("isPinned"), false).toBool())
	{
		m_tabBar->setTabText(index, text);
	}

	if (index == getCurrentWindow())
	{
		emit windowTitleChanged(QString("%1 - Otter").arg(text));
	}
}

QAction *WindowsManager::getAction(WindowAction action)
{
	Window *window = getWindow();

	if (window)
	{
		return window->getContentsWidget()->getAction(action);
	}

	return NULL;
}

Window* WindowsManager::getWindow(int index) const
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	if (index < 0 || index >= m_tabBar->count())
	{
		return NULL;
	}

	return qvariant_cast<Window*>(m_tabBar->tabData(index));
}

QString WindowsManager::getDefaultTextEncoding() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getDefaultTextEncoding() : QString());
}

QString WindowsManager::getTitle() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getContentsWidget()->getTitle() : tr("Empty"));
}

QUrl WindowsManager::getUrl() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getContentsWidget()->getUrl() : QUrl());
}

SessionEntry WindowsManager::getSession() const
{
	SessionEntry session;
	session.index = getCurrentWindow();

	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		Window *window = getWindow(i);

		if (window && !window->isPrivate())
		{
			const WindowHistoryInformation history = window->getContentsWidget()->getHistory();
			SessionWindow information;
			information.searchEngine = window->getSearchEngine();
			information.history = history.entries;
			information.group = 0;
			information.index = history.index;
			information.pinned = window->isPinned();

			session.windows.append(information);
		}
		else if (i < session.index)
		{
			--session.index;
		}
	}

	return session;
}

QList<SessionWindow> WindowsManager::getClosedWindows() const
{
	return m_closedWindows;
}

int WindowsManager::getWindowIndex(Window *window) const
{
	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		if (window == qvariant_cast<Window*>(m_tabBar->tabData(i)))
		{
			return i;
		}
	}

	return -1;
}

int WindowsManager::getWindowCount() const
{
	return m_tabBar->count();
}

int WindowsManager::getCurrentWindow() const
{
	return m_tabBar->currentIndex();
}

int WindowsManager::getZoom() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getContentsWidget()->getZoom() : 100);
}

bool WindowsManager::canZoom() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getContentsWidget()->canZoom() : false);
}

bool WindowsManager::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		Window *window = getWindow(i);

		if (window && window->getContentsWidget()->getUrl() == url)
		{
			if (activate)
			{
				setCurrentWindow(i);
			}

			return true;
		}
	}

	return false;
}

}
