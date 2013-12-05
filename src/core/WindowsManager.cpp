#include "WindowsManager.h"
#include "SettingsManager.h"
#include "../ui/ContentsWidget.h"
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
	connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentWindow(int)));
	connect(m_tabBar, SIGNAL(requestedClone(int)), this, SLOT(cloneWindow(int)));
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

		if (window && window->getType() == "web" && window->getUrl().scheme() == "about" && (window->getUrl().path() == "blank" || window->getUrl().path().isEmpty()))
		{
			if (window->isPrivate() == privateWindow)
			{
				window->setHistory(HistoryInformation());
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

void WindowsManager::close(int index)
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	closeWindow(index);
}

void WindowsManager::closeOther(int index)
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	if (index >= m_tabBar->count())
	{
		return;
	}

	for (int i = (m_tabBar->count() - 1); i > index; --i)
	{
		closeWindow(i);
	}

	for (int i = 0; i < index; ++i)
	{
		closeWindow(0);
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

	SessionWindow entry = m_closedWindows.at(index);
	HistoryInformation history;
	history.index = entry.index;
	history.entries = entry.history;

	Window *window = new Window(m_privateSession, NULL, m_area);
	window->setUrl(entry.url());
	window->setHistory(history);
	window->setPinned(entry.pinned);
	window->setZoom(entry.zoom());

	m_closedWindows.removeAt(index);

	if (SessionsManager::getClosedWindows().isEmpty())
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

	window->print(&printer);
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
		window->print(printer);
	}
}

void WindowsManager::triggerAction(WindowAction action, bool checked)
{
	Window *window = getWindow(getCurrentWindow());

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

void WindowsManager::addWindow(Window *window, bool background)
{
	if (!window)
	{
		return;
	}

	QMdiSubWindow *mdiWindow = m_area->addSubWindow(window, Qt::CustomizeWindowHint);
	mdiWindow->showMaximized();

	const int index = m_tabBar->count();

	m_tabBar->insertTab(index, window->getTitle());
	m_tabBar->setTabData(index, QVariant::fromValue(window));

	if (background)
	{
		setCurrentWindow(m_tabBar->currentIndex());
	}
	else
	{
		m_tabBar->setCurrentIndex(index);

		setCurrentWindow(index);
	}

	connect(window, SIGNAL(requestedAddBookmark(QUrl)), this, SIGNAL(requestedAddBookmark(QUrl)));
	connect(window, SIGNAL(requestedOpenUrl(QUrl,bool,bool,bool)), this, SLOT(open(QUrl,bool,bool,bool)));
	connect(window, SIGNAL(requestedNewWindow(ContentsWidget*)), this, SLOT(addWindow(ContentsWidget*)));
	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(iconChanged(QIcon)), m_tabBar, SLOT(updateTabs()));
	connect(window, SIGNAL(loadingChanged(bool)), m_tabBar, SLOT(updateTabs()));
	connect(m_tabBar->tabButton(index, QTabBar::LeftSide), SIGNAL(destroyed()), window, SLOT(deleteLater()));

	emit windowAdded(index);
}

void WindowsManager::addWindow(ContentsWidget *widget)
{
	if (widget)
	{
		addWindow(new Window(widget->isPrivate(), widget, m_area));
	}
}

void WindowsManager::cloneWindow(int index)
{
	Window *window = getWindow(index);

	if (window && window->isClonable())
	{
		addWindow(window->clone(m_area));
	}
}

void WindowsManager::pinWindow(int index, bool pin)
{
	int offset = 0;

	for (int i = 0; i < m_tabBar->count(); ++i)
	{
		if (!m_tabBar->getTabProperty(i, "isPinned", false).toBool())
		{
			break;
		}

		++offset;
	}

	if (!pin)
	{
		m_tabBar->setTabProperty(index, "isPinned", false);
		m_tabBar->setTabText(index, m_tabBar->getTabProperty(index, "title", tr("(Untitled)")).toString());
		m_tabBar->moveTab(index, offset);
		m_tabBar->updateTabs();

		return;
	}

	m_tabBar->setTabProperty(index, "isPinned", true);
	m_tabBar->setTabText(index, QString());
	m_tabBar->moveTab(index, offset);
	m_tabBar->updateTabs();
}

void WindowsManager::closeWindow(int index)
{
	if (index < 0 || index >= m_tabBar->count() || m_tabBar->getTabProperty(index, "isPinned", false).toBool())
	{
		return;
	}

	Window *window = getWindow(index);

	if (window && !window->isPrivate())
	{
		const HistoryInformation history = window->getHistory();
		SessionWindow information;
		information.history = history.entries;
		information.group = 0;
		information.index = history.index;
		information.pinned = window->isPinned();

		m_closedWindows.prepend(information);

		emit closedWindowsAvailableChanged(true);
	}

	if (m_tabBar->count() == 1)
	{
		window = getWindow(0);

		if (window)
		{
			window->setUrl(QUrl("about:blank"));

			return;
		}
	}

	if (index < m_currentWindow)
	{
		--m_currentWindow;
	}

	m_tabBar->removeTab(index);

	emit windowRemoved(index);
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
		window->setZoom(zoom);
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
		window->showMinimized();

		disconnect(window->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
		disconnect(window->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
		disconnect(window->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
		disconnect(window->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		disconnect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		disconnect(window, SIGNAL(statusMessageChanged(QString,int)), m_statusBar, SLOT(showMessage(QString,int)));
		disconnect(window, SIGNAL(zoomChanged(int)), m_statusBar, SLOT(setZoom(int)));
		disconnect(m_statusBar, SIGNAL(requestedZoomChange(int)), window, SLOT(setZoom(int)));
	}

	m_statusBar->clearMessage();

	m_currentWindow = index;

	window = getWindow(index);

	if (window)
	{
		QList<QMdiSubWindow*> windows = m_area->subWindowList();

		for (int i = 0; i < windows.count(); ++i)
		{
			if (window == windows.at(i)->widget())
			{
				m_area->setActiveSubWindow(windows.at(i));

				window->showMaximized();

				break;
			}
		}

		m_statusBar->setZoom(window->getZoom());

		emit windowTitleChanged(QString("%1 - Otter").arg(window->getTitle()));

		connect(window->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(actionsChanged()));
		connect(window->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(actionsChanged()));
		connect(window->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(actionsChanged()));
		connect(window->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(actionsChanged()));
		connect(window, SIGNAL(actionsChanged()), this, SIGNAL(actionsChanged()));
		connect(window, SIGNAL(statusMessageChanged(QString,int)), m_statusBar, SLOT(showMessage(QString,int)));
		connect(window, SIGNAL(zoomChanged(int)), m_statusBar, SLOT(setZoom(int)));
		connect(m_statusBar, SIGNAL(requestedZoomChange(int)), window, SLOT(setZoom(int)));
	}

	emit currentWindowChanged(index);
}

void WindowsManager::setTitle(const QString &title)
{
	const QString text = (title.isEmpty() ? tr("Empty") : title);
	const int index = getWindowIndex(qobject_cast<Window*>(sender()));

	if (!m_tabBar->getTabProperty(index, "isPinned", false).toBool())
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
		return window->getAction(action);
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

	return (window ? window->getTitle() : tr("Empty"));
}

QUrl WindowsManager::getUrl() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window ? window->getUrl() : QUrl());
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
			const HistoryInformation history = window->getHistory();
			SessionWindow information;
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

	return (window ? window->getZoom() : 100);
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

bool WindowsManager::canUndo() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window && window->getUndoStack()->canUndo());
}

bool WindowsManager::canRedo() const
{
	Window *window = getWindow(getCurrentWindow());

	return (window && window->getUndoStack()->canRedo());
}

}
