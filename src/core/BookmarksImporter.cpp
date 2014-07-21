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

#include "BookmarksImporter.h"
#include "BookmarksManager.h"

namespace Otter
{

BookmarksImporter::BookmarksImporter(QObject *parent): Importer(parent),
	m_baseFolder(NULL),
	m_currentFolder(NULL)
{
}

void BookmarksImporter::addUrl(BookmarkInformation bookmark, bool duplicate)
{
	bookmark.type = UrlBookmark;

	if (duplicate || !BookmarksManager::hasBookmark(bookmark.url))
	{
		addBookmark(bookmark);
	}
}

void BookmarksImporter::addSeparator(BookmarkInformation bookmark)
{
	bookmark.type = SeparatorBookmark;

	addBookmark(bookmark);
}

void BookmarksImporter::enterNewFolder(BookmarkInformation bookmark)
{
	bookmark.type = FolderBookmark;

	m_currentFolder = addBookmark(bookmark);
}

void BookmarksImporter::goToParent()
{
	if (m_currentFolder == m_baseFolder)
	{
		return;
	}

	if (m_currentFolder)
	{
		m_currentFolder = BookmarksManager::getBookmark(m_currentFolder->parent);
	}
	else
	{
		m_currentFolder = BookmarksManager::getBookmark(0);
	}
}

void BookmarksImporter::setImportFolder(BookmarkInformation *folder)
{
	m_baseFolder = folder;
	m_currentFolder = folder;
}

BookmarkInformation* BookmarksImporter::addBookmark(BookmarkInformation bookmark)
{
	BookmarkInformation *added = new BookmarkInformation(bookmark);

	if (BookmarksManager::getBookmark(added->keyword))
	{
		added->keyword = QString();
	}

	if (m_currentFolder)
	{
		BookmarksManager::addBookmark(added, m_currentFolder->identifier);
	}
	else
	{
		BookmarksManager::addBookmark(added);
	}

	return added;
}

ImportType BookmarksImporter::getType() const
{
	return BookmarksImport;
}

}
