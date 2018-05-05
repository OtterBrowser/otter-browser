/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Application.h"
#include "ThemesManager.h"
#include "../ui/MainWindow.h"
#include "../ui/TabBarWidget.h"
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
	switch (role)
	{
		case Qt::DecorationRole:
			{
				const SessionModel::EntityType type(static_cast<SessionModel::EntityType>(data(SessionModel::TypeRole).toInt()));

				if (type == SessionModel::SessionEntity)
				{
					return ThemesManager::createIcon(QLatin1String("inode-directory"));
				}

				if (type == SessionModel::TrashEntity)
				{
					return ThemesManager::createIcon(QLatin1String("user-trash"));
				}
			}

			break;
		case Qt::ToolTipRole:
			return data(Qt::DisplayRole);
		case SessionModel::IsTrashedRole:
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
			}

			return false;
		default:
			break;
	}

	return QStandardItem::data(role);
}

MainWindowSessionItem::MainWindowSessionItem(MainWindow *mainWindow) : SessionItem(),
	m_mainWindow(mainWindow)
{
	for (int i = 0; i < mainWindow->getTabBar()->count(); ++i)
	{
		const Window *window(mainWindow->getTabBar()->getWindow(i));

		if (window)
		{
			handleWindowAdded(window->getIdentifier());
		}
	}

	connect(mainWindow, &MainWindow::titleChanged, this, &MainWindowSessionItem::notifyMainWindowModified);
	connect(mainWindow, &MainWindow::currentWindowChanged, this, &MainWindowSessionItem::notifyMainWindowModified);
	connect(mainWindow, &MainWindow::windowAdded, this, &MainWindowSessionItem::handleWindowAdded);
	connect(mainWindow, &MainWindow::windowRemoved, this, &MainWindowSessionItem::handleWindowRemoved);
}

void MainWindowSessionItem::handleWindowAdded(quint64 identifier)
{
	for (int i = 0; i < rowCount(); ++i)
	{
		if (index().child(i, 0).data(SessionModel::IdentifierRole).toULongLong() == identifier)
		{
			return;
		}
	}

	insertRow(m_mainWindow->getWindowIndex(identifier), new WindowSessionItem(m_mainWindow->getWindowByIdentifier(identifier)));
}

void MainWindowSessionItem::handleWindowRemoved(quint64 identifier)
{
	for (int i = 0; i < rowCount(); ++i)
	{
		if (index().child(i, 0).data(SessionModel::IdentifierRole).toULongLong() == identifier)
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
	return m_mainWindow->getActiveWindow();
}

MainWindow* MainWindowSessionItem::getMainWindow() const
{
	return m_mainWindow;
}

QVariant MainWindowSessionItem::data(int role) const
{
	if (!m_mainWindow)
	{
		return {};
	}

	switch (role)
	{
		case SessionModel::TitleRole:
			return m_mainWindow->getTitle();
		case SessionModel::UrlRole:
			return m_mainWindow->getUrl();
		case SessionModel::IconRole:
			return ThemesManager::createIcon(QLatin1String("window"));
		case SessionModel::IdentifierRole:
			return m_mainWindow->getIdentifier();
		case SessionModel::TypeRole:
			return SessionModel::MainWindowEntity;
		case SessionModel::IndexRole:
			return m_mainWindow->getCurrentWindowIndex();
		case SessionModel::IsActiveRole:
			return (m_mainWindow == Application::getActiveWindow());
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
	setFlags(flags() | Qt::ItemNeverHasChildren);
}

Window* WindowSessionItem::getActiveWindow() const
{
	return m_window;
}

QVariant WindowSessionItem::data(int role) const
{
	if (!m_window)
	{
		return {};
	}

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
				const MainWindow *mainWindow(MainWindow::findMainWindow(m_window));

				return (mainWindow ? mainWindow->getWindowIndex(m_window->getIdentifier()) : -1);
			}
		case SessionModel::LastActivityRole:
			return m_window->getLastActivity();
		case SessionModel::ZoomRole:
			return m_window->getZoom();
		case SessionModel::IsActiveRole:
			return m_window->isActive();
		case SessionModel::IsAudibleRole:
			return ((m_window->getLoadingState() != WebWidget::DeferredLoadingState && m_window->getWebWidget()) ? m_window->getWebWidget()->isAudible() : false);
		case SessionModel::IsAudioMutedRole:
			return ((m_window->getLoadingState() != WebWidget::DeferredLoadingState && m_window->getWebWidget()) ? m_window->getWebWidget()->isAudioMuted() : false);
		case SessionModel::IsDeferredRole:
			return (m_window->getLoadingState() == WebWidget::DeferredLoadingState);
		case SessionModel::IsPinnedRole:
			return m_window->isPinned();
		case SessionModel::IsPrivateRole:
			return m_window->isPrivate();
		default:
			break;
	}

	return SessionItem::data(role);
}

SessionModel::SessionModel(QObject *parent) : QStandardItemModel(parent),
	m_rootItem(new SessionItem()),
	m_trashItem(new SessionItem())
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

	connect(Application::getInstance(), &Application::windowAdded, this, &SessionModel::handleMainWindowAdded);
	connect(Application::getInstance(), &Application::windowRemoved, this, &SessionModel::handleMainWindowRemoved);
	connect(Application::getInstance(), &Application::currentWindowChanged, this, &SessionModel::modelModified);
	connect(this, &SessionModel::itemChanged, this, &SessionModel::modelModified);
	connect(this, &SessionModel::rowsInserted, this, &SessionModel::modelModified);
	connect(this, &SessionModel::rowsRemoved, this, &SessionModel::modelModified);
	connect(this, &SessionModel::rowsMoved, this, &SessionModel::modelModified);
}

void SessionModel::handleMainWindowAdded(MainWindow *mainWindow)
{
	if (!mainWindow)
	{
		return;
	}

	for (int i = 0; i < m_rootItem->rowCount(); ++i)
	{
		if (index(i, 0, m_rootItem->index()).data(IdentifierRole).toULongLong() == mainWindow->getIdentifier())
		{
			return;
		}
	}

	MainWindowSessionItem *item(new MainWindowSessionItem(mainWindow));

	m_rootItem->appendRow(item);

	m_mainWindowItems[mainWindow] = item;

	connect(mainWindow, &MainWindow::currentWindowChanged, this, &SessionModel::modelModified);
}

void SessionModel::handleMainWindowRemoved(MainWindow *mainWindow)
{
	if (!mainWindow)
	{
		return;
	}

	for (int i = 0; i < m_rootItem->rowCount(); ++i)
	{
		if (index(i, 0, m_rootItem->index()).data(IdentifierRole).toULongLong() == mainWindow->getIdentifier())
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

MainWindowSessionItem* SessionModel::getMainWindowItem(MainWindow *mainWindow) const
{
	return m_mainWindowItems.value(mainWindow, nullptr);
}

}
