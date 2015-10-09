/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ItemViewWidget.h"
#include "../core/SettingsManager.h"

#include <QtCore/QTimer>
#include <QtGui/QDropEvent>

namespace Otter
{

ItemViewWidget::ItemViewWidget(QWidget *parent) : QTreeView(parent),
	m_model(NULL),
	m_dropRow(-1),
	m_isModified(false)
{
	optionChanged(QLatin1String("Interface/ShowScrollBars"), SettingsManager::getValue(QLatin1String("Interface/ShowScrollBars")));
	setIndentation(0);
	setAllColumnsShowFocus(true);

	viewport()->setAcceptDrops(true);
}

void ItemViewWidget::dropEvent(QDropEvent *event)
{
	QDropEvent modifiedEvent(QPointF((visualRect(m_model->index(0, 0)).x() + 1), event->posF().y()), Qt::MoveAction, event->mimeData(), event->mouseButtons(), event->keyboardModifiers(), event->type());

	QTreeView::dropEvent(&modifiedEvent);

	if (modifiedEvent.isAccepted())
	{
		event->accept();

		m_dropRow = indexAt(event->pos()).row();

		if (dropIndicatorPosition() == QAbstractItemView::BelowItem)
		{
			++m_dropRow;
		}

		m_isModified = true;

		emit modified();

		QTimer::singleShot(50, this, SLOT(updateDropSelection()));
	}
}

void ItemViewWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Interface/ShowScrollBars"))
	{
		setHorizontalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	}
}

void ItemViewWidget::moveRow(bool up)
{
	if (!m_model)
	{
		return;
	}

	const int sourceRow = currentIndex().row();
	const int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));

	if ((up && sourceRow > 0) || (!up && sourceRow < (m_model->rowCount() - 1)))
	{
		m_model->insertRow(sourceRow, m_model->takeRow(destinationRow));

		setCurrentIndex(getIndex(destinationRow, 0));
		notifySelectionChanged();

		m_isModified = true;

		emit modified();
	}
}

void ItemViewWidget::insertRow(const QList<QStandardItem*> &items)
{
	if (!m_model)
	{
		return;
	}

	const int row = (currentIndex().row() + 1);

	if (items.count() > 0)
	{
		m_model->insertRow(row, items);
	}
	else
	{
		m_model->insertRow(row);
	}

	setCurrentIndex(getIndex(row, 0));

	m_isModified = true;

	emit modified();
}

void ItemViewWidget::insertRow(QStandardItem *item)
{
	QList<QStandardItem*> items;
	items.append(item);

	insertRow(items);
}

void ItemViewWidget::removeRow()
{
	if (!m_model)
	{
		return;
	}

	const int row = currentIndex().row();

	if (row >= 0)
	{
		m_model->removeRow(row);

		m_isModified = true;

		emit modified();
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

void ItemViewWidget::notifySelectionChanged()
{
	if (m_model)
	{
		emit canMoveUpChanged(canMoveUp());
		emit canMoveDownChanged(canMoveDown());
		emit needsActionsUpdate();
	}
}

void ItemViewWidget::updateDropSelection()
{
	setCurrentIndex(getIndex(qBound(0, m_dropRow, getRowCount()), 0));

	m_dropRow = -1;
}

void ItemViewWidget::setFilter(const QString filter)
{
	if (!m_model)
	{
		return;
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		bool found = filter.isEmpty();

		if (!found)
		{
			for (int j = 0; j < m_model->columnCount(); ++j)
			{
				QStandardItem *item = m_model->item(i, j);

				if (item && item->text().contains(filter, Qt::CaseInsensitive))
				{
					found = true;

					break;
				}
			}
		}

		setRowHidden(i, m_model->invisibleRootItem()->index(), !found);
	}
}

void ItemViewWidget::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (m_model)
	{
		m_model->setData(index, value, role);
	}
}

void ItemViewWidget::setModel(QAbstractItemModel *model)
{
	QTreeView::setModel(model);

	if (!model)
	{
		return;
	}

	model->setParent(this);

	if (model->inherits(QStringLiteral("QStandardItemModel").toLatin1()))
	{
		m_model = qobject_cast<QStandardItemModel*>(model);

		connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(notifySelectionChanged()));
	}

	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(notifySelectionChanged()));
	connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(modified()));
}

QStandardItemModel* ItemViewWidget::getModel()
{
	return m_model;
}

QStandardItem* ItemViewWidget::getItem(int row, int column) const
{
	return(m_model ? m_model->item(row, column) : NULL);
}

QModelIndex ItemViewWidget::getIndex(int row, int column) const
{
	return (m_model ? m_model->index(row, column) : QModelIndex());
}

QSize ItemViewWidget::sizeHint() const
{
	const QSize size = QTreeView::sizeHint();

	if (m_model && m_model->columnCount() == 1)
	{
		return QSize((sizeHintForColumn(0) + (frameWidth() * 2)), size.height());
	}

	return size;
}

int ItemViewWidget::getCurrentRow() const
{
	return (selectionModel()->hasSelection() ? currentIndex().row() : -1);
}

int ItemViewWidget::getRowCount() const
{
	return (m_model ? m_model->rowCount() : 0);
}

int ItemViewWidget::getColumnCount() const
{
	return (m_model ? m_model->columnCount() : 0);
}

bool ItemViewWidget::canMoveUp() const
{
	return (currentIndex().row() > 0 && m_model->rowCount() > 1);
}

bool ItemViewWidget::canMoveDown() const
{
	const int currentRow = currentIndex().row();

	return (currentRow >= 0 && m_model->rowCount() > 1 && currentRow < (m_model->rowCount() - 1));
}

bool ItemViewWidget::isModified() const
{
	return m_isModified;
}

}
