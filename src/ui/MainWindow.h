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

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

namespace Otter
{

namespace Ui
{
	class MainWindow;
}

class ActionsManager;
class MdiWidget;
class MenuBarWidget;
class StatusBarWidget;
class TabBarWidget;
class TabSwitcherWidget;
class ToolBarWidget;
class WindowsManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(bool isPrivate = false, const SessionMainWindow &session = SessionMainWindow(), QWidget *parent = NULL);
	~MainWindow();

	static MainWindow* findMainWindow(QObject *parent);
	MdiWidget* getMdi();
	TabBarWidget* getTabBar();
	ActionsManager* getActionsManager();
	WindowsManager* getWindowsManager();
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(int identifier, bool checked = false);
	void openUrl(const QString &text = QString());
	void storeWindowState();
	void restoreWindowState();

protected:
	void timerEvent(QTimerEvent *event);
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void createSidebar();
	void createToggleEdge();
	void placeSidebars();
	void updateSidebars();
	bool event(QEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void addBookmark(const QUrl &url = QUrl(), const QString &title = QString(), const QString &description = QString(), bool warn = false);
	void addToolBar(int identifier);
	void toolBarModified(int identifier);
	void splitterMoved();
	void transferStarted();
	void updateWindowTitle(const QString &title);

private:
	ActionsManager *m_actionsManager;
	WindowsManager *m_windowsManager;
	TabSwitcherWidget *m_tabSwitcher;
	MdiWidget *m_mdiWidget;
	TabBarWidget *m_tabBar;
	ToolBarWidget *m_tabBarToolBar;
	MenuBarWidget *m_menuBar;
	StatusBarWidget *m_statusBar;
	ActionWidget *m_toggleEdge;
	SidebarWidget *m_sidebarWidget;
	QSplitter *m_splitter;
	QString m_currentBookmark;
	Qt::WindowStates m_previousState;
	int m_tabSwitcherKey;
	int m_tabSwictherTimer;
	Ui::MainWindow *m_ui;

signals:
	void requestedNewWindow(bool isPrivate = false, bool inBackground = false, const QUrl &url = QUrl());
	void controlsHiddenChanged(bool hidden);
	void statusMessageChanged(const QString &message);
};

}

#endif
