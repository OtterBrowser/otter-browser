/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ITEMVIEWWIDGET_H
#define OTTER_ITEMVIEWWIDGET_H

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
	explicit HeaderViewWidget(Qt::Orientation orientation, QWidget *parent = NULL);

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

	explicit ItemViewWidget(QWidget *parent = NULL);

	void setData(const QModelIndex &index, const QVariant &value, int role);
	void setModel(QAbstractItemModel *model);
	void setViewMode(ViewMode mode);
	QStandardItemModel* getModel();
	QStandardItem* getItem(const QModelIndex &index) const;
	QStandardItem* getItem(int row, int column = 0) const;
	QModelIndex getIndex(int row, int column = 0) const;
	QSize sizeHint() const;
	ViewMode getViewMode() const;
	Qt::SortOrder getSortOrder() const;
	int getSortColumn() const;
	int getCurrentRow() const;
	int getPreviousRow() const;
	int getRowCount() const;
	int getColumnCount() const;
	bool canMoveUp() const;
	bool canMoveDown() const;
	bool isModified() const;

public slots:
	void insertRow(const QList<QStandardItem*> &items = QList<QStandardItem*>());
	void insertRow(QStandardItem *item);
	void removeRow();
	void moveUpRow();
	void moveDownRow();
	void setSort(int column, Qt::SortOrder order);
	void setColumnVisibility(int column, bool hide);
	void setFilterString(const QString filter = QString());
	void setFilterRoles(const QSet<int> &roles);

protected:
	void showEvent(QShowEvent *event);
	void dropEvent(QDropEvent *event);
	void startDrag(Qt::DropActions supportedActions);
	void moveRow(bool up);
	bool applyFilter(QStandardItem *item);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void saveState();
	void notifySelectionChanged();
	void updateDropSelection();
	void updateFilter();

private:
	HeaderViewWidget *m_headerWidget;
	QStandardItemModel *m_model;
	QString m_filterString;
	QModelIndex m_currentIndex;
	QModelIndex m_previousIndex;
	QSet<QStandardItem*> m_expandedBranches;
	QSet<int> m_filterRoles;
	ViewMode m_viewMode;
	Qt::SortOrder m_sortOrder;
	int m_sortColumn;
	int m_dragRow;
	int m_dropRow;
	bool m_canGatherExpanded;
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
