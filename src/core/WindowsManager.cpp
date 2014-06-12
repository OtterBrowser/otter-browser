/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "SettingsManager.h"
#include "../ui/ContentsWidget.h"
#include "../ui/MainWindow.h"
#include "../ui/MdiWidget.h"
#include "../ui/StatusBarWidget.h"
#include "../ui/TabBarWidget.h"

#include <QtGui/QPainter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>

namespace Otter
{

WindowsManager::WindowsManager(MdiWidget *mdi, TabBarWidget *tabBar, StatusBarWidget *statusBar, bool isPrivate) : QObject(mdi),
	m_mdi(mdi),
	m_tabBar(tabBar),
	m_statusBar(statusBar),
	m_printedWindow(-1),
	m_isPrivate(isPrivate),
	m_isRestored(false)
{
}

void WindowsManager::open(const QUrl &url, OpenHints hints)
{
	Window *window = m_mdi->getActiveWindow();

	if (m_isPrivate)
	{
		hints |= PrivateOpen;
	}

	if (hints & NewWindowOpen)
	{
		emit requestedNewWindow((hints & PrivateOpen), (hints & BackgroundOpen), url);
	}
	else if (url.isEmpty() || (hints & NewTabOpen))
	{
		openTab(url, hints);
	}
	else if (window && ((hints & CurrentTabOpen) || (window->getType() == QLatin1String("web") && ((window->getUrl().scheme() == QLatin1String("about") && window->isUrlEmpty()) || url.scheme() == QLatin1String("javascript")))))
	{
		if (window->isPrivate() == (hints & PrivateOpen))
		{
			window->getContentsWidget()->setHistory(WindowHistoryInformation());
			window->setUrl(url, false);
		}
		else
		{
			closeWindow(m_tabBar->currentIndex());
			openTab(url, hints);
		}
	}
	else
	{
		openTab(url, hints);
	}
}

void WindowsManager::openTab(QUrl url, OpenHints hints)
{
	Window *window = new Window((hints & PrivateOpen), NULL, m_mdi);

	addWindow(window, hints);

	window->setUrl((url.isEmpty() ? QUrl(SettingsManager::getValue(QLatin1String("Browser/StartPage")).toString()) : url), false);
}

void WindowsManager::open(const BookmarkInformation *bookmark, OpenHints hints)
{
	if (hints == DefaultOpen && SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool())
	{
		hints = CurrentTabOpen;
	}

	switch (bookmark->type)
	{
		case UrlBookmark:
			open(QUrl(bookmark->url), hints);

			break;
		case FolderBookmark:
			{
				gatherBookmarks(bookmark->identifier);

				if (m_bookmarksToOpen.count() > 1 && SettingsManager::getValue(QLatin1String("Choices/WarnOpenBookmarkFolder")).toBool())
				{
					QMessageBox messageBox;
					messageBox.setWindowTitle(tr("Question"));
					messageBox.setText(tr("You are about to open %n bookmarks.", "", m_bookmarksToOpen.count()));
					messageBox.setInformativeText("Do you want to continue?");
					messageBox.setIcon(QMessageBox::Question);
					messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
					messageBox.setDefaultButton(QMessageBox::Yes);
					messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

					if (messageBox.exec() == QMessageBox::Cancel)
					{
						m_bookmarksToOpen.clear();
					}

					SettingsManager::setValue(QLatin1String("Choices/WarnOpenBookmarkFolder"), !messageBox.checkBox()->isChecked());
				}

				if (m_bookmarksToOpen.isEmpty())
				{
					return;
				}

				open(QUrl(m_bookmarksToOpen.at(0)), hints);

				for (int i = 1; i < m_bookmarksToOpen.count(); ++i)
				{
					open(QUrl(m_bookmarksToOpen.at(i)), ((hints == DefaultOpen || hints == CurrentTabOpen) ? NewTabOpen : hints));
				}

				m_bookmarksToOpen.clear();
			}

			break;
		default:
			break;
	}
}

void WindowsManager::gatherBookmarks(int folder)
{
	const QList<BookmarkInformation*> bookmarks = BookmarksManager::getFolder(folder);

	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			gatherBookmarks(bookmarks.at(i)->identifier);
		}
		else if (bookmarks.at(i)->type == UrlBookmark)
		{
			m_bookmarksToOpen.append(bookmarks.at(i)->url);
		}
	}
}

void WindowsManager::search(const QString &query, const QString &engine)
{
	Window *window = m_mdi->getActiveWindow();

	if (window && SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool())
	{
		window->search(query, engine);

		return;
	}

	if (window && window->canClone())
	{
		window = window->clone(false, m_mdi);

		addWindow(window);
	}
	else
	{
		open();

		window = m_mdi->getActiveWindow();
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
		index = m_tabBar->currentIndex();
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
		index = m_tabBar->currentIndex();
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

void WindowsManager::restore(const SessionMainWindow &session)
{
	if (session.windows.isEmpty())
	{
		open();
	}
	else
	{
		for (int i = 0; i < session.windows.count(); ++i)
		{
			Window *window = new Window(m_isPrivate, NULL, m_mdi);
			window->setSession(session.windows.at(i));

			addWindow(window);
		}
	}

	m_isRestored = true;

	connect(SessionsManager::getInstance(), SIGNAL(requestedRemoveStoredUrl(QString)), this, SLOT(removeStoredUrl(QString)));
	connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setActiveWindow(int)));
	connect(m_tabBar, SIGNAL(requestedClone(int)), this, SLOT(cloneWindow(int)));
	connect(m_tabBar, SIGNAL(requestedDetach(int)), this, SLOT(detachWindow(int)));
	connect(m_tabBar, SIGNAL(requestedPin(int,bool)), this, SLOT(pinWindow(int,bool)));
	connect(m_tabBar, SIGNAL(requestedClose(int)), this, SLOT(closeWindow(int)));
	connect(m_tabBar, SIGNAL(requestedCloseOther(int)), this, SLOT(closeOther(int)));

	setActiveWindow(session.index);
}

void WindowsManager::restore(int index)
{
	if (index < 0 || index >= m_closedWindows.count())
	{
		return;
	}

	Window *window = new Window(m_isPrivate, NULL, m_mdi);
	window->setSession(m_closedWindows.at(index));

	m_closedWindows.removeAt(index);

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
	QPrintDialog printDialog(&printer, m_mdi);
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
		index = m_tabBar->currentIndex();
	}

	if (index < 0 || index >= m_tabBar->count())
	{
		return;
	}

	m_printedWindow = index;

	QPrintPreviewDialog printPreviewtDialog(m_mdi);
	printPreviewtDialog.setWindowTitle(tr("Print Preview"));

	connect(&printPreviewtDialog, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printPreview(QPrinter*)));

	printPreviewtDialog.exec();

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
	Window *window = m_mdi->getActiveWindow();

	if (window)
	{
		window->triggerAction(action, checked);
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

void WindowsManager::addWindow(Window *window, OpenHints hints)
{
	if (!window)
	{
		return;
	}

	int index = ((!(hints & EndOpen) && SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool()) ? (m_tabBar->currentIndex() + 1) : m_tabBar->count());

	if (!window->isPinned())
	{
		const int offset = m_tabBar->getPinnedTabsAmount();

		if (index < offset)
		{
			index = offset;
		}
	}

	m_tabBar->addTab(index, window);

	m_mdi->addWindow(window);

	if (!(hints & BackgroundOpen))
	{
		m_tabBar->setCurrentIndex(index);

		if (m_isRestored)
		{
			setActiveWindow(index);
		}
	}

	connect(window, SIGNAL(requestedCloseWindow(Window*)), this, SLOT(closeWindow(Window*)));
	connect(window, SIGNAL(requestedAddBookmark(QUrl,QString)), this, SIGNAL(requestedAddBookmark(QUrl,QString)));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), this, SLOT(open(QUrl,OpenHints)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)), this, SLOT(openWindow(ContentsWidget*,OpenHints)));
	connect(window, SIGNAL(requestedSearch(QString,QString)), this, SLOT(search(QString,QString)));
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));

	emit windowAdded(index);
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
		addWindow(new Window(widget->isPrivate(), widget, m_mdi), hints);
	}
}

void WindowsManager::cloneWindow(int index)
{
	Window *window = getWindow(index);

	if (window && window->canClone())
	{
		addWindow(window->clone(true, m_mdi));
	}
}

void WindowsManager::detachWindow(int index)
{
	Window *window = getWindow(index);

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

		m_tabBar->removeTab(index);

		emit windowRemoved(index);
	}
}

void WindowsManager::pinWindow(int index, bool pin)
{
	const int offset = m_tabBar->getPinnedTabsAmount();

	if (!pin)
	{
		m_tabBar->setTabProperty(index, QLatin1String("isPinned"), false);
		m_tabBar->setTabText(index, m_tabBar->getTabProperty(index, QLatin1String("title"), tr("(Untitled)")).toString());
		m_tabBar->moveTab(index, offset);

		return;
	}

	m_tabBar->setTabProperty(index, QLatin1String("isPinned"), true);
	m_tabBar->setTabText(index, QString());
	m_tabBar->moveTab(index, offset);
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

		if (!window->isUrlEmpty() || history.entries.count() > 1)
		{
			const SessionWindow information = window->getSession();

			if (window->getType() != QLatin1String("web"))
			{
				removeStoredUrl(information.getUrl());
			}

			m_closedWindows.prepend(information);

			emit closedWindowsAvailableChanged(true);
		}
	}

	if (m_tabBar->count() == 1)
	{
		window = getWindow(0);

		if (window)
		{
			window->clear();

			return;
		}
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
		if (url == m_closedWindows.at(i).getUrl())
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
	Window *window = m_mdi->getActiveWindow();

	if (window)
	{
		window->setDefaultTextEncoding(encoding);
	}
}

void WindowsManager::setUserAgent(const QString &identifier)
{
	Window *window = m_mdi->getActiveWindow();

	if (window)
	{
		window->setUserAgent(identifier);
	}
}

void WindowsManager::setZoom(int zoom)
{
	Window *window = m_mdi->getActiveWindow();

	if (window)
	{
		window->getContentsWidget()->setZoom(zoom);
	}
}

void WindowsManager::setActiveWindow(int index)
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

	Window *window = m_mdi->getActiveWindow();

	if (window)
	{
		if (window->getContentsWidget()->getUndoStack())
		{
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
			disconnect(window->getContentsWidget()->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		}

		disconnect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		disconnect(window, SIGNAL(statusMessageChanged(QString)), m_statusBar, SLOT(showMessage(QString)));
		disconnect(window, SIGNAL(zoomChanged(int)), m_statusBar, SLOT(setZoom(int)));
		disconnect(window, SIGNAL(canZoomChanged(bool)), m_statusBar, SLOT(setZoomEnabled(bool)));
		disconnect(m_statusBar, SIGNAL(requestedZoomChange(int)), window->getContentsWidget(), SLOT(setZoom(int)));
	}

	m_statusBar->clearMessage();

	window = getWindow(index);

	if (window)
	{
		m_mdi->setActiveWindow(window);

		window->setFocus();

		m_statusBar->showMessage(window->getContentsWidget()->getStatusMessage());
		m_statusBar->setZoom(window->getContentsWidget()->getZoom());
		m_statusBar->setZoomEnabled(window->getContentsWidget()->canZoom());

		emit windowTitleChanged(window->getContentsWidget()->getTitle());

		if (window->getContentsWidget()->getUndoStack())
		{
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
			connect(window->getContentsWidget()->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		}

		connect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		connect(window, SIGNAL(statusMessageChanged(QString)), m_statusBar, SLOT(showMessage(QString)));
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

	if (index == m_tabBar->currentIndex())
	{
		emit windowTitleChanged(text);
	}
}

QAction* WindowsManager::getAction(WindowAction action)
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getContentsWidget()->getAction(action) : NULL);
}

Window* WindowsManager::getWindow(int index) const
{
	if (index < 0)
	{
		return m_mdi->getActiveWindow();
	}

	return ((index >= m_tabBar->count()) ? NULL : qvariant_cast<Window*>(m_tabBar->tabData(index)));
}

QString WindowsManager::getDefaultTextEncoding() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getDefaultTextEncoding() : QString());
}

QString WindowsManager::getTitle() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getTitle() : tr("Empty"));
}

QUrl WindowsManager::getUrl() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getUrl() : QUrl());
}

SessionMainWindow WindowsManager::getSession() const
{
	SessionMainWindow session;
	session.index = m_tabBar->currentIndex();

	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		Window *window = getWindow(i);

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

QPair<QString, QString> WindowsManager::getUserAgent() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getUserAgent() : qMakePair(QString(), QString()));
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

int WindowsManager::getZoom() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getContentsWidget()->getZoom() : 100);
}

bool WindowsManager::canZoom() const
{
	Window *window = m_mdi->getActiveWindow();

	return (window ? window->getContentsWidget()->canZoom() : false);
}

bool WindowsManager::isPrivate() const
{
	return m_isPrivate;
}

bool WindowsManager::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		Window *window = getWindow(i);

		if (window && window->getUrl() == url)
		{
			if (activate)
			{
				setActiveWindow(i);
			}

			return true;
		}
	}

	return false;
}

}
