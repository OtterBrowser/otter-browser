/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "../core/SessionsManager.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QActionGroup>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMainWindow>

namespace Otter
{

namespace Ui
{
	class MainWindow;
}

class ActionsManager;
class Menu;
class WindowsManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(bool isPrivate = false, const SessionMainWindow &windows = SessionMainWindow(), QWidget *parent = NULL);
	~MainWindow();

	ActionsManager* getActionsManager();
	WindowsManager* getWindowsManager();
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void openUrl(const QString &input = QString());
	void storeWindowState();
	void restoreWindowState();

protected:
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	void createMenuBar();
	void updateAction(QAction *source, QAction *target);
	Menu* getMenu(const QString &identifier);
	bool event(QEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void actionNewTabPrivate();
	void actionNewWindowPrivate();
	void actionOpen();
	void actionSaveSession();
	void actionManageSessions();
	void actionSession(QAction *action);
	void actionImport(QAction *action);
	void actionWorkOffline(bool enabled);
	void actionShowMenuBar(bool enable);
	void actionFullScreen();
	void actionUserAgent(QAction *action);
	void actionCharacterEncoding(QAction *action);
	void actionClearClosedWindows();
	void actionRestoreClosedWindow();
	void actionViewHistory();
	void actionClearHistory();
	void actionAddBookmark(const QUrl &url = QUrl(), const QString &title = QString());
	void actionManageBookmarks();
	void actionOpenBookmark();
	void actionOpenBookmarkFolder();
	void actionCookies();
	void actionTransfers();
	void actionErrorConsole(bool enabled);
	void actionSidebar(bool enabled);
	void actionContentBlocking();
	void actionPreferences();
	void actionSwitchApplicationLanguage();
	void actionAboutApplication();
	void menuSessionsAboutToShow();
	void menuUserAgentAboutToShow();
	void menuCharacterEncodingAboutToShow();
	void menuClosedWindowsAboutToShow();
	void menuBookmarksAboutToShow();
	void openBookmark();
	void updateClosedWindows();
	void updateBookmarks();
	void updateActions();
	void updateWindowTitle(const QString &title);

private:
	ActionsManager *m_actionsManager;
	WindowsManager *m_windowsManager;
	QMenuBar *m_menuBar;
	QActionGroup *m_sessionsGroup;
	QActionGroup *m_characterEncodingGroup;
	QActionGroup *m_userAgentGroup;
	QMenu *m_closedWindowsMenu;
	QString m_currentBookmark;
	Qt::WindowStates m_previousState;
	Ui::MainWindow *m_ui;

signals:
	void requestedNewWindow(bool isPrivate = false, bool inBackground = false, QUrl url = QUrl());
};

}

#endif
