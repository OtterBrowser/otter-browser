/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ITEMMODEL_H
#define OTTER_ITEMMODEL_H

#include <QtGui/QStandardItemModel>

namespace Otter
{

class ItemModel : public QStandardItemModel
{
	Q_OBJECT

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

	class Item : public QStandardItem
	{
	public:
		explicit Item(ItemModel::ItemType type = EntryType);
		explicit Item(const QString &title, ItemModel::ItemType type = EntryType);
		explicit Item(const QIcon &icon, const QString &title, ItemModel::ItemType type = EntryType);

		bool isAncestorOf(QStandardItem *child) const;

	protected:
		void setup(ItemModel::ItemType type);
	};

	explicit ItemModel(QObject *parent = nullptr);

	void insertRow(QStandardItem *item = nullptr, QStandardItem *parent = nullptr, int row = -1, ItemType type = EntryType);
	void insertRow(const QList<QStandardItem*> &items, QStandardItem *parent = nullptr, int row = -1, ItemType type = EntryType);
	void setExclusive(bool isExclusive);
	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariantList getAllData(int role, int column = -1, const QModelIndex &parent = {}) const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	bool isExclusive() const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

protected:
	void setupItem(QStandardItem *item, ItemType type);
	void resetCheckState(const QModelIndex &parent);

private:
	bool m_isExclusive;
	bool m_isIgnoringCheckStateReset;
};

}

#endif
