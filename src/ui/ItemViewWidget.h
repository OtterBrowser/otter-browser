/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ITEMVIEWWIDGET_H
#define OTTER_ITEMVIEWWIDGET_H

#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeView>

namespace Otter
{

class HeaderViewWidget : public QHeaderView
{
	Q_OBJECT

public:
	explicit HeaderViewWidget(Qt::Orientation orientation, QWidget *parent = nullptr);

public slots:
	void setSort(int column, Qt::SortOrder order);

protected:
	void showEvent(QShowEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

protected slots:
	void toggleColumnVisibility(QAction *action);
	void toggleSort(QAction *action);
	void toggleSort(int column);

signals:
	void sortChanged(int column, Qt::SortOrder order);
	void columnVisibilityChanged(int column, bool hidden);
};

class ItemViewWidget : public QTreeView
{
	Q_OBJECT

public:
	enum ViewMode
	{
		ListViewMode = 0,
		TreeViewMode = 1
	};

	explicit ItemViewWidget(QWidget *parent = nullptr);

	void setData(const QModelIndex &index, const QVariant &value, int role);
	void setModel(QAbstractItemModel *model);
	void setModel(QAbstractItemModel *model, bool useSortProxy);
	void setViewMode(ViewMode mode);
	QStandardItemModel* getSourceModel();
	QSortFilterProxyModel* getProxyModel();
	QStandardItem* getItem(const QModelIndex &index) const;
	QStandardItem* getItem(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex getCurrentIndex(int column = 0) const;
	QModelIndex getIndex(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
	QSize sizeHint() const;
	ViewMode getViewMode() const;
	Qt::SortOrder getSortOrder() const;
	int getSortColumn() const;
	int getCurrentRow() const;
	int getRowCount(const QModelIndex &parent = QModelIndex()) const;
	int getColumnCount(const QModelIndex &parent = QModelIndex()) const;
	bool canMoveUp() const;
	bool canMoveDown() const;
	bool isExclusive() const;
	bool isModified() const;

public slots:
	void insertRow(const QList<QStandardItem*> &items = QList<QStandardItem*>());
	void insertRow(QStandardItem *item);
	void removeRow();
	void moveUpRow();
	void moveDownRow();
	void markAsModified();
	void setSort(int column, Qt::SortOrder order);
	void setColumnVisibility(int column, bool hide);
	void setExclusive(bool isExclusive);
	void setFilterString(const QString filter = QString());
	void setFilterRoles(const QSet<int> &roles);

protected:
	void showEvent(QShowEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void dropEvent(QDropEvent *event);
	void startDrag(Qt::DropActions supportedActions);
	void ensureInitialized();
	void moveRow(bool up);
	bool applyFilter(const QModelIndex &index);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void currentChanged(const QModelIndex &current, const QModelIndex &previous);
	void saveState();
	void notifySelectionChanged();
	void updateDropSelection();
	void updateFilter();

private:
	HeaderViewWidget *m_headerWidget;
	QStandardItemModel *m_sourceModel;
	QSortFilterProxyModel *m_proxyModel;
	QString m_filterString;
	QSet<QModelIndex> m_expandedBranches;
	QSet<int> m_filterRoles;
	ViewMode m_viewMode;
	Qt::SortOrder m_sortOrder;
	int m_sortColumn;
	int m_dragRow;
	int m_dropRow;
	bool m_canGatherExpanded;
	bool m_isExclusive;
	bool m_isModified;
	bool m_isInitialized;

	static int m_treeIndentation;

signals:
	void canMoveUpChanged(bool available);
	void canMoveDownChanged(bool available);
	void needsActionsUpdate();
	void modified();
	void sortChanged(int column, Qt::SortOrder order);
};

}

#endif
