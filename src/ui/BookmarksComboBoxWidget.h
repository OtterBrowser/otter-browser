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

#ifndef OTTER_BOOKMARKSCOMBOBOXWIDGET_H
#define OTTER_BOOKMARKSCOMBOBOXWIDGET_H

#include "ComboBoxWidget.h"
#include "../core/BookmarksModel.h"

namespace Otter
{

class BookmarksComboBoxWidget final : public ComboBoxWidget
{
	Q_OBJECT

public:
	explicit BookmarksComboBoxWidget(QWidget *parent = nullptr);

	void setCurrentFolder(BookmarksModel::Bookmark *folder);
	void setMode(BookmarksModel::FormatMode mode);
	BookmarksModel::Bookmark* getCurrentFolder() const;

public slots:
	void createFolder();

protected slots:
	void handleLayoutChanged();
	void updateBranch(const QModelIndex &parent = {});

private:
	BookmarksModel *m_model;
};

}

#endif
