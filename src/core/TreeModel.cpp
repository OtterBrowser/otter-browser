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

void TreeModel::setupItem(QStandardItem *item, TreeModel::ItemType type)
{
	item->setData(type, TypeRole);

	if (type != FolderType)
	{
		item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
	}
}

void TreeModel::insertRow(QStandardItem *item, QStandardItem *parent, int row, ItemType type)
{
	if (!item)
	{
		item = new QStandardItem();
	}

	if (!parent)
	{
		parent = invisibleRootItem();
	}

	setupItem(item, type);

	if (row >= 0)
	{
		parent->insertRow(row, item);
	}
	else
	{
		parent->appendRow(item);
	}
}

void TreeModel::insertRow(const QList<QStandardItem*> &items, QStandardItem *parent, int row, ItemType type)
{
	if (!parent)
	{
		parent = invisibleRootItem();
	}

	for (int i = 0; i < items.count(); ++i)
	{
		setupItem(items.at(i), type);
	}

	if (row >= 0)
	{
		parent->insertRow(row, items);
	}
	else
	{
		parent->appendRow(items);
	}
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

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::AccessibleDescriptionRole && static_cast<ItemType>(QStandardItemModel::data(index, TypeRole).toInt()) == SeparatorType)
	{
		return QLatin1String("separator");
	}

	return QStandardItemModel::data(index, role);
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
