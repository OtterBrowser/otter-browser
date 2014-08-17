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
#include "WebBackend.h"
#include "WebBackendsManager.h"

namespace Otter
{

BookmarksItem::BookmarksItem() : QStandardItem()
{
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DecorationRole && QStandardItem::data(Qt::DecorationRole).isNull())
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
}

}
