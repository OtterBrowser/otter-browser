/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

class BookmarksItem;

class BookmarkPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarkPropertiesDialog(BookmarksItem *bookmark, QStandardItem *folder = NULL, QWidget *parent = NULL);
	~BookmarkPropertiesDialog();

	void setReadOnly(bool readOnly);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void saveBookmark();

private:
	BookmarksItem *m_bookmark;
	QStandardItemModel *m_model;
	Ui::BookmarkPropertiesDialog *m_ui;
};

}

#endif
