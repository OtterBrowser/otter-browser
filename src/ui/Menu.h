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

#ifndef OTTER_MENU_H
#define OTTER_MENU_H

#include <QtCore/QJsonObject>
#include <QtWidgets/QMenu>

namespace Otter
{

enum MenuRole
{
	NoMenuRole = 0,
	BookmarksMenuRole = 1,
	BookmarkSelectorMenuRole = 2,
	CharacterEncodingMenuRole = 3,
	ClosedWindowsMenu = 4,
	ImportExportMenuRole = 5,
	SessionsMenuRole = 6,
	UserAgentMenuRole = 7
};

class Action;
class BookmarksItem;

class Menu : public QMenu
{
	Q_OBJECT

public:
	explicit Menu(QWidget *parent = NULL);

	void load(const QJsonObject &definition);
	void setRole(MenuRole role);
	Action* addAction(int identifier);

protected:
	void changeEvent(QEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

protected slots:
	void populateBookmarksMenu();
	void populateCharacterEncodingMenu();
	void populateClosedWindowsMenu();
	void populateSessionsMenu();
	void populateUserAgentMenu();
	void clearBookmarksMenu();
	void clearClosedWindows();
	void restoreClosedWindow();
	void openBookmark();
	void openImporter(QAction *action);
	void openSession(QAction *action);
	void selectCharacterEncoding(QAction *action);
	void selectUserAgent(QAction *action);
	void updateClosedWindowsMenu();

private:
	QActionGroup *m_actionGroup;
	BookmarksItem *m_bookmark;
	QString m_title;
	MenuRole m_role;
};

}

#endif
