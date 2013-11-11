#ifndef OTTER_MAINWINDOW_H
#define OTTER_MAINWINDOW_H

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
	explicit MainWindow(QWidget *parent = NULL);
	~MainWindow();

public slots:
	void openUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	bool event(QEvent *event);

private:
	WindowsManager *m_windowsManager;
	Ui::MainWindow *m_ui;
};

}

#endif
