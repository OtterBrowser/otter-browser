/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

class HeaderViewWidget final : public QHeaderView
{
	Q_OBJECT

public:
	enum DataRole
	{
		WidthRole = Qt::UserRole,
		UserRole
	};

	explicit HeaderViewWidget(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
	enum SortOrder
	{
		NoOrder = -3,
		AscendingOrder = -2,
		DescendingOrder = -1
	};

	void showEvent(QShowEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	bool viewportEvent(QEvent *event) override;

protected slots:
	void toggleColumnVisibility(QAction *action);
	void toggleSort(QAction *action);
	void handleSectionClicked(int column);
	void setSort(int column, Qt::SortOrder order);

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
		ListView = 0,
		TreeView
	};

	explicit ItemViewWidget(QWidget *parent = nullptr);

	void setData(const QModelIndex &index, const QVariant &value, int role);
	void setModel(QAbstractItemModel *model) override;
	void setModel(QAbstractItemModel *model, bool useSortProxy);
	void setSortRoleMapping(const QMap<int, int> &mapping);
	void setViewMode(ViewMode mode);
	void setModified(bool isModified);
	QStandardItemModel* getSourceModel() const;
	QSortFilterProxyModel* getProxyModel() const;
	QStandardItem* getItem(const QModelIndex &index) const;
	QStandardItem* getItem(int row, int column = 0, const QModelIndex &parent = {}) const;
	QModelIndex getCheckedIndex(const QModelIndex &parent = {}) const;
	QModelIndex getCurrentIndex(int column = 0) const;
	QModelIndex getIndex(int row, int column = 0, const QModelIndex &parent = {}) const;
	QSize sizeHint() const override;
	ViewMode getViewMode() const;
	Qt::SortOrder getSortOrder() const;
	int getSortColumn() const;
	int getCurrentRow() const;
	int getRowCount(const QModelIndex &parent = {}) const;
	int getColumnCount(const QModelIndex &parent = {}) const;
	bool canMoveRowUp() const;
	bool canMoveRowDown() const;
	bool isExclusive() const;
	bool isModified() const;

public slots:
	void insertRow(const QList<QStandardItem*> &items = {});
	void removeRow();
	void moveUpRow();
	void moveDownRow();
	void markAsModified();
	void setSort(int column, Qt::SortOrder order);
	void setColumnVisibility(int column, bool hide);
	void setExclusive(bool isExclusive);
	void setFilterString(const QString &filter);
	void setFilterRoles(const QSet<int> &roles);

protected:
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void startDrag(Qt::DropActions supportedActions) override;
	void ensureInitialized();
	void moveRow(bool moveUp);
	void selectRow(const QModelIndex &index);
	bool applyFilter(const QModelIndex &index, bool parentHasMatch = false);

protected slots:
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
	void saveState();
	void handleOptionChanged(int identifier, const QVariant &value);
	void notifySelectionChanged();
	void updateFilter();
	void updateSize();

private:
	HeaderViewWidget *m_headerWidget;
	QStandardItemModel *m_sourceModel;
	QSortFilterProxyModel *m_proxyModel;
	QString m_filterString;
	QMap<int, int> m_sortRoleMapping;
	QSet<QModelIndex> m_expandedBranches;
	QSet<int> m_filterRoles;
	ViewMode m_viewMode;
	Qt::SortOrder m_sortOrder;
	int m_sortColumn;
	int m_dragRow;
	bool m_canGatherExpanded;
	bool m_isExclusive;
	bool m_isModified;
	bool m_isInitialized;

signals:
	void canMoveRowUpChanged(bool isAllowed);
	void canMoveRowDownChanged(bool isAllowed);
	void needsActionsUpdate();
	void modified();
	void sortChanged(int column, Qt::SortOrder order);
};

}

#endif
