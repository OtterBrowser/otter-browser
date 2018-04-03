/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksComboBoxWidget.h"
#include "ItemViewWidget.h"
#include "../core/BookmarksManager.h"
#include "../core/NotesManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QInputDialog>

namespace Otter
{

BookmarksComboBoxWidget::BookmarksComboBoxWidget(QWidget *parent) : ComboBoxWidget(parent),
	m_model(BookmarksManager::getModel())
{
	setModel(m_model);
	updateBranch();

	connect(m_model, &BookmarksModel::layoutChanged, this, &BookmarksComboBoxWidget::handleLayoutChanged);
}

void BookmarksComboBoxWidget::createFolder()
{
	const QString title(QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:")));

	if (!title.isEmpty())
	{
		switch (m_model->getFormatMode())
		{
			case BookmarksModel::BookmarksMode:
				setCurrentFolder(BookmarksManager::addBookmark(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, title}}, getCurrentFolder()));

				break;
			case BookmarksModel::NotesMode:
				setCurrentFolder(NotesManager::addNote(BookmarksModel::FolderBookmark, {{BookmarksModel::TitleRole, title}}, getCurrentFolder()));

				break;
			default:
				break;
		}
	}
}

void BookmarksComboBoxWidget::handleLayoutChanged()
{
	updateBranch();
}

void BookmarksComboBoxWidget::updateBranch(const QModelIndex &parent)
{
	for (int i = 0; i < m_model->rowCount(parent); ++i)
	{
		const QModelIndex index(m_model->index(i, 0, parent));

		if (index.isValid())
		{
			const BookmarksModel::BookmarkType type(static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()));

			if (type == BookmarksModel::RootBookmark || type == BookmarksModel::FolderBookmark)
			{
				updateBranch(index);
			}
			else
			{
				getView()->setRowHidden(i, parent, true);
			}
		}
	}

	if (!parent.isValid())
	{
		getView()->expandAll();
	}
}

void BookmarksComboBoxWidget::setCurrentFolder(BookmarksModel::Bookmark *folder)
{
	if (folder)
	{
		setCurrentIndex(folder->index());
	}
}

void BookmarksComboBoxWidget::setMode(BookmarksModel::FormatMode mode)
{
	disconnect(m_model, &BookmarksModel::layoutChanged, this, &BookmarksComboBoxWidget::handleLayoutChanged);

	m_model = ((mode == BookmarksModel::NotesMode) ? NotesManager::getModel() : BookmarksManager::getModel());

	setModel(m_model);
	updateBranch();

	connect(m_model, &BookmarksModel::layoutChanged, this, &BookmarksComboBoxWidget::handleLayoutChanged);
}

BookmarksModel::Bookmark* BookmarksComboBoxWidget::getCurrentFolder() const
{
	BookmarksModel::Bookmark *item(m_model->getBookmark(currentData(BookmarksModel::IdentifierRole).toULongLong()));

	return (item ? item : m_model->getRootItem());
}

}
