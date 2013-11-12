#include "WindowsManager.h"
#include "SettingsManager.h"
#include "../ui/TabBarWidget.h"
#include "../ui/Window.h"

#include <QtGui/QPainter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QMdiSubWindow>


#include <QDebug>

namespace Otter
{

WindowsManager::WindowsManager(QMdiArea *area, TabBarWidget *tabBar) : QObject(area),
	m_area(area),
	m_tabBar(tabBar),
	m_currentWindow(-1),
	m_printedWindow(-1)
{
	open();
	setCurrentWindow(0);

	connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentWindow(int)));
	connect(m_tabBar, SIGNAL(requestedOpen()), this, SLOT(open()));
	connect(m_tabBar, SIGNAL(requestedClose(int)), this, SLOT(closeWindow(int)));
	connect(m_tabBar, SIGNAL(requestedCloseOther(int)), this, SLOT(closeOther(int)));
}

void WindowsManager::open(const QUrl &url)
{
	Window *window = new Window(m_area);
	QMdiSubWindow *mdiWindow = m_area->addSubWindow(window, Qt::CustomizeWindowHint);
	mdiWindow->showMaximized();

	m_windows.append(window);

	const int index = (m_windows.count() - 1);

	m_tabBar->addTab(window->getIcon(), window->getTitle());
	m_tabBar->setCurrentIndex(index);

	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));

	window->setUrl(url);

	emit windowAdded(index);
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

	if (index >= m_windows.count())
	{
		return;
	}

	for (int i = (m_windows.count() - 1); i > index; --i)
	{
		closeWindow(i);
	}

	for (int i = 0; i < index; ++i)
	{
		closeWindow(0);
	}
}

void WindowsManager::print(int index)
{
	Window *window = getWindow(m_printedWindow);

	if (!window)
	{
		return;
	}

	QPrinter printer;
	QPrintDialog printDialog(&printer, m_area);
	printDialog.setWindowTitle(tr("Print Project"));

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

	if (index < 0 || index >= m_windows.count())
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

void WindowsManager::closeWindow(int index)
{
	if (index < 0 || index >= m_windows.count())
	{
		return;
	}

	if (m_windows.count() == 1)
	{
		m_windows.at(0)->setUrl(QUrl("about:blank"));

		return;
	}

	if (index < m_currentWindow)
	{
		--m_currentWindow;
	}

	m_windows.at(index)->deleteLater();
	m_windows.removeAt(index);

	m_tabBar->removeTab(index);

	emit windowRemoved(index);
}

void WindowsManager::undo()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->undo();
	}
}

void WindowsManager::redo()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->redo();
	}
}

void WindowsManager::cut()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->cut();
	}
}

void WindowsManager::copy()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->copy();
	}
}

void WindowsManager::paste()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->paste();
	}
}

void WindowsManager::remove()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->remove();
	}
}

void WindowsManager::selectAll()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->selectAll();
	}
}

void WindowsManager::zoomIn()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->zoomIn();
	}
}

void WindowsManager::zoomOut()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->zoomOut();
	}
}

void WindowsManager::zoomOriginal()
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->zoomOriginal();
	}
}

void WindowsManager::setZoom(int zoom)
{
	Window *window = m_windows.at(getCurrentWindow());

	if (window)
	{
		window->setZoom(zoom);
	}
}

void WindowsManager::setCurrentWindow(int index)
{
	if (index < 0 || index >= m_windows.count())
	{
		index = 0;
	}

	Window *window = ((m_currentWindow >= 0 && m_currentWindow < m_windows.count()) ? m_windows.at(m_currentWindow) : NULL);

	if (window)
	{
		disconnect(window->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(undoTextChanged(QString)));
		disconnect(window->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(redoTextChanged(QString)));
		disconnect(window->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
		disconnect(window->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
	}

	m_currentWindow = index;

	window = m_windows.at(m_currentWindow);

	if (window)
	{
		QList<QMdiSubWindow*> windows = m_area->subWindowList();

		for (int i = 0; i < windows.count(); ++i)
		{
			if (window == windows.at(i)->widget())
			{
				m_area->setActiveSubWindow(windows.at(i));

				break;
			}
		}

		emit windowTitleChanged(QString("%1 - Otter").arg(window->getTitle()));
		emit undoTextChanged(window->getUndoStack()->undoText());
		emit redoTextChanged(window->getUndoStack()->redoText());
		emit canUndoChanged(window->getUndoStack()->canUndo());
		emit canRedoChanged(window->getUndoStack()->canRedo());

		connect(window->getUndoStack(), SIGNAL(undoTextChanged(QString)), this, SIGNAL(undoTextChanged(QString)));
		connect(window->getUndoStack(), SIGNAL(redoTextChanged(QString)), this, SIGNAL(redoTextChanged(QString)));
		connect(window->getUndoStack(), SIGNAL(canUndoChanged(bool)), this, SIGNAL(canUndoChanged(bool)));
		connect(window->getUndoStack(), SIGNAL(canRedoChanged(bool)), this, SIGNAL(canRedoChanged(bool)));
	}

	emit currentWindowChanged(index);
}

void WindowsManager::setTitle(const QString &title)
{
	const QString text = (title.isEmpty() ? tr("Empty") : title);
	const int index = m_windows.indexOf(qobject_cast<Window*>(sender()));

	m_tabBar->setTabText(index, text);

	if (index == getCurrentWindow())
	{
		emit windowTitleChanged(QString("%1 - Otter").arg(text));
	}
}

void WindowsManager::setIcon(const QIcon &icon)
{
	m_tabBar->setTabIcon(m_windows.indexOf(qobject_cast<Window*>(sender())), (icon.isNull() ? QIcon(":/icons/tab.png") : icon));
}

Window* WindowsManager::getWindow(int index) const
{
	if (index < 0)
	{
		index = getCurrentWindow();
	}

	if (index < 0 || index >= m_windows.count())
	{
		return NULL;
	}

	return m_windows.at(index);
}

QString WindowsManager::getTitle() const
{
	Window *window = m_windows.at(getCurrentWindow());

	return QString("%1 - Otter").arg(window ? window->getTitle() : tr("Empty"));
}

int WindowsManager::getCurrentWindow() const
{
	return m_tabBar->currentIndex();
}

int WindowsManager::getZoom() const
{
	Window *window = m_windows.at(getCurrentWindow());

	return (window ? window->getZoom() : 100);
}

bool WindowsManager::canUndo() const
{
	Window *window = m_windows.at(getCurrentWindow());

	return (window && window->getUndoStack()->canUndo());
}

bool WindowsManager::canRedo() const
{
	Window *window = m_windows.at(getCurrentWindow());

	return (window && window->getUndoStack()->canRedo());
}

}
