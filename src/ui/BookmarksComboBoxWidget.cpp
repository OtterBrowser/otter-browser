/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	m_model(BookmarksManager::getModel()),
	m_isIgnoringChanges(false)
{
	setModel(m_model);
	updateBranch();

	connect(m_model, &BookmarksModel::layoutChanged, this, [&]()
	{
		if (!m_isIgnoringChanges)
		{
			updateBranch();
		}
	});
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

void BookmarksComboBoxWidget::updateBranch(const QModelIndex &parent)
{
	for (int i = 0; i < m_model->rowCount(parent); ++i)
	{
		const QModelIndex index(m_model->index(i, 0, parent));

		if (!index.isValid())
		{
			continue;
		}

		switch (static_cast<BookmarksModel::BookmarkType>(index.data(BookmarksModel::TypeRole).toInt()))
		{
			case BookmarksModel::RootBookmark:
			case BookmarksModel::FolderBookmark:
				updateBranch(index);

				break;
			default:
				getView()->setRowHidden(i, parent, true);

				break;
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

		getView()->expand(folder->index());
	}
}

void BookmarksComboBoxWidget::setMode(BookmarksModel::FormatMode mode)
{
	m_isIgnoringChanges = true;
	m_model = ((mode == BookmarksModel::NotesMode) ? NotesManager::getModel() : BookmarksManager::getModel());

	setModel(m_model);
	updateBranch();

	m_isIgnoringChanges = false;
}

BookmarksModel::Bookmark* BookmarksComboBoxWidget::getCurrentFolder() const
{
	BookmarksModel::Bookmark *item(m_model->getBookmark(currentData(BookmarksModel::IdentifierRole).toULongLong()));

	return (item ? item : m_model->getRootItem());
}

}
