#ifndef OTTER_MAINWINDOW_H
#define OTTER_MAINWINDOW_H

#include "../core/SessionsManager.h"

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

public slots:
	void openUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	bool event(QEvent *event);

protected slots:
	void actionNewTabPrivate();
	void actionOpen();
	void actionTextEncoding(QAction *action);
	void actionClosedWindows(QAction *action);
	void actionAboutApplication();
	void menuTextEncodingAboutToShow();
	void menuClosedWindowsAboutToShow();
	void triggerWindowAction();
	void updateClosedWindows();

private:
	WindowsManager *m_windowsManager;
	QAction *m_closedWindowsAction;
	QActionGroup *m_textEncodingGroup;
	Ui::MainWindow *m_ui;

signals:
	void requestedNewWindow();
	void requestedNewWindowPrivate();
};

}

#endif
