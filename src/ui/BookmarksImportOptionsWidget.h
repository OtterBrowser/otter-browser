/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_BOOKMARKSIMPORTOPTIONSWIDGET_H
#define OTTER_BOOKMARKSIMPORTOPTIONSWIDGET_H

#include "../core/BookmarksModel.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class BookmarksImportOptionsWidget;
}

class BookmarksImportOptionsWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit BookmarksImportOptionsWidget(QWidget *parent = nullptr);
	~BookmarksImportOptionsWidget();

	BookmarksModel::Bookmark* getTargetFolder() const;
	QString getSubfolderName() const;
	bool areDuplicatesAllowed() const;
	bool isImportingIntoSubfolder() const;
	bool hasToRemoveExisting() const;

protected slots:
	void handleRemoveExistingChanged(bool isChecked);
	void handleImportToSubfolderChanged(bool isChecked);

private:
	Ui::BookmarksImportOptionsWidget *m_ui;
};

}

#endif
