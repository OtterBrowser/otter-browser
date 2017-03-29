/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_MAINWINDOW_H
#define OTTER_MAINWINDOW_H

#include "../core/Application.h"
#include "../core/SessionsManager.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QMainWindow>

namespace Otter
{

namespace Ui
{
	class MainWindow;
}

class Action;
class MenuBarWidget;
class Shortcut;
class StatusBarWidget;
class TabBarWidget;
class TabSwitcherWidget;
class ToolBarWidget;
class WindowsManager;
class WorkspaceWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(const QVariantMap &parameters = QVariantMap(), const SessionMainWindow &session = SessionMainWindow(), QWidget *parent = nullptr);
	~MainWindow();

	static MainWindow* findMainWindow(QObject *parent);
	Action* getAction(int identifier);
	WorkspaceWidget* getWorkspace();
	TabBarWidget* getTabBar();
	WindowsManager* getWindowsManager();
	QVector<ToolBarWidget*> getToolBars(Qt::ToolBarArea area);
	bool areToolBarsVisible() const;
	bool isAboutToClose() const;
	bool isPrivate() const;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void openUrl(const QString &text = QString(), bool isPrivate = false);
	void storeWindowState();
	void restoreWindowState();
	void raiseWindow();

protected:
	void timerEvent(QTimerEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void beginToolBarDragging(bool isSidebar = false);
	void endToolBarDragging();
	bool event(QEvent *event) override;

protected slots:
	void triggerAction();
	void triggerAction(bool checked);
	void addBookmark(const QUrl &url = QUrl(), const QString &title = QString(), const QString &description = QString());
	void saveToolBarPositions();
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleToolBarAdded(int identifier);
	void handleToolBarModified(int identifier);
	void handleToolBarMoved(int identifier);
	void handleToolBarRemoved(int identifier);
	void handleTransferStarted();
	void updateWindowTitle(const QString &title);
	void updateShortcuts();
	void setCurrentWindow(Window *window);

private:
	WindowsManager *m_windowsManager;
	TabSwitcherWidget *m_tabSwitcher;
	WorkspaceWidget *m_workspace;
	TabBarWidget *m_tabBar;
	MenuBarWidget *m_menuBar;
	StatusBarWidget *m_statusBar;
	Window *m_currentWindow;
	QString m_currentBookmark;
	QString m_windowTitle;
	QVector<Action*> m_actions;
	QVector<Shortcut*> m_shortcuts;
	Qt::WindowStates m_previousState;
	Qt::WindowStates m_previousRaisedState;
	int m_mouseTrackerTimer;
	int m_tabSwitcherTimer;
	bool m_isAboutToClose;
	bool m_isDraggingToolBar;
	bool m_isPrivate;
	bool m_hasToolBars;
	Ui::MainWindow *m_ui;

signals:
	void activated(MainWindow *window);
	void areToolBarsVisibleChanged(bool areVisible);
	void statusMessageChanged(const QString &message);

friend class ToolBarDropZoneWidget;
friend class ToolBarWidget;
friend class WindowsManager;
};

}

#endif
