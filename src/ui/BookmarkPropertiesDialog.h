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

#ifndef OTTER_BOOKMARKPROPERTIESDIALOG_H
#define OTTER_BOOKMARKPROPERTIESDIALOG_H

#include "../core/BookmarksManager.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class BookmarkPropertiesDialog;
}

class BookmarkPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarkPropertiesDialog(BookmarkInformation *bookmark, int folder = -1, QWidget *parent = NULL);
	~BookmarkPropertiesDialog();

protected:
	void changeEvent(QEvent *event);
	void populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent);

protected slots:
	void folderChanged(const QModelIndex &index);
	void createFolder();
	void reloadFolders();
	void saveBookmark();

private:
	BookmarkInformation *m_bookmark;
	QStandardItemModel *m_model;
	QModelIndex m_index;
	int m_folder;
	Ui::BookmarkPropertiesDialog *m_ui;
};

}

#endif
