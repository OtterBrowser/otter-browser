/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/IniSettings.h"
#include "../core/SessionsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QDropEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

HeaderViewWidget::HeaderViewWidget(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
	setMinimumSectionSize(0);
	setTextElideMode(Qt::ElideRight);
	setSectionsMovable(true);

	connect(this, &HeaderViewWidget::sectionClicked, this, &HeaderViewWidget::handleSectionClicked);
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
	const ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

	if (!view)
	{
		return;
	}

	const int sortColumn(view->getSortColumn());
	const int sortOrder(view->getSortOrder());
	QMenu menu(this);
	QMenu *sortMenu(menu.addMenu(tr("Sorting")));
	QAction *sortAscendingAction(sortMenu->addAction(tr("Sort Ascending")));
	sortAscendingAction->setData(AscendingOrder);
	sortAscendingAction->setCheckable(true);
	sortAscendingAction->setChecked(sortColumn >= 0 && sortOrder == Qt::AscendingOrder);

	QAction *sortDescendingAction(sortMenu->addAction(tr("Sort Descending")));
	sortDescendingAction->setData(DescendingOrder);
	sortDescendingAction->setCheckable(true);
	sortDescendingAction->setChecked(sortColumn >= 0 && sortOrder == Qt::DescendingOrder);

	sortMenu->addSeparator();

	QAction *noSortAction(sortMenu->addAction(tr("No Sorting")));
	noSortAction->setData(NoOrder);
	noSortAction->setCheckable(true);
	noSortAction->setChecked(sortColumn < 0);

	QActionGroup orderActionGroup(sortMenu);
	orderActionGroup.setExclusive(true);
	orderActionGroup.addAction(sortAscendingAction);
	orderActionGroup.addAction(sortDescendingAction);
	orderActionGroup.addAction(noSortAction);

	sortMenu->addSeparator();

	QMenu *visibilityMenu(menu.addMenu(tr("Visible Columns")));
	visibilityMenu->setEnabled(model()->columnCount() > 1);

	QAction *showAllColumnsAction(nullptr);
	bool areAllColumnsVisible(true);

	if (visibilityMenu->isEnabled())
	{
		showAllColumnsAction = visibilityMenu->addAction(tr("Show All"));
		showAllColumnsAction->setData(-1);
		showAllColumnsAction->setCheckable(true);

		visibilityMenu->addSeparator();
	}

	QActionGroup sortActionGroup(sortMenu);
	sortActionGroup.setExclusive(true);

	for (int i = 0; i < model()->columnCount(); ++i)
	{
		const QString title(model()->headerData(i, orientation()).toString().isEmpty() ? tr("(Untitled)") : model()->headerData(i, orientation()).toString());
		QAction *sortAction(sortMenu->addAction(title));
		sortAction->setData(i);
		sortAction->setCheckable(true);
		sortAction->setChecked(i == sortColumn);

		sortActionGroup.addAction(sortAction);

		if (visibilityMenu->isEnabled())
		{
			QAction *visibilityAction(visibilityMenu->addAction(title));
			visibilityAction->setData(i);
			visibilityAction->setCheckable(true);
			visibilityAction->setChecked(!view->isColumnHidden(i));

			if (!visibilityAction->isChecked())
			{
				areAllColumnsVisible = false;
			}
		}
	}

	if (showAllColumnsAction)
	{
		showAllColumnsAction->setChecked(areAllColumnsVisible);
		showAllColumnsAction->setEnabled(!areAllColumnsVisible);
	}

	connect(sortMenu, &QMenu::triggered, this, &HeaderViewWidget::toggleSort);
	connect(visibilityMenu, &QMenu::triggered, this, &HeaderViewWidget::toggleColumnVisibility);

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
	const ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

	if (action && view)
	{
		const int value(action->data().toInt());
		int column(view->getSortColumn());

		if (column < 0)
		{
			for (int i = 0; i < count(); ++i)
			{
				column = logicalIndex(i);

				if (!isSectionHidden(column))
				{
					break;
				}
			}
		}

		if (value == AscendingOrder)
		{
			setSort(column, Qt::AscendingOrder);
		}
		else if (value == DescendingOrder)
		{
			setSort(column, Qt::DescendingOrder);
		}
		else
		{
			handleSectionClicked(value);
		}
	}
}

void HeaderViewWidget::handleSectionClicked(int column)
{
	const ItemViewWidget *view(qobject_cast<ItemViewWidget*>(parent()));

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

bool HeaderViewWidget::viewportEvent(QEvent *event)
{
	if (event->type() == QEvent::ToolTip && model())
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const int column(logicalIndexAt(helpEvent->pos()));

		if (column >= 0)
		{
			const QString text(model()->headerData(column, orientation(), Qt::DisplayRole).toString());

			if (!text.isEmpty())
			{
				QToolTip::showText(helpEvent->globalPos(), text, this);

				return true;
			}
		}
	}

	return QHeaderView::viewportEvent(event);
}

ItemViewWidget::ItemViewWidget(QWidget *parent) : QTreeView(parent),
	m_headerWidget(new HeaderViewWidget(Qt::Horizontal, this)),
	m_sourceModel(nullptr),
	m_proxyModel(nullptr),
	m_viewMode(ListView),
	m_sortOrder(Qt::AscendingOrder),
	m_sortColumn(-1),
	m_dragRow(-1),
	m_canGatherExpanded(false),
	m_isExclusive(false),
	m_isModified(false),
	m_isInitialized(false)
{
	handleOptionChanged(SettingsManager::Interface_ShowScrollBarsOption, SettingsManager::getOption(SettingsManager::Interface_ShowScrollBarsOption));
	setHeader(m_headerWidget);
	setItemDelegate(new ItemDelegate(this));
	setIndentation(0);
	setAllColumnsShowFocus(true);

	m_filterRoles.insert(Qt::DisplayRole);

	viewport()->setAcceptDrops(true);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ItemViewWidget::handleOptionChanged);
	connect(m_headerWidget, &HeaderViewWidget::sortChanged, this, &ItemViewWidget::setSort);
	connect(m_headerWidget, &HeaderViewWidget::columnVisibilityChanged, this, &ItemViewWidget::setColumnVisibility);
	connect(m_headerWidget, &HeaderViewWidget::sectionMoved, this, &ItemViewWidget::saveState);
}

void ItemViewWidget::showEvent(QShowEvent *event)
{
	ensureInitialized();

	QTreeView::showEvent(event);
}

void ItemViewWidget::resizeEvent(QResizeEvent *event)
{
	QTreeView::resizeEvent(event);

	updateSize();
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
			const QItemSelectionModel::SelectionFlags command(selectionCommand(newIndex, event));

			if (command != QItemSelectionModel::NoUpdate || style()->styleHint(QStyle::SH_ItemView_MovementWithoutUpdatingSelection, nullptr, this))
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
	if (m_viewMode == TreeView)
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

	int dropRow(indexAt(event->pos()).row());

	if (dropRow > m_dragRow)
	{
		--dropRow;
	}

	if (dropIndicatorPosition() == QAbstractItemView::BelowItem)
	{
		++dropRow;
	}

	markAsModified();

	QTimer::singleShot(0, this, [=]()
	{
		setCurrentIndex(getIndex(qBound(0, dropRow, getRowCount()), 0));
	});
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

	const QString name(Utils::normalizeObjectName(objectName(), QLatin1String("ViewWidget")));

	if (name.isEmpty())
	{
		return;
	}

	updateSize();

	IniSettings settings(SessionsManager::getReadableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(name);

	setSort(settings.getValue(QLatin1String("sortColumn"), -1).toInt(), ((settings.getValue(QLatin1String("sortOrder"), QLatin1String("ascending")).toString() == QLatin1String("ascending")) ? Qt::AscendingOrder : Qt::DescendingOrder));

	const QStringList columns(settings.getValue(QLatin1String("columns")).toString().split(QLatin1Char(','), QString::SkipEmptyParts));
	bool shouldStretchLastSection(true);

	if (!columns.isEmpty())
	{
		for (int i = 0; i < model()->columnCount(); ++i)
		{
			setColumnHidden(i, true);
		}

		disconnect(m_headerWidget, &HeaderViewWidget::sectionMoved, this, &ItemViewWidget::saveState);

		for (int i = 0; i < columns.count(); ++i)
		{
			const int column(columns.at(i).toInt());

			setColumnHidden(column, false);

			if (m_headerWidget)
			{
				m_headerWidget->moveSection(m_headerWidget->visualIndex(column), i);

				if (m_headerWidget->sectionResizeMode(i) == QHeaderView::Stretch)
				{
					shouldStretchLastSection = false;
				}
			}
		}

		connect(m_headerWidget, &HeaderViewWidget::sectionMoved, this, &ItemViewWidget::saveState);
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

	if (selectionModel() && selectionModel()->hasSelection())
	{
		if (m_sourceModel)
		{
			emit canMoveRowUpChanged(canMoveRowUp());
			emit canMoveRowDownChanged(canMoveRowDown());
		}

		emit needsActionsUpdate();
	}
}

void ItemViewWidget::moveRow(bool moveUp)
{
	if (!m_sourceModel)
	{
		return;
	}

	const int sourceRow(currentIndex().row());
	const int destinationRow(moveUp ? (sourceRow - 1) : (sourceRow + 1));

	if ((moveUp && sourceRow > 0) || (!moveUp && sourceRow < (m_sourceModel->rowCount() - 1)))
	{
		m_sourceModel->insertRow(sourceRow, m_sourceModel->takeRow(destinationRow));

		selectRow(getIndex(destinationRow, 0));
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

		selectRow(getIndex(row, 0));
	}
	else
	{
		if (items.isEmpty())
		{
			QStandardItem *item(new QStandardItem());
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			m_sourceModel->appendRow(item);
		}
		else
		{
			m_sourceModel->appendRow(items);
		}

		selectRow(getIndex(0, 0));
	}

	markAsModified();
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

void ItemViewWidget::selectRow(const QModelIndex &index)
{
	setCurrentIndex(index);
	scrollTo(index);
}

void ItemViewWidget::markAsModified()
{
	setModified(true);
}

void ItemViewWidget::saveState()
{
	if (!m_isInitialized)
	{
		return;
	}

	const QString name(Utils::normalizeObjectName(objectName(), QLatin1String("ViewWidget")));

	if (name.isEmpty())
	{
		return;
	}

	IniSettings settings(SessionsManager::getWritableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(name);

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
		emit canMoveRowUpChanged(canMoveRowUp());
		emit canMoveRowDownChanged(canMoveRowDown());
	}

	emit needsActionsUpdate();
}

void ItemViewWidget::updateFilter()
{
	for (int i = 0; i < getRowCount(); ++i)
	{
		applyFilter(getIndex(i, 0));
	}
}

void ItemViewWidget::updateSize()
{
	if (!m_headerWidget || !model())
	{
		return;
	}

	int maximumSectionWidth(0);
	int minimumTotalWidth(0);
	QVector<int> widestSections;

	for (int i = 0; i < m_headerWidget->count(); ++i)
	{
		const int width(model()->headerData(i, Qt::Horizontal, HeaderViewWidget::WidthRole).toInt());

		if (width > 0)
		{
			m_headerWidget->resizeSection(i, width);

			if (width > maximumSectionWidth)
			{
				widestSections = {i};

				maximumSectionWidth = width;
			}
			else if (width == maximumSectionWidth)
			{
				widestSections.append(i);
			}

			minimumTotalWidth += width;
		}
		else
		{
			minimumTotalWidth += m_headerWidget->defaultSectionSize();
		}
	}

	if (!widestSections.isEmpty() && minimumTotalWidth < m_headerWidget->width())
	{
		const int sectionWidth((m_headerWidget->width() - minimumTotalWidth) / widestSections.count());

		for (int i = 0; i < widestSections.count(); ++i)
		{
			m_headerWidget->resizeSection(widestSections.at(i), (m_headerWidget->sectionSize(widestSections.at(i)) + sectionWidth));
		}
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

	const int sortRole(m_proxyModel ? m_proxyModel->sortRole() : (m_sourceModel ? m_sourceModel->sortRole() : Qt::DisplayRole));

	if (m_sortRoleMapping.contains(column))
	{
		if (m_proxyModel)
		{
			m_proxyModel->setSortRole(m_sortRoleMapping[column]);
		}
		else if (m_sourceModel)
		{
			m_sourceModel->setSortRole(m_sortRoleMapping[column]);
		}
	}

	sortByColumn(column, order);
	update();
	saveState();

	if (m_proxyModel)
	{
		m_proxyModel->setSortRole(sortRole);
	}
	else if (m_sourceModel)
	{
		m_sourceModel->setSortRole(sortRole);
	}

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

void ItemViewWidget::setFilterString(const QString &filter)
{
	if (filter == m_filterString || !model())
	{
		return;
	}

	if (m_filterString.isEmpty())
	{
		connect(model(), &QAbstractItemModel::rowsInserted, this, &ItemViewWidget::updateFilter);
		connect(model(), &QAbstractItemModel::rowsMoved, this, &ItemViewWidget::updateFilter);
		connect(model(), &QAbstractItemModel::rowsRemoved, this, &ItemViewWidget::updateFilter);
	}

	m_canGatherExpanded = m_filterString.isEmpty();
	m_filterString = filter;

	updateFilter();

	if (m_filterString.isEmpty())
	{
		m_expandedBranches.clear();

		disconnect(model(), &QAbstractItemModel::rowsInserted, this, &ItemViewWidget::updateFilter);
		disconnect(model(), &QAbstractItemModel::rowsMoved, this, &ItemViewWidget::updateFilter);
		disconnect(model(), &QAbstractItemModel::rowsRemoved, this, &ItemViewWidget::updateFilter);
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
	QAbstractItemModel *activeModel(model);

	if (model && useSortProxy)
	{
		m_proxyModel = new QSortFilterProxyModel(this);
		m_proxyModel->setSourceModel(model);
		m_proxyModel->setDynamicSortFilter(true);

		activeModel = m_proxyModel;
	}
	else if (m_proxyModel)
	{
		m_proxyModel->deleteLater();
		m_proxyModel = nullptr;
	}

	m_sourceModel = qobject_cast<QStandardItemModel*>(model);

	QTreeView::setModel(activeModel);

	if (!model)
	{
		emit needsActionsUpdate();

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
		connect(m_sourceModel, &QStandardItemModel::itemChanged, this, &ItemViewWidget::notifySelectionChanged);
	}

	emit needsActionsUpdate();

	connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ItemViewWidget::notifySelectionChanged);
	connect(model, &QAbstractItemModel::dataChanged, this, &ItemViewWidget::markAsModified);
	connect(model, &QAbstractItemModel::headerDataChanged, this, &ItemViewWidget::updateSize);
	connect(model, &QAbstractItemModel::rowsInserted, this, &ItemViewWidget::markAsModified);
	connect(model, &QAbstractItemModel::rowsRemoved, this, &ItemViewWidget::markAsModified);
	connect(model, &QAbstractItemModel::rowsMoved, this, &ItemViewWidget::markAsModified);
}

void ItemViewWidget::setSortRoleMapping(const QMap<int, int> &mapping)
{
	m_sortRoleMapping = mapping;
}

void ItemViewWidget::setViewMode(ItemViewWidget::ViewMode mode)
{
	m_viewMode = mode;

	setIndentation((mode == TreeView) ? style()->pixelMetric(QStyle::PM_TreeViewIndentation) : 0);
}

void ItemViewWidget::setModified(bool isModified)
{
	m_isModified = isModified;

	if (isModified)
	{
		emit modified();
	}

	emit needsActionsUpdate();
}

QStandardItemModel* ItemViewWidget::getSourceModel() const
{
	return m_sourceModel;
}

QSortFilterProxyModel* ItemViewWidget::getProxyModel() const
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
		return {};
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

	return {};
}

QModelIndex ItemViewWidget::getCurrentIndex(int column) const
{
	if (!selectionModel() || !selectionModel()->hasSelection())
	{
		return {};
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
	return ((selectionModel() && selectionModel()->hasSelection()) ? currentIndex().row() : -1);
}

int ItemViewWidget::getRowCount(const QModelIndex &parent) const
{
	return (model() ? model()->rowCount(parent) : 0);
}

int ItemViewWidget::getColumnCount(const QModelIndex &parent) const
{
	return (model() ? model()->columnCount(parent) : 0);
}

bool ItemViewWidget::canMoveRowUp() const
{
	return (currentIndex().row() > 0 && getRowCount() > 1);
}

bool ItemViewWidget::canMoveRowDown() const
{
	const int currentRow(currentIndex().row());
	const int rowCount(getRowCount());

	return (currentRow >= 0 && rowCount > 1 && currentRow < (rowCount - 1));
}

bool ItemViewWidget::isExclusive() const
{
	return m_isExclusive;
}

bool ItemViewWidget::applyFilter(const QModelIndex &index, bool parentHasMatch)
{
	const bool isFolder(!index.flags().testFlag(Qt::ItemNeverHasChildren));
	const bool hasFilter(!m_filterString.isEmpty());
	bool hasMatch(!hasFilter || (isFolder && parentHasMatch));

	if (!hasMatch)
	{
		for (int i = 0; i < getColumnCount(index.parent()); ++i)
		{
			const QModelIndex childIndex(index.sibling(index.row(), i));

			if (!childIndex.isValid())
			{
				continue;
			}

			QSet<int>::iterator iterator;

			for (iterator = m_filterRoles.begin(); iterator != m_filterRoles.end(); ++iterator)
			{
				const QVariant roleData(childIndex.data(*iterator));

				if (!roleData.isNull() && roleData.toString().contains(m_filterString, Qt::CaseInsensitive))
				{
					hasMatch = true;

					break;
				}
			}

			if (hasMatch)
			{
				break;
			}
		}
	}

	if (isFolder)
	{
		if (m_canGatherExpanded && isExpanded(index))
		{
			m_expandedBranches.insert(index);
		}

		const int rowCount(getRowCount(index));
		bool folderHasMatch(false);

		for (int i = 0; i < rowCount; ++i)
		{
			if (applyFilter(index.child(i, 0), hasMatch))
			{
				folderHasMatch = true;
			}
		}

		if (!hasMatch)
		{
			hasMatch = folderHasMatch;
		}
	}

	setRowHidden(index.row(), index.parent(), (hasFilter ? (!(hasMatch || parentHasMatch) || (isFolder && getRowCount(index) == 0)) : false));

	if (isFolder)
	{
		setExpanded(index, ((hasMatch && hasFilter) || (!hasFilter && m_expandedBranches.contains(index))));
	}

	return hasMatch;
}

bool ItemViewWidget::isModified() const
{
	return m_isModified;
}

}
