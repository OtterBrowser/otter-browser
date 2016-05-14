/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_BOOKMARKSCOMBOBOXWIDGET_H
#define OTTER_BOOKMARKSCOMBOBOXWIDGET_H

#include "../core/BookmarksModel.h"

#include <QtGui/QStandardItem>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTreeView>

namespace Otter
{

class BookmarksItem;

class BookmarksComboBoxWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit BookmarksComboBoxWidget(QWidget *parent = NULL);

	void setCurrentFolder(BookmarksItem *folder);
	void setMode(BookmarksModel::FormatMode mode);
	BookmarksItem* getCurrentFolder();

protected slots:
	void createFolder();
	void updateBranch(QStandardItem *branch = NULL);

private:
	QTreeView *m_view;
	BookmarksModel::FormatMode m_mode;
};

}

#endif
