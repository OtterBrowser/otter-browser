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

#ifndef OTTER_TREEMODEL_H
#define OTTER_TREEMODEL_H

#include <QtGui/QStandardItemModel>

namespace Otter
{

class TreeModel : public QStandardItemModel
{
public:
	enum ItemType
	{
		UnknownType = 0,
		FolderType,
		EntryType,
		SeparatorType
	};

	enum ItemRole
	{
		TitleRole = Qt::DisplayRole,
		DescriptionRole = Qt::ToolTipRole,
		TypeRole = Qt::UserRole,
		UserRole
	};

	explicit TreeModel(QObject *parent = nullptr);

	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
};

}

#endif
