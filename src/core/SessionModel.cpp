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
#include "../ui/MainWindow.h"

namespace Otter
{

SessionItem::SessionItem() : QStandardItem()
{
}

Window* SessionItem::getActiveWindow() const
{
	return nullptr;
}

MainWindowSessionItem::MainWindowSessionItem(MainWindow *mainWindow) : SessionItem(),
	m_mainWindow(mainWindow)
{
}

Window* MainWindowSessionItem::getActiveWindow() const
{
	return m_mainWindow->getWindowsManager()->getWindowByIndex(-1);
}

QVariant MainWindowSessionItem::data(int role) const
{
	switch (role)
	{
		case SessionModel::TitleRole:
			return m_mainWindow->getWindowsManager()->getTitle();
		case SessionModel::UrlRole:
			return m_mainWindow->getWindowsManager()->getUrl();
		case SessionModel::TypeRole:
			return SessionModel::MainWindowEntity;
		case SessionModel::IsPrivateRole:
			return m_mainWindow->getWindowsManager()->isPrivate();
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
}

SessionItem* SessionModel::getRootItem() const
{
	return m_rootItem;
}

SessionItem* SessionModel::getTrashItem() const
{
	return m_trashItem;
}

}
