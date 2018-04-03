/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_BOOKMARKSIMPORTER_H
#define OTTER_BOOKMARKSIMPORTER_H

#include "BookmarksModel.h"
#include "Importer.h"

namespace Otter
{

class BookmarksImporter : public Importer
{
	Q_OBJECT

public:
	explicit BookmarksImporter(QObject *parent = nullptr);

	ImportType getImportType() const override;

protected:
	void goToParent();
	void removeAllBookmarks();
	void setAllowDuplicates(bool allow);
	void setCurrentFolder(BookmarksModel::Bookmark *folder);
	void setImportFolder(BookmarksModel::Bookmark *folder);
	BookmarksModel::Bookmark* getCurrentFolder() const;
	BookmarksModel::Bookmark* getImportFolder() const;
	bool areDuplicatesAllowed() const;

private:
	BookmarksModel::Bookmark *m_currentFolder;
	BookmarksModel::Bookmark *m_importFolder;
	bool m_areDuplicatesAllowed;
};

}

#endif
