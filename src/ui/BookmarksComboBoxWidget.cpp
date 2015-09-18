/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksComboBoxWidget.h"
#include "../core/BookmarksManager.h"
#include "../core/NotesManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QInputDialog>

namespace Otter
{

BookmarksComboBoxWidget::BookmarksComboBoxWidget(QWidget *parent) : QComboBox(parent),
	m_view(new QTreeView(this)),
	m_mode(BookmarksModel::BookmarksMode)
{
	setView(m_view);
	setModel(BookmarksManager::getModel());
	updateBranch();

	m_view->viewport()->installEventFilter(this);
	m_view->setHeaderHidden(true);
	m_view->setItemsExpandable(false);
	m_view->setRootIsDecorated(false);
	m_view->setStyleSheet(QLatin1String("QTreeView::branch { border-image:url(invalid.png); }"));

	connect(model(), SIGNAL(layoutChanged()), this, SLOT(updateBranch()));
}

void BookmarksComboBoxWidget::createFolder()
{
	const QString title = QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:"));

	if (!title.isEmpty())
	{
		switch (m_mode)
		{
			case BookmarksModel::BookmarksMode:
				setCurrentFolder(BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, QUrl(), title, getCurrentFolder()));

				break;
			case BookmarksModel::NotesMode:
				setCurrentFolder(NotesManager::addNote(BookmarksModel::FolderBookmark, QUrl(), title, getCurrentFolder()));

				break;
			default:
				break;
		}
	}
}

void BookmarksComboBoxWidget::updateBranch(QStandardItem *branch)
{
	if (!branch)
	{
		branch = qobject_cast<BookmarksModel*>(model())->invisibleRootItem();
	}

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i, 0);

		if (item)
		{
			const BookmarksModel::BookmarkType type = static_cast<BookmarksModel::BookmarkType>(item->data(BookmarksModel::TypeRole).toInt());

			if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark)
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

void BookmarksComboBoxWidget::setCurrentFolder(BookmarksItem *folder)
{
	m_index = (folder ? folder->index() : QModelIndex());

	setRootModelIndex(m_index.parent());
	setModelColumn(0);
	setCurrentIndex(m_index.row());
	setRootModelIndex(QModelIndex());
}

void BookmarksComboBoxWidget::setMode(BookmarksModel::FormatMode mode)
{
	disconnect(model(), SIGNAL(layoutChanged()), this, SLOT(updateBranch()));

	m_mode = mode;
	m_index = QModelIndex();

	switch (mode)
	{
		case BookmarksModel::BookmarksMode:
			setModel(BookmarksManager::getModel());

			break;
		case BookmarksModel::NotesMode:
			setModel(NotesManager::getModel());

			break;
		default:
			break;
	}

	updateBranch();

	connect(model(), SIGNAL(layoutChanged()), this, SLOT(updateBranch()));
}

BookmarksItem* BookmarksComboBoxWidget::getCurrentFolder()
{
	BookmarksItem *item = qobject_cast<BookmarksModel*>(model())->bookmarkFromIndex(m_index);

	return (item ? item :qobject_cast<BookmarksModel*>(model())->getRootItem());
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
