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

class Action;
class BookmarksItem;

class Menu : public QMenu
{
	Q_OBJECT

public:
	enum MenuRole
	{
		NoMenuRole = 0,
		BookmarksMenuRole = 1,
		BookmarkSelectorMenuRole = 2,
		NotesMenuRole = 3,
		CharacterEncodingMenuRole = 5,
		ClosedWindowsMenu = 6,
		ImportExportMenuRole = 7,
		SessionsMenuRole = 8,
		ToolBarsMenuRole = 9,
		UserAgentMenuRole = 10
	};

	explicit Menu(MenuRole role = NoMenuRole, QWidget *parent = NULL);

	void load(const QJsonObject &definition);
	Action* addAction(int identifier, bool isGlobal = false);
	MenuRole getRole() const;
	static MenuRole getRole(const QString &identifier);

protected:
	void changeEvent(QEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

protected slots:
	void populateModelMenu();
	void populateCharacterEncodingMenu();
	void populateClosedWindowsMenu();
	void populateSessionsMenu();
	void populateToolBarsMenu();
	void populateUserAgentMenu();
	void clearModelMenu();
	void clearClosedWindows();
	void restoreClosedWindow();
	void openBookmark();
	void openImporter(QAction *action);
	void openSession(QAction *action);
	void selectCharacterEncoding(QAction *action);
	void selectUserAgent(QAction *action);
	void updateClosedWindowsMenu();
	void setToolBarVisibility(bool visible);

private:
	QActionGroup *m_actionGroup;
	BookmarksItem *m_bookmark;
	QString m_title;
	MenuRole m_role;
};

}

#endif
