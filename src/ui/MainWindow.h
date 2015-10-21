/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_MAINWINDOW_H
#define OTTER_MAINWINDOW_H

#include "SidebarWidget.h"
#include "toolbars/ActionWidget.h"
#include "../core/SessionsManager.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QShortcut>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

namespace Otter
{

namespace Ui
{
	class MainWindow;
}

class ActionsManager;
class WorkspaceWidget;
class MenuBarWidget;
class StatusBarWidget;
class TabBarWidget;
class TabSwitcherWidget;
class ToolBarAreaWidget;
class ToolBarWidget;
class WindowsManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(bool isPrivate = false, const SessionMainWindow &session = SessionMainWindow(), QWidget *parent = NULL);
	~MainWindow();

	static MainWindow* findMainWindow(QObject *parent);
	Action* getAction(int identifier);
	WorkspaceWidget* getWorkspace();
	TabBarWidget* getTabBar();
	WindowsManager* getWindowsManager();
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void openUrl(const QString &text = QString());
	void storeWindowState();
	void restoreWindowState();

protected:
	void timerEvent(QTimerEvent *event);
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void startToolBarDragging();
	void endToolBarDragging();
	void moveToolBar(ToolBarWidget *toolBar, Qt::ToolBarArea area);
	void placeSidebars();
	void updateSidebars();
	void setTabBar(TabBarWidget *tabBar);
	bool event(QEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void triggerAction();
	void triggerAction(bool checked);
	void addBookmark(const QUrl &url = QUrl(), const QString &title = QString(), const QString &description = QString(), bool warn = false);
	void editBookmark(const QUrl &url);
	void toolBarModified(int identifier);
	void transferStarted();
	void updateWindowTitle(const QString &title);
	void updateShortcuts();
	void setCurrentWindow(Window *window);

private:
	WindowsManager *m_windowsManager;
	TabSwitcherWidget *m_tabSwitcher;
	WorkspaceWidget *m_workspace;
	ToolBarAreaWidget *m_topToolBarArea;
	ToolBarAreaWidget *m_bottomToolBarArea;
	ToolBarAreaWidget *m_leftToolBarArea;
	ToolBarAreaWidget *m_rightToolBarArea;
	TabBarWidget *m_tabBar;
	MenuBarWidget *m_menuBar;
	StatusBarWidget *m_statusBar;
	ActionWidget *m_sidebarToggle;
	SidebarWidget *m_sidebar;
	QSplitter *m_splitter;
	Window *m_currentWindow;
	QString m_currentBookmark;
	QString m_windowTitle;
	QVector<Action*> m_standardActions;
	QVector<QPair<int, QVector<QShortcut*> > > m_actionShortcuts;
	Qt::WindowStates m_previousState;
	int m_tabSwitcherKey;
	int m_tabSwictherTimer;
	Ui::MainWindow *m_ui;

signals:
	void requestedNewWindow(bool isPrivate = false, bool inBackground = false, const QUrl &url = QUrl());
	void controlsHiddenChanged(bool hidden);
	void statusMessageChanged(const QString &message);

friend class ToolBarAreaWidget;
friend class WindowsManager;
};

}

#endif
