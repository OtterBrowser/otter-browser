/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ItemViewWidget.h"
#include "ItemDelegate.h"
#include "../core/SessionsManager.h"
#include "../core/Settings.h"

#include <QtCore/QTimer>
#include <QtGui/QDropEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

HeaderViewWidget::HeaderViewWidget(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
	setTextElideMode(Qt::ElideRight);
	setSectionsMovable(true);

	connect(this, SIGNAL(sectionClicked(int)), this, SLOT(toggleSort(int)));
}

void HeaderViewWidget::showEvent(QShowEvent *event)
{
	setSectionsClickable(true);
	setSortIndicatorShown(true);
	setSortIndicator(-1, Qt::AscendingOrder);

	QHeaderView::showEvent(event);
}

void HeaderViewWidget::contextMenuEvent(QContextMenuEvent *event)
{
	ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

	if (!view)
	{
		return;
	}

	const int sortColumn(view->getSortColumn());
	const int sortOrder(view->getSortOrder());

	QMenu menu(this);
	QMenu *sortMenu(menu.addMenu(tr("Sorting")));
	QAction *sortAscendingAction(sortMenu->addAction(tr("Sort Ascending")));
	sortAscendingAction->setData(-2);
	sortAscendingAction->setCheckable(true);
	sortAscendingAction->setChecked(sortOrder == Qt::AscendingOrder);

	QAction *sortDescendingAction(sortMenu->addAction(tr("Sort Descending")));
	sortDescendingAction->setData(-3);
	sortDescendingAction->setCheckable(true);
	sortDescendingAction->setChecked(sortOrder == Qt::DescendingOrder);

	sortMenu->addSeparator();

	QAction *noSortAction(sortMenu->addAction(tr("No Sorting")));
	noSortAction->setData(-1);
	noSortAction->setCheckable(true);
	noSortAction->setChecked(sortColumn < 0);

	sortMenu->addSeparator();

	QMenu *visibilityMenu(menu.addMenu(tr("Visible Columns")));
	visibilityMenu->setEnabled(model()->columnCount() > 1);

	QAction *showAllColumnsAction(nullptr);
	bool allColumnsVisible(true);

	if (visibilityMenu->isEnabled())
	{
		showAllColumnsAction = visibilityMenu->addAction(tr("Show All"));
		showAllColumnsAction->setData(-1);
		showAllColumnsAction->setCheckable(true);

		visibilityMenu->addSeparator();
	}

	for (int i = 0; i < model()->columnCount(); ++i)
	{
		const QString title(model()->headerData(i, orientation()).toString());
		QAction *action(sortMenu->addAction(title));
		action->setData(i);
		action->setCheckable(true);
		action->setChecked(i == sortColumn);

		if (visibilityMenu->isEnabled())
		{
			QAction *action(visibilityMenu->addAction(title));
			action->setData(i);
			action->setCheckable(true);
			action->setChecked(!view->isColumnHidden(i));

			if (!action->isChecked())
			{
				allColumnsVisible = false;
			}
		}
	}

	if (showAllColumnsAction)
	{
		showAllColumnsAction->setChecked(allColumnsVisible);
		showAllColumnsAction->setEnabled(!allColumnsVisible);
	}

	connect(sortMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleSort(QAction*)));
	connect(visibilityMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));

	menu.exec(event->globalPos());
}

void HeaderViewWidget::toggleColumnVisibility(QAction *action)
{
	if (action)
	{
		emit columnVisibilityChanged(action->data().toInt(), !action->isChecked());
	}
}

void HeaderViewWidget::toggleSort(QAction *action)
{
	if (action)
	{
		const int value(action->data().toInt());

		if (value == -2 || value == -3)
		{
			ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

			if (view)
			{
				setSort(view->getSortColumn(), ((value == -2) ? Qt::AscendingOrder : Qt::DescendingOrder));
			}
		}
		else
		{
			toggleSort(value);
		}
	}
}

void HeaderViewWidget::toggleSort(int column)
{
	ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

	if (!view)
	{
		return;
	}

	if (column >= 0 && view->getSortColumn() != column)
	{
		setSort(column, Qt::AscendingOrder);
	}
	else if (column >= 0 && view->getSortOrder() == Qt::AscendingOrder)
	{
		setSort(column, Qt::DescendingOrder);
	}
	else
	{
		setSort(-1, Qt::AscendingOrder);
	}
}

void HeaderViewWidget::setSort(int column, Qt::SortOrder order)
{
	setSortIndicator(column, order);

	emit sortChanged(column, order);
}

int ItemViewWidget::m_treeIndentation = 0;

ItemViewWidget::ItemViewWidget(QWidget *parent) : QTreeView(parent),
	m_headerWidget(new HeaderViewWidget(Qt::Horizontal, this)),
	m_sourceModel(nullptr),
	m_proxyModel(nullptr),
	m_viewMode(ListViewMode),
	m_sortOrder(Qt::AscendingOrder),
	m_sortColumn(-1),
	m_dragRow(-1),
	m_dropRow(-1),
	m_canGatherExpanded(false),
	m_isExclusive(false),
	m_isModified(false),
	m_isInitialized(false)
{
	m_treeIndentation = indentation();

	handleOptionChanged(SettingsManager::Interface_ShowScrollBarsOption, SettingsManager::getValue(SettingsManager::Interface_ShowScrollBarsOption));
	setHeader(m_headerWidget);
	setItemDelegate(new ItemDelegate(this));
	setIndentation(0);
	setAllColumnsShowFocus(true);

	m_filterRoles.insert(Qt::DisplayRole);

	viewport()->setAcceptDrops(true);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
	connect(this, SIGNAL(sortChanged(int,Qt::SortOrder)), m_headerWidget, SLOT(setSort(int,Qt::SortOrder)));
	connect(m_headerWidget, SIGNAL(sortChanged(int,Qt::SortOrder)), this, SLOT(setSort(int,Qt::SortOrder)));
	connect(m_headerWidget, SIGNAL(columnVisibilityChanged(int,bool)), this, SLOT(setColumnVisibility(int,bool)));
	connect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));
}

void ItemViewWidget::showEvent(QShowEvent *event)
{
	ensureInitialized();

	QTreeView::showEvent(event);
}

void ItemViewWidget::keyPressEvent(QKeyEvent *event)
{
	const int rowCount(getRowCount());

	if ((event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) && rowCount > 1 && moveCursor(((event->key() == Qt::Key_Up) ? MoveUp : MoveDown), event->modifiers()) == currentIndex())
	{
		QModelIndex newIndex;

		if (event->key() == Qt::Key_Down)
		{
			for (int i = 0; i < rowCount; ++i)
			{
				const QModelIndex index(getIndex(i, 0));

				if (index.flags().testFlag(Qt::ItemIsSelectable))
				{
					newIndex = index;

					break;
				}
			}
		}
		else
		{
			for (int i = (rowCount - 1); i >= 0; --i)
			{
				const QModelIndex index(getIndex(i, 0));

				if (index.flags().testFlag(Qt::ItemIsSelectable))
				{
					newIndex = index;

					break;
				}
			}
		}

		if (newIndex.isValid())
		{
			QItemSelectionModel::SelectionFlags command(selectionCommand(newIndex, event));

			if (command != QItemSelectionModel::NoUpdate || style()->styleHint(QStyle::SH_ItemView_MovementWithoutUpdatingSelection, 0, this))
			{
				if (event->key() == Qt::Key_Down)
				{
					scrollTo(getIndex(0, 0));
				}

				selectionModel()->setCurrentIndex(newIndex, command);
			}

			event->accept();

			return;
		}
	}

	QTreeView::keyPressEvent(event);
}

void ItemViewWidget::dropEvent(QDropEvent *event)
{
	if (m_viewMode == TreeViewMode)
	{
		QTreeView::dropEvent(event);

		return;
	}

	QDropEvent mutableEvent(QPointF((visualRect(getIndex(0, 0)).x() + 1), event->posF().y()), Qt::MoveAction, event->mimeData(), event->mouseButtons(), event->keyboardModifiers(), event->type());

	QTreeView::dropEvent(&mutableEvent);

	if (!mutableEvent.isAccepted())
	{
		return;
	}

	event->accept();

	m_dropRow = indexAt(event->pos()).row();

	if (m_dragRow <= m_dropRow)
	{
		--m_dropRow;
	}

	if (dropIndicatorPosition() == QAbstractItemView::BelowItem)
	{
		++m_dropRow;
	}

	markAsModified();

	QTimer::singleShot(50, this, SLOT(updateDropSelection()));
}

void ItemViewWidget::startDrag(Qt::DropActions supportedActions)
{
	m_dragRow = currentIndex().row();

	QTreeView::startDrag(supportedActions);
}

void ItemViewWidget::ensureInitialized()
{
	if (m_isInitialized || !model())
	{
		return;
	}

	const QString suffix(QLatin1String("ViewWidget"));
	const QString type(objectName().endsWith(suffix) ? objectName().left(objectName().size() - suffix.size()) : objectName());

	if (type.isEmpty())
	{
		return;
	}

	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(type);

	setSort(settings.getValue(QLatin1String("sortColumn"), -1).toInt(), ((settings.getValue(QLatin1String("sortOrder"), QLatin1String("ascending")).toString() == QLatin1String("ascending")) ? Qt::AscendingOrder : Qt::DescendingOrder));

	const QStringList columns(settings.getValue(QLatin1String("columns")).toString().split(QLatin1Char(','), QString::SkipEmptyParts));
	bool shouldStretchLastSection(true);

	if (!columns.isEmpty())
	{
		for (int i = 0; i < model()->columnCount(); ++i)
		{
			setColumnHidden(i, true);
		}

		disconnect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));

		for (int i = 0; i < columns.count(); ++i)
		{
			setColumnHidden(columns[i].toInt(), false);

			if (m_headerWidget)
			{
				m_headerWidget->moveSection(m_headerWidget->visualIndex(columns[i].toInt()), i);

				if (m_headerWidget->sectionResizeMode(i) == QHeaderView::Stretch)
				{
					shouldStretchLastSection = false;
				}
			}
		}

		connect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));
	}

	if (shouldStretchLastSection)
	{
		m_headerWidget->setStretchLastSection(true);
	}

	m_isInitialized = true;
}

void ItemViewWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QTreeView::currentChanged(current, previous);

	if (selectionModel()->hasSelection())
	{
		if (m_sourceModel)
		{
			emit canMoveUpChanged(canMoveUp());
			emit canMoveDownChanged(canMoveDown());
		}

		emit needsActionsUpdate();
	}
}

void ItemViewWidget::moveRow(bool up)
{
	if (!m_sourceModel)
	{
		return;
	}

	const int sourceRow(currentIndex().row());
	const int destinationRow(up ? (sourceRow - 1) : (sourceRow + 1));

	if ((up && sourceRow > 0) || (!up && sourceRow < (m_sourceModel->rowCount() - 1)))
	{
		m_sourceModel->insertRow(sourceRow, m_sourceModel->takeRow(destinationRow));

		setCurrentIndex(getIndex(destinationRow, 0));
		notifySelectionChanged();

		markAsModified();
	}
}

void ItemViewWidget::insertRow(const QList<QStandardItem*> &items)
{
	if (!m_sourceModel)
	{
		return;
	}

	if (m_sourceModel->rowCount() > 0)
	{
		const int row(currentIndex().row() + 1);

		if (items.count() > 0)
		{
			m_sourceModel->insertRow(row, items);
		}
		else
		{
			m_sourceModel->insertRow(row);
		}

		setCurrentIndex(getIndex(row, 0));
	}
	else
	{
		if (items.isEmpty())
		{
			m_sourceModel->appendRow(new QStandardItem());
		}
		else
		{
			m_sourceModel->appendRow(items);
		}

		setCurrentIndex(getIndex(0, 0));
	}

	markAsModified();
}

void ItemViewWidget::insertRow(QStandardItem *item)
{
	insertRow(QList<QStandardItem*>({item}));
}

void ItemViewWidget::removeRow()
{
	if (!m_sourceModel)
	{
		return;
	}

	const int row(currentIndex().row());

	if (row >= 0)
	{
		QStandardItem *parent(m_sourceModel->itemFromIndex(currentIndex().parent()));

		if (parent)
		{
			parent->removeRow(row);
		}
		else
		{
			m_sourceModel->removeRow(row);
		}

		markAsModified();
	}
}

void ItemViewWidget::moveUpRow()
{
	moveRow(true);
}

void ItemViewWidget::moveDownRow()
{
	moveRow(false);
}

void ItemViewWidget::markAsModified()
{
	m_isModified = true;

	emit modified();
}

void ItemViewWidget::saveState()
{
	if (!m_isInitialized)
	{
		return;
	}

	const QString suffix(QLatin1String("ViewWidget"));
	const QString type(objectName().endsWith(suffix) ? objectName().left(objectName().size() - suffix.size()) : objectName());

	if (type.isEmpty())
	{
		return;
	}

	Settings settings(SessionsManager::getWritableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(type);

	QStringList columns;

	for (int i = 0; i < getColumnCount(); ++i)
	{
		const int section(m_headerWidget->logicalIndex(i));

		if (section >= 0 && !isColumnHidden(section))
		{
			columns.append(QString::number(section));
		}
	}

	settings.setValue(QLatin1String("columns"), columns.join(QLatin1Char(',')));
	settings.setValue(QLatin1String("sortColumn"), ((m_sortColumn >= 0) ? QVariant(m_sortColumn) : QVariant()));
	settings.setValue(QLatin1String("sortOrder"), ((m_sortColumn >= 0) ? QVariant((m_sortOrder == Qt::AscendingOrder) ? QLatin1String("ascending") : QLatin1String("descending")) : QVariant()));
	settings.save();
}

void ItemViewWidget::handleOptionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Interface_ShowScrollBarsOption)
	{
		setHorizontalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	}
}

void ItemViewWidget::notifySelectionChanged()
{
	if (m_sourceModel)
	{
		emit canMoveUpChanged(canMoveUp());
		emit canMoveDownChanged(canMoveDown());
	}

	emit needsActionsUpdate();
}

void ItemViewWidget::updateDropSelection()
{
	setCurrentIndex(getIndex(qBound(0, m_dropRow, getRowCount()), 0));

	m_dropRow = -1;
}

void ItemViewWidget::updateFilter()
{
	for (int i = 0; i < getRowCount(); ++i)
	{
		applyFilter(getIndex(i, 0));
	}
}

void ItemViewWidget::setSort(int column, Qt::SortOrder order)
{
	if (column == m_sortColumn && order == m_sortOrder)
	{
		return;
	}

	m_sortColumn = column;
	m_sortOrder = order;

	sortByColumn(column, order);
	update();
	saveState();

	emit sortChanged(column, order);
}

void ItemViewWidget::setColumnVisibility(int column, bool hide)
{
	if (column < 0)
	{
		for (int i = 0; i < getColumnCount(); ++i)
		{
			setColumnHidden(i, hide);
		}
	}
	else
	{
		setColumnHidden(column, hide);
	}

	saveState();
}

void ItemViewWidget::setExclusive(bool isExclusive)
{
	m_isExclusive = isExclusive;

	update();
}

void ItemViewWidget::setFilterString(const QString filter)
{
	if (filter == m_filterString || !model())
	{
		return;
	}

	if (m_filterString.isEmpty())
	{
		connect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateFilter()));
		connect(model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(updateFilter()));
		connect(model(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateFilter()));
	}

	m_canGatherExpanded = m_filterString.isEmpty();
	m_filterString = filter;

	updateFilter();

	if (m_filterString.isEmpty())
	{
		m_expandedBranches.clear();

		disconnect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateFilter()));
		disconnect(model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(updateFilter()));
		disconnect(model(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateFilter()));
	}
}

void ItemViewWidget::setFilterRoles(const QSet<int> &roles)
{
	m_filterRoles = roles;
}

void ItemViewWidget::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (m_sourceModel)
	{
		m_sourceModel->setData(index, value, role);
	}
}

void ItemViewWidget::setModel(QAbstractItemModel *model)
{
	setModel(model, false);
}

void ItemViewWidget::setModel(QAbstractItemModel *model, bool useSortProxy)
{
	QAbstractItemModel *usedModel(model);

	if (model && useSortProxy)
	{
		m_proxyModel = new QSortFilterProxyModel(this);
		m_proxyModel->setSourceModel(model);
		m_proxyModel->setDynamicSortFilter(true);

		usedModel = m_proxyModel;
	}
	else if (m_proxyModel)
	{
		m_proxyModel->deleteLater();
		m_proxyModel = nullptr;
	}

	m_sourceModel = qobject_cast<QStandardItemModel*>(model);

	QTreeView::setModel(usedModel);

	if (!model)
	{
		return;
	}

	if (isVisible())
	{
		ensureInitialized();
	}

	if (!model->parent())
	{
		model->setParent(this);
	}

	if (m_sourceModel)
	{
		connect(m_sourceModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(notifySelectionChanged()));
	}

	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(notifySelectionChanged()));
	connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(markAsModified()));
	connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(markAsModified()));
	connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(markAsModified()));
	connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(markAsModified()));
}

void ItemViewWidget::setViewMode(ItemViewWidget::ViewMode mode)
{
	m_viewMode = mode;

	setIndentation((mode == TreeViewMode) ? m_treeIndentation : 0);
}

QStandardItemModel* ItemViewWidget::getSourceModel()
{
	return m_sourceModel;
}

QSortFilterProxyModel* ItemViewWidget::getProxyModel()
{
	return m_proxyModel;
}

QStandardItem* ItemViewWidget::getItem(const QModelIndex &index) const
{
	return(m_sourceModel ? m_sourceModel->itemFromIndex(index) : nullptr);
}

QStandardItem* ItemViewWidget::getItem(int row, int column, const QModelIndex &parent) const
{
	return(m_sourceModel ? m_sourceModel->itemFromIndex(getIndex(row, column, parent)) : nullptr);
}

QModelIndex ItemViewWidget::getCheckedIndex(const QModelIndex &parent) const
{
	if (!m_isExclusive || !m_sourceModel)
	{
		return QModelIndex();
	}

	for (int i = 0; i < m_sourceModel->rowCount(parent); ++i)
	{
		const QModelIndex index(m_sourceModel->index(i, 0, parent));

		if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
		{
			return index;
		}

		const QModelIndex result(getCheckedIndex(index));

		if (result.isValid())
		{
			return result;
		}
	}

	return QModelIndex();
}

QModelIndex ItemViewWidget::getCurrentIndex(int column) const
{
	if (!selectionModel()->hasSelection())
	{
		return QModelIndex();
	}

	if (column >= 0)
	{
		return currentIndex().sibling(currentIndex().row(), column);
	}

	return currentIndex();
}

QModelIndex ItemViewWidget::getIndex(int row, int column, const QModelIndex &parent) const
{
	return (model() ? model()->index(row, column, parent) : QModelIndex());
}

QSize ItemViewWidget::sizeHint() const
{
	const QSize size(QTreeView::sizeHint());

	if (m_sourceModel && m_sourceModel->columnCount() == 1)
	{
		return QSize((sizeHintForColumn(0) + (frameWidth() * 2)), size.height());
	}

	return size;
}

ItemViewWidget::ViewMode ItemViewWidget::getViewMode() const
{
	return m_viewMode;
}

Qt::SortOrder ItemViewWidget::getSortOrder() const
{
	return m_sortOrder;
}

int ItemViewWidget::getSortColumn() const
{
	return m_sortColumn;
}

int ItemViewWidget::getCurrentRow() const
{
	return (selectionModel()->hasSelection() ? currentIndex().row() : -1);
}

int ItemViewWidget::getRowCount(const QModelIndex &parent) const
{
	return (model() ? model()->rowCount(parent) : 0);
}

int ItemViewWidget::getColumnCount(const QModelIndex &parent) const
{
	return (model() ? model()->columnCount(parent) : 0);
}

bool ItemViewWidget::canMoveUp() const
{
	return (currentIndex().row() > 0 && getRowCount() > 1);
}

bool ItemViewWidget::canMoveDown() const
{
	const int currentRow(currentIndex().row());
	const int rowCount(getRowCount());

	return (currentRow >= 0 && rowCount > 1 && currentRow < (rowCount - 1));
}

bool ItemViewWidget::isExclusive() const
{
	return m_isExclusive;
}

bool ItemViewWidget::applyFilter(const QModelIndex &index)
{
	bool hasFound(m_filterString.isEmpty());
	const bool isFolder(!index.flags().testFlag(Qt::ItemNeverHasChildren));

	if (isFolder)
	{
		if (m_canGatherExpanded && isExpanded(index))
		{
			m_expandedBranches.insert(index);
		}

		const int rowCount(getRowCount(index));

		for (int i = 0; i < rowCount; ++i)
		{
			if (applyFilter(index.child(i, 0)))
			{
				hasFound = true;
			}
		}
	}
	else
	{
		const int columnCount(index.parent().isValid() ? getRowCount(index.parent()) : getColumnCount());

		for (int i = 0; i < columnCount; ++i)
		{
			const QModelIndex childIndex(index.sibling(index.row(), i));

			if (!childIndex.isValid())
			{
				continue;
			}

			QSet<int>::iterator iterator;

			for (iterator = m_filterRoles.begin(); iterator != m_filterRoles.end(); ++iterator)
			{
				if (childIndex.data(*iterator).toString().contains(m_filterString, Qt::CaseInsensitive))
				{
					hasFound = true;

					break;
				}
			}

			if (hasFound)
			{
				break;
			}
		}
	}

	setRowHidden(index.row(), index.parent(), (!hasFound || (isFolder && getRowCount(index) == 0)));

	if (isFolder)
	{
		setExpanded(index, ((hasFound && !m_filterString.isEmpty()) || (m_filterString.isEmpty() && m_expandedBranches.contains(index))));
	}

	return hasFound;
}

bool ItemViewWidget::isModified() const
{
	return m_isModified;
}

}
