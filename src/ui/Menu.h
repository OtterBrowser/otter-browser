/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_MENU_H
#define OTTER_MENU_H

#include "../core/ActionsManager.h"

#include <QtCore/QJsonObject>
#include <QtWidgets/QMenu>

namespace Otter
{

class BookmarksItem;

class Menu : public QMenu
{
	Q_OBJECT
	Q_ENUMS(MenuRole)

public:
	enum MenuRole
	{
		NoMenuRole = 0,
		BookmarksMenuRole,
		BookmarkSelectorMenuRole,
		NotesMenuRole,
		CharacterEncodingMenuRole,
		ClosedWindowsMenuRole,
		DictionariesMenuRole,
		ImportExportMenuRole,
		OpenInApplicationMenuRole,
		ProxyMenuRole,
		SearchMenuRole,
		SessionsMenuRole,
		StyleSheetsMenuRole,
		ToolBarsMenuRole,
		UserAgentMenuRole,
		ValidateMenuRole,
		WindowsMenuRole
	};

	explicit Menu(int role = NoMenuRole, QWidget *parent = nullptr);

	void load(const QString &path, const QStringList &includeSections = {}, ActionExecutor::Object executor = ActionExecutor::Object());
	void load(const QJsonObject &definition, const QStringList &includeSections = {}, ActionExecutor::Object executor = ActionExecutor::Object());
	void load(int option);
	void setTitle(const QString &title);
	void setExecutor(ActionExecutor::Object executor);
	void setActionParameters(const QVariantMap &parameters);
	void setMenuOptions(const QVariantMap &options);
	int getRole() const;
	static int getMenuRoleIdentifier(const QString &name);

protected:
	void changeEvent(QEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void appendAction(const QJsonValue &definition, const QStringList &includeSections, ActionExecutor::Object executor);
	ActionExecutor::Object getExecutor();
	bool canInclude(const QJsonObject &definition, const QStringList &includeSections);

protected slots:
	void populateBookmarksMenu();
	void populateOptionMenu();
	void populateCharacterEncodingMenu();
	void populateClosedWindowsMenu();
	void populateDictionariesMenu();
	void populateNotesMenu();
	void populateOpenInApplicationMenu();
	void populateProxiesMenu();
	void populateSearchMenu();
	void populateSessionsMenu();
	void populateStyleSheetsMenu();
	void populateToolBarsMenu();
	void populateUserAgentMenu();
	void populateWindowsMenu();
	void clearBookmarksMenu();
	void clearClosedWindows();
	void clearNotesMenu();
	void openBookmark();
	void openImporter(QAction *action);
	void openSession(QAction *action);
	void selectDictionary(QAction *action);
	void selectOption(QAction *action);
	void selectStyleSheet(QAction *action);
	void updateClosedWindowsMenu();

private:
	QActionGroup *m_actionGroup;
	QAction *m_clickedAction;
	BookmarksItem *m_bookmark;
	QString m_title;
	ActionExecutor::Object m_executor;
	QHash<QString, QActionGroup*> m_actionGroups;
	QVariantMap m_actionParameters;
	QVariantMap m_menuOptions;
	int m_role;
	int m_option;

	static int m_menuRoleIdentifierEnumerator;
};

}

#endif
