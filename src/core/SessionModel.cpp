/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SessionModel.h"
#include "ThemesManager.h"
#include "../ui/MainWindow.h"
#include "../ui/Window.h"

namespace Otter
{

SessionItem::SessionItem() : QStandardItem()
{
}

Window* SessionItem::getActiveWindow() const
{
	return nullptr;
}

QVariant SessionItem::data(int role) const
{
	if (role == Qt::DecorationRole)
	{
		const SessionModel::EntityType type(static_cast<SessionModel::EntityType>(data(SessionModel::TypeRole).toInt()));

		if (type == SessionModel::SessionEntity)
		{
			return ThemesManager::getIcon(QLatin1String("inode-directory"));
		}

		if (type == SessionModel::TrashEntity)
		{
			return ThemesManager::getIcon(QLatin1String("user-trash"));
		}
	}
	else if (role == SessionModel::IsTrashedRole)
	{
		QModelIndex parent(index().parent());

		while (parent.isValid())
		{
			const SessionModel::EntityType type(static_cast<SessionModel::EntityType>(parent.data(SessionModel::TypeRole).toInt()));

			if (type == SessionModel::TrashEntity)
			{
				return true;
			}

			if (type == SessionModel::SessionEntity)
			{
				break;
			}

			parent = parent.parent();
		}

		return false;
	}

	return QStandardItem::data(role);
}

MainWindowSessionItem::MainWindowSessionItem(MainWindow *mainWindow) : SessionItem(),
	m_mainWindow(mainWindow)
{
	const QVector<quint64> windows(mainWindow->getWindowsManager()->getWindows());

	for (int i = 0; i < windows.count(); ++i)
	{
		handleWindowAdded(windows.at(i));
	}

	connect(mainWindow->getWindowsManager(), SIGNAL(titleChanged(QString)), this, SLOT(notifyMainWindowModified()));
	connect(mainWindow->getWindowsManager(), SIGNAL(currentWindowChanged(quint64)), this, SLOT(notifyMainWindowModified()));
	connect(mainWindow->getWindowsManager(), SIGNAL(windowAdded(quint64)), this, SLOT(handleWindowAdded(quint64)));
	connect(mainWindow->getWindowsManager(), SIGNAL(windowRemoved(quint64)), this, SLOT(handleWindowRemoved(quint64)));
}

void MainWindowSessionItem::handleWindowAdded(quint64 identifier)
{
	Window *window(m_mainWindow->getWindowsManager()->getWindowByIdentifier(identifier));

	for (int i = 0; i < rowCount(); ++i)
	{
		WindowSessionItem *item(dynamic_cast<WindowSessionItem*>(child(i)));

		if (item && item->getActiveWindow() == window)
		{
			return;
		}
	}

	insertRow(m_mainWindow->getWindowsManager()->getWindowIndex(identifier), new WindowSessionItem(window));
}

void MainWindowSessionItem::handleWindowRemoved(quint64 identifier)
{
	Window *window(m_mainWindow->getWindowsManager()->getWindowByIdentifier(identifier));

	for (int i = 0; i < rowCount(); ++i)
	{
		WindowSessionItem *item(dynamic_cast<WindowSessionItem*>(child(i)));

		if (item && item->getActiveWindow() == window)
		{
			removeRow(i);

			break;
		}
	}
}

void MainWindowSessionItem::notifyMainWindowModified()
{
	emitDataChanged();
}

Window* MainWindowSessionItem::getActiveWindow() const
{
	return m_mainWindow->getWindowsManager()->getWindowByIndex(-1);
}

MainWindow* MainWindowSessionItem::getMainWindow() const
{
	return m_mainWindow;
}

QVariant MainWindowSessionItem::data(int role) const
{
	switch (role)
	{
		case SessionModel::TitleRole:
			return m_mainWindow->getWindowsManager()->getTitle();
		case SessionModel::UrlRole:
			return m_mainWindow->getWindowsManager()->getUrl();
		case SessionModel::IconRole:
			return ThemesManager::getIcon(QLatin1String("window"));
		case SessionModel::TypeRole:
			return SessionModel::MainWindowEntity;
		case SessionModel::IndexRole:
			return m_mainWindow->getWindowsManager()->getCurrentWindowIndex();
		case SessionModel::IsPrivateRole:
			return m_mainWindow->isPrivate();
		default:
			break;
	}

	return SessionItem::data(role);
}

WindowSessionItem::WindowSessionItem(Window *window) : SessionItem(),
	m_window(window)
{
}

Window* WindowSessionItem::getActiveWindow() const
{
	return m_window;
}

QVariant WindowSessionItem::data(int role) const
{
	switch (role)
	{
		case SessionModel::TitleRole:
			return m_window->getTitle();
		case SessionModel::UrlRole:
			return m_window->getUrl();
		case SessionModel::IconRole:
			return m_window->getIcon();
		case SessionModel::IdentifierRole:
			return m_window->getIdentifier();
		case SessionModel::TypeRole:
			return SessionModel::WindowEntity;
		case SessionModel::IndexRole:
			{
				MainWindow *mainWindow(MainWindow::findMainWindow(m_window));

				return (mainWindow ? mainWindow->getWindowsManager()->getWindowIndex(m_window->getIdentifier()) : -1);
			}
		case SessionModel::LastActivityRole:
			return m_window->getLastActivity();
		case SessionModel::ZoomRole:
			return m_window->getContentsWidget()->getZoom();
		case SessionModel::IsPinnedRole:
			return m_window->isPinned();
		case SessionModel::IsPrivateRole:
			return m_window->isPrivate();
		case SessionModel::IsSuspendedRole:
			return (m_window->getLoadingState() == WebWidget::DelayedLoadingState);
		default:
			break;
	}

	return SessionItem::data(role);
}

SessionModel::SessionModel(const QString &path, QObject *parent) : QStandardItemModel(parent),
	m_rootItem(new SessionItem()),
	m_trashItem(new SessionItem()),
	m_path(path)
{
	m_rootItem->setData(SessionEntity, TypeRole);
	m_rootItem->setData(tr("Session"), TitleRole);
	m_rootItem->setDragEnabled(false);
	m_trashItem->setData(TrashEntity, TypeRole);
	m_trashItem->setData(tr("Trash"), TitleRole);
	m_trashItem->setDragEnabled(false);
	m_trashItem->setEnabled(false);

	appendRow(m_rootItem);
	appendRow(m_trashItem);

	const QVector<MainWindow*> mainWindows(Application::getWindows());

	for (int i = 0; i < mainWindows.count(); ++i)
	{
		handleMainWindowAdded(mainWindows.at(i));
	}

	connect(Application::getInstance(), SIGNAL(windowAdded(MainWindow*)), this, SLOT(handleMainWindowAdded(MainWindow*)));
	connect(Application::getInstance(), SIGNAL(windowRemoved(MainWindow*)), this, SLOT(handleMainWindowRemoved(MainWindow*)));
}

void SessionModel::handleMainWindowAdded(MainWindow *mainWindow)
{
	for (int i = 0; i < m_rootItem->rowCount(); ++i)
	{
		MainWindowSessionItem *item(dynamic_cast<MainWindowSessionItem*>(m_rootItem->child(i)));

		if (item && item->getMainWindow() == mainWindow)
		{
			return;
		}
	}

	MainWindowSessionItem *item(new MainWindowSessionItem(mainWindow));

	m_rootItem->appendRow(item);

	m_mainWindowItems[mainWindow] = item;
}

void SessionModel::handleMainWindowRemoved(MainWindow *mainWindow)
{
	for (int i = 0; i < m_rootItem->rowCount(); ++i)
	{
		MainWindowSessionItem *item(dynamic_cast<MainWindowSessionItem*>(m_rootItem->child(i)));

		if (item && item->getMainWindow() == mainWindow)
		{
			m_rootItem->removeRow(i);

			break;
		}
	}

	m_mainWindowItems.remove(mainWindow);
}

SessionItem* SessionModel::getRootItem() const
{
	return m_rootItem;
}

SessionItem* SessionModel::getTrashItem() const
{
	return m_trashItem;
}

SessionItem* SessionModel::getMainWindowItem(MainWindow *mainWindow) const
{
	return m_mainWindowItems.value(mainWindow, nullptr);
}

}
