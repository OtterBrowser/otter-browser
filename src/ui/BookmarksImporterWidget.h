/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_BOOKMARKSIMPORTERWIDGET_H
#define OTTER_BOOKMARKSIMPORTERWIDGET_H

#include "../core/BookmarksManager.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class BookmarksImporterWidget;
}

class BookmarksImporterWidget : public QWidget
{
	Q_OBJECT

public:
	explicit BookmarksImporterWidget(QWidget *parent = 0);
	~BookmarksImporterWidget();

	QString subfolderName();
	int targetFolder();
	bool duplicate();
	bool importIntoSubfolder();
	bool removeExisting();

protected:
	void populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent);

protected slots:
	void folderChanged(const QModelIndex &index);
	void reloadFolders();
	void removeStateChanged(bool checked);
	void toSubfolderChanged(bool checked);

private:
	QStandardItemModel *m_model;
	QModelIndex m_index;
	int m_folder;
	Ui::BookmarksImporterWidget *m_ui;
};

}

#endif
