/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "InlineListWidget.h"
#include "../core/ThemesManager.h"

#include <QtGui/QPainter>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>

namespace Otter
{

ListEntryDelegate::ListEntryDelegate(QWidget *parent) : QStyledItemDelegate(parent),
	m_parent(parent)
{
}

void ListEntryDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItem mutableOption(option);
	mutableOption.backgroundBrush = option.palette.brush((index.column() % 2) ? QPalette::Base : QPalette::AlternateBase);

	painter->fillRect(option.rect, mutableOption.backgroundBrush);

	QStyledItemDelegate::paint(painter, mutableOption, index);
}

QSize ListEntryDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const QFontMetrics fontMetrics(index.data(Qt::FontRole).isValid() ? QFontMetrics(index.data(Qt::FontRole).value<QFont>()) : option.fontMetrics);

	return {qMin(100, fontMetrics.boundingRect(index.data().toString()).width()), qMax(m_parent->height(), qRound(fontMetrics.lineSpacing() * 1.25))};
}

InlineListWidget::InlineListWidget(QWidget *parent) : QWidget(parent),
	m_treeView(new QTreeView(this)),
	m_addButton(new QToolButton(this)),
	m_removeButton(new QToolButton(this)),
	m_model(new QStandardItemModel(this))
{
	m_addButton->setToolTip(tr("Add"));
	m_addButton->setIcon(ThemesManager::createIcon(QLatin1String("list-add")));
	m_addButton->setAutoRaise(true);

	m_removeButton->setToolTip(tr("Remove"));
	m_removeButton->setIcon(ThemesManager::createIcon(QLatin1String("list-remove")));
	m_removeButton->setAutoRaise(true);
	m_removeButton->setEnabled(false);

	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_treeView);
	layout->addWidget(m_addButton);
	layout->addWidget(m_removeButton);

	m_treeView->setModel(m_model);
	m_treeView->setEditTriggers(QAbstractItemView::CurrentChanged);
	m_treeView->setHeaderHidden(true);
	m_treeView->setIndentation(0);
	m_treeView->setItemDelegate(new ListEntryDelegate(m_treeView));
	m_treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_treeView->header()->setStretchLastSection(false);

	setMaximumHeight(qMax(m_treeView->sizeHintForRow(0), m_addButton->height()));

	connect(m_model, &QAbstractItemModel::dataChanged, this, &InlineListWidget::modified);
	connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [&]()
	{
		m_removeButton->setEnabled(m_treeView->currentIndex().isValid());
	});
	connect(m_addButton, &QToolButton::clicked, this, [&]()
	{
		const int column(m_treeView->currentIndex().isValid() ? (m_treeView->currentIndex().column() + 1) : m_model->columnCount());

		m_model->insertColumn(column, {new QStandardItem()});

		m_treeView->setCurrentIndex(m_model->index(0, column));

		emit modified();
	});
	connect(m_removeButton, &QToolButton::clicked, this, [&]()
	{
		m_model->removeColumn(m_treeView->currentIndex().column());

		emit modified();
	});
}

void InlineListWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::ApplicationFontChange:
		case QEvent::StyleChange:
			setMaximumHeight(qMax(m_treeView->sizeHintForRow(0), m_addButton->height()));

			break;
		case QEvent::LanguageChange:
			m_addButton->setToolTip(tr("Add"));
			m_removeButton->setToolTip(tr("Remove"));

			break;
		default:
			break;
	}
}

void InlineListWidget::setValues(const QStringList &values)
{
	QList<QStandardItem*> items;
	items.reserve(values.count());

	disconnect(m_model, &QAbstractItemModel::dataChanged, this, &InlineListWidget::modified);

	m_model->clear();

	for (int i = 0; i < values.count(); ++i)
	{
		items.append(new QStandardItem(values.at(i)));
	}

	m_model->appendRow(items);

	for (int i = 0; i < values.count(); ++i)
	{
		m_treeView->resizeColumnToContents(i);
	}

	setMaximumHeight(qMax(m_treeView->sizeHintForRow(0), m_addButton->height()));

	connect(m_model, &QAbstractItemModel::dataChanged, this, &InlineListWidget::modified);
}

QStringList InlineListWidget::getValues() const
{
	QStringList values;
	values.reserve(m_model->columnCount());

	for (int i = 0; i < m_model->columnCount(); ++i)
	{
		values.append(m_model->index(0, i).data().toString());
	}

	return values;
}

}
