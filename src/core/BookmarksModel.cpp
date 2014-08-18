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

#include "BookmarksModel.h"
#include "Utils.h"
#include "WebBackend.h"
#include "WebBackendsManager.h"

namespace Otter
{

BookmarksItem::BookmarksItem(BookmarkType type, const QUrl &url, const QString &title) : QStandardItem()
{
	setData(type, BookmarksModel::TypeRole);
	setData(url, BookmarksModel::UrlRole);
	setData(title, BookmarksModel::TitleRole);

	if (type == RootBookmark || type == FolderBookmark)
	{
		setData(Utils::getIcon(QLatin1String("inode-directory")), Qt::DecorationRole);
	}
	else if (type == TrashBookmark)
	{
		setData(Utils::getIcon(QLatin1String("user-trash")), Qt::DecorationRole);
		setEnabled(false);
	}
	else if (type == SeparatorBookmark)
	{
		setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
	}
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DecorationRole && QStandardItem::data(Qt::DecorationRole).isNull() && static_cast<BookmarkType>(QStandardItem::data(BookmarksModel::TypeRole).toInt()) != SeparatorBookmark)
	{
		return WebBackendsManager::getBackend()->getIconForUrl(data(BookmarksModel::UrlRole).toUrl());
	}

	return QStandardItem::data(role);
}

void BookmarksItem::setData(const QVariant &value, int role)
{
	QStandardItem::setData(value, role);
}

BookmarksModel::BookmarksModel(QObject *parent) : QStandardItemModel(parent)
{
	appendRow(new BookmarksItem(RootBookmark, QUrl(), tr("Bookmarks")));
	appendRow(new BookmarksItem(TrashBookmark, QUrl(), tr("Trash")));
}

BookmarksItem* BookmarksModel::getRootItem()
{
	return dynamic_cast<BookmarksItem*>(item(0, 0));
}

BookmarksItem* BookmarksModel::getTrashItem()
{
	return dynamic_cast<BookmarksItem*>(item(1, 0));
}

}
