/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ProxyModel.h"
#include "../core/Utils.h"

#include <QtCore/QDateTime>

namespace Otter
{

ProxyModel::ProxyModel(QStandardItemModel *model, const QVector<ProxyModel::Column> &mapping, QObject *parent) : QIdentityProxyModel(parent),
	m_model(model),
	m_mapping(mapping)
{
	setSourceModel(model);
}

QMimeData* ProxyModel::mimeData(const QModelIndexList &indexes) const
{
	QModelIndexList sourceIndexes;
	sourceIndexes.reserve(indexes.count());

	for (int i = 0; i < indexes.count(); ++i)
	{
		const QModelIndex index(indexes.at(i));
		const QModelIndex sourceIndex(mapToSource(index.sibling(index.row(), 0)));

		if (!sourceIndexes.contains(sourceIndex))
		{
			sourceIndexes.append(sourceIndex);
		}
	}

	return m_model->mimeData(sourceIndexes);
}

QVariant ProxyModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() < m_mapping.count())
	{
		const QVariant data(mapToSource(index.sibling(index.row(), 0)).data(m_mapping.at(index.column()).role));

		if (data.type() == QVariant::DateTime)
		{
			return Utils::formatDateTime(data.toDateTime());
		}

		return data;
	}

	if (role == Qt::DecorationRole && index.column() > 0)
	{
		return {};
	}

	return QIdentityProxyModel::data(index.sibling(index.row(), 0), role);
}

QVariant ProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && section >= 0 && section < m_mapping.count())
	{
		if (role == Qt::DisplayRole)
		{
			return m_mapping[section].getTitle();
		}

		if (m_headerData.contains(section) && m_headerData[section].contains(role))
		{
			return m_headerData[section][role];
		}
	}

	return QIdentityProxyModel::headerData(section, orientation, role);
}

QModelIndex ProxyModel::index(int row, int column, const QModelIndex &parent) const
{
	const QModelIndex sourceParent(mapToSource(parent));
	const QModelIndex sourceIndex(m_model->index(row, 0, sourceParent));

	if (column > 0)
	{
		return createIndex(row, column, sourceIndex.internalId());
	}

	return mapFromSource(sourceIndex);
}

QModelIndex ProxyModel::sibling(int row, int column, const QModelIndex &index) const
{
	return this->index(row, column, index.parent());
}

Qt::ItemFlags ProxyModel::flags(const QModelIndex &index) const
{
	return QIdentityProxyModel::flags((index.column() == 0) ? index : index.sibling(index.row(), 0));
}

int ProxyModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	return m_mapping.count();
}

int ProxyModel::rowCount(const QModelIndex &parent) const
{
	return (m_model ? m_model->rowCount(mapToSource(parent)) : 0);
}

bool ProxyModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (orientation == Qt::Vertical)
	{
		return false;
	}

	if (!m_headerData.contains(section))
	{
		m_headerData[section] = {};
	}

	m_headerData[section][role] = value;

	emit headerDataChanged(orientation, section, section);

	return true;
}

}
