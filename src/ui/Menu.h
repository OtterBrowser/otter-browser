/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "../core/ActionExecutor.h"
#include "../ui/Action.h"

#include <QtCore/QJsonObject>
#if QT_VERSION >= 0x060000
#include <QtGui/QActionGroup>
#endif
#include <QtWidgets/QMenu>

namespace Otter
{

class MainWindow;
class Window;
class WebWidget;

class Menu : public QMenu
{
	Q_OBJECT

public:
	enum MenuRole
	{
		UnknownMenu = 0,
		BookmarkMenu,
		BookmarksMenu,
		BookmarkSelectorMenu,
		FeedsMenu,
		NotesMenu,
		CharacterEncodingMenu,
		ClosedWindowsMenu,
		DictionariesMenu,
		ImportExportMenu,
		OpenInApplicationMenu,
		ProxyMenu,
		SearchMenu,
		SessionsMenu,
		SpellCheckSuggestionsMenu,
		StyleSheetsMenu,
		ToolBarsMenu,
		UserAgentMenu,
		ValidateMenu,
		WindowsMenu
	};

	Q_ENUM(MenuRole)

	explicit Menu(QWidget *parent = nullptr);
	explicit Menu(int role, QWidget *parent);

	void load(const QString &path, const QStringList &includeSections = {}, const ActionExecutor::Object &executor = ActionExecutor::Object());
	void load(const QJsonObject &definition, const QStringList &includeSections = {}, ActionExecutor::Object executor = ActionExecutor::Object());
	void load(int option);
	void setTitle(const QString &title);
	void setExecutor(const ActionExecutor::Object &executor);
	void setActionParameters(const QVariantMap &parameters);
	void setMenuOptions(const QVariantMap &options);
	int getRole() const;
	static int getMenuRoleIdentifier(const QString &name);

protected:
	struct MenuContext final
	{
		MainWindow *mainWindow = nullptr;
		Window *window = nullptr;
		WebWidget *webWidget = nullptr;
	};

	void changeEvent(QEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void appendAction(const QJsonValue &definition, const QStringList &sections, const ActionExecutor::Object &executor);
	ActionExecutor::Object getExecutor() const;
	MenuContext getMenuContext() const;
	bool canInclude(const QJsonObject &definition, const QStringList &sections);
	bool hasIncludeMatch(const QJsonObject &definition, const QString &key, const QStringList &sections);

protected slots:
	void hideMenu();
	void populateBookmarksMenu();
	void populateBookmarkSelectorMenu();
	void populateOptionMenu();
	void populateCharacterEncodingMenu();
	void populateClosedWindowsMenu();
	void populateDictionariesMenu();
	void populateFeedsMenu();
	void populateNotesMenu();
	void populateOpenInApplicationMenu();
	void populateProxiesMenu();
	void populateSearchEnginesMenu();
	void populateSessionsMenu();
	void populateSpellCheckSuggestionsMenu();
	void populateStyleSheetsMenu();
	void populateToolBarsMenu();
	void populateUserAgentMenu();
	void populateWindowsMenu();
	void clearBookmarksMenu();
	void clearNotesMenu();
	void selectOption(QAction *action);
	void updateClosedWindowsMenu();

private:
	QActionGroup *m_actionGroup;
	QAction *m_clickedAction;
	QString m_title;
	ActionExecutor::Object m_executor;
	QHash<QString, QActionGroup*> m_actionGroups;
	QVariantMap m_actionParameters;
	QVariantMap m_menuOptions;
	int m_role;
	int m_option;

	static int m_menuRoleIdentifierEnumerator;
};

class MenuAction : public Action
{
	Q_OBJECT

public:
	explicit MenuAction(const QString &text, bool isTranslateable, QMenu *parent);
	explicit MenuAction(int identifier, const QVariantMap &parameters, const ActionExecutor::Object &executor, QMenu *parent);

	virtual QMenu* createContextMenu(QWidget *parent = nullptr) const;
	virtual bool hasContextMenu() const;

protected slots:
	void setState(const ActionsManager::ActionDefinition::State &state) override;

private:
	QMenu *m_menu;
};

class OpenBookmarkMenuAction final : public MenuAction
{
public:
	explicit OpenBookmarkMenuAction(quint64 bookmark, const ActionExecutor::Object &executor, QMenu *parent);

	QMenu* createContextMenu(QWidget *parent = nullptr) const override;
	bool hasContextMenu() const override;
};

}

#endif
