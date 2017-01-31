/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TreeModel.h"
#include "ThemesManager.h"

#include <QtCore/QMimeData>

namespace Otter
{

TreeModel::TreeModel(QObject *parent) : QStandardItemModel(parent)
{
}

QMimeData* TreeModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData(QStandardItemModel::mimeData(indexes));

	if (indexes.count() == 1)
	{
		mimeData->setProperty("x-item-index", indexes.first());
	}

	return mimeData;
}

bool TreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action)
	Q_UNUSED(column)

	if (static_cast<ItemType>(parent.data(TypeRole).toInt()) != FolderType)
	{
		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	QStandardItem *item(itemFromIndex(data->property("x-item-index").toModelIndex()));
	QStandardItem *targetItem(itemFromIndex(parent.sibling(parent.row(), 0)));

	if (!targetItem)
	{
		targetItem = invisibleRootItem();
	}

	if (!item || !targetItem)
	{
		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	const QList<QStandardItem*> sourceItems(item->parent() ? item->parent()->takeRow(item->row()) : takeRow(item->row()));

	if (sourceItems.isEmpty())
	{
		return false;
	}

	if (row < 0)
	{
		targetItem->appendRow(sourceItems);
	}
	else
	{
		if (item->parent() == targetItem && item->row() < row)
		{
			--row;
		}

		targetItem->insertRow(row, sourceItems);
	}

	return true;
}

}
