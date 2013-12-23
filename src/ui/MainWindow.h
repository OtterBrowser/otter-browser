#ifndef OTTER_MAINWINDOW_H
#define OTTER_MAINWINDOW_H

#include "../core/SessionsManager.h"
#include "../core/WindowsManager.h"

#include <QtWidgets/QActionGroup>
#include <QtWidgets/QMainWindow>

namespace Otter
{

namespace Ui
{
	class MainWindow;
}

class WindowsManager;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(bool privateSession = false, const SessionEntry &windows = SessionEntry(), QWidget *parent = NULL);
	~MainWindow();

	WindowsManager* getWindowsManager();

public slots:
	void openUrl(const QUrl &url = QUrl());

protected:
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	void gatherBookmarks(int folder);
	void updateAction(QAction *source, QAction *target);
	bool event(QEvent *event);

protected slots:
	void actionNewTabPrivate();
	void actionNewWindowPrivate();
	void actionOpen();
	void actionSaveSession();
	void actionManageSessions();
	void actionSession(QAction *action);
	void actionWorkOffline(bool enabled);
	void actionTextEncoding(QAction *action);
	void actionClosedWindows(QAction *action);
	void actionViewHistory();
	void actionAddBookmark(const QUrl &url = QUrl());
	void actionManageBookmarks();
	void actionOpenBookmark();
	void actionOpenBookmarkFolder();
	void actionCookies();
	void actionTransfers();
	void actionPreferences();
	void actionAboutApplication();
	void menuFileAboutToShow();
	void menuSessionsAboutToShow();
	void menuTextEncodingAboutToShow();
	void menuClosedWindowsAboutToShow();
	void menuBookmarksAboutToShow();
	void triggerWindowAction();
	void updateClosedWindows();
	void updateBookmarks(int folder);
	void updateActions();

private:
	WindowsManager *m_windowsManager;
	QAction *m_closedWindowsAction;
	QActionGroup *m_sessionsGroup;
	QActionGroup *m_textEncodingGroup;
	QList<QString> m_bookmarksToOpen;
	Ui::MainWindow *m_ui;

signals:
	void requestedNewWindow(bool privateSession = false, bool background = false, QUrl url = QUrl());
};

}

#endif
