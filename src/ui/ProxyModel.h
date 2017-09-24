/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_PROXYMODEL_H
#define OTTER_PROXYMODEL_H

#include <QtCore/QIdentityProxyModel>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class ProxyModel final : public QIdentityProxyModel
{
	Q_OBJECT

public:
	explicit ProxyModel(QStandardItemModel *model, const QVector<QPair<QString, int> > &mapping, QObject *parent = nullptr);

	QMimeData* mimeData(const QModelIndexList &indexes) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	QModelIndex sibling(int row, int column, const QModelIndex &index) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	int columnCount(const QModelIndex &parent) const override;
	int rowCount(const QModelIndex &parent) const override;
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

private:
	QStandardItemModel *m_model;
	QVector<QPair<QString, int> > m_mapping;
	QMap<int, QMap<int, QVariant> > m_headerData;
};

}

#endif
