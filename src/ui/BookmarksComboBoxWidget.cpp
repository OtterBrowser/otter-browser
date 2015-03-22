/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksComboBoxWidget.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QInputDialog>

namespace Otter
{

BookmarksComboBoxWidget::BookmarksComboBoxWidget(QWidget *parent) : QComboBox(parent),
	m_view(new QTreeView(this))
{
	setView(m_view);
	setModel(BookmarksManager::getModel());
	updateBranch();

	m_view->viewport()->installEventFilter(this);
	m_view->setHeaderHidden(true);
	m_view->setItemsExpandable(false);
	m_view->setRootIsDecorated(false);
	m_view->setStyleSheet(QLatin1String("QTreeView::branch { border-image:url(invalid.png); }"));

	connect(BookmarksManager::getModel(), SIGNAL(layoutChanged()), this, SLOT(updateBranch()));
}

void BookmarksComboBoxWidget::createFolder()
{
	const QString title = QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:"));

	if (title.isEmpty())
	{
		return;
	}

	BookmarksItem *item = new BookmarksItem(BookmarksItem::FolderBookmark, 0, QUrl(), title);

	getCurrentFolder()->appendRow(item);
	setCurrentFolder(item->index());
}

void BookmarksComboBoxWidget::updateBranch(QStandardItem *branch)
{
	if (!branch)
	{
		branch = BookmarksManager::getModel()->invisibleRootItem();
	}

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i, 0);

		if (item)
		{
			const BookmarksItem::BookmarkType type = static_cast<BookmarksItem::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

			if (type == BookmarksItem::RootBookmark || type == BookmarksItem::FolderBookmark)
			{
				updateBranch(item);
			}
			else
			{
				m_view->setRowHidden(i, branch->index(), true);
			}
		}
	}

	m_view->expandAll();
}

void BookmarksComboBoxWidget::setCurrentFolder(const QModelIndex &index)
{
	m_index = index;

	setRootModelIndex(index.parent());
	setModelColumn(0);
	setCurrentIndex(index.row());
	setRootModelIndex(QModelIndex());
}

QStandardItem* BookmarksComboBoxWidget::getCurrentFolder()
{
	QStandardItem *item = BookmarksManager::getModel()->itemFromIndex(m_index);

	if (item)
	{
		return item;
	}

	return BookmarksManager::getModel()->getRootItem();
}

bool BookmarksComboBoxWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_view->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);

		if (mouseEvent)
		{
			m_index = m_view->indexAt(mouseEvent->pos());
		}
	}

	return QComboBox::eventFilter(object, event);
}

}
