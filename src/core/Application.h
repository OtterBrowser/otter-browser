#ifndef OTTER_APPLICATION_H
#define OTTER_APPLICATION_H

#include "SessionsManager.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QUrl>
#include <QtWidgets/QApplication>
#include <QtNetwork/QLocalServer>

namespace Otter
{

class MainWindow;

class Application : public QApplication
{
	Q_OBJECT

public:
	explicit Application(int &argc, char **argv);
	~Application();

	void removeWindow(MainWindow* window);
	MainWindow* createWindow(bool privateSession = false, bool background = false, const SessionEntry &windows = SessionEntry());
	MainWindow* getWindow();
	QList<MainWindow*> getWindows();
	QCommandLineParser* getParser() const;
	bool isRunning() const;

public slots:
	void newWindow(bool privateSession = false, bool background = false, const QUrl &url = QUrl());

protected slots:
	void newConnection();

private:
	QLocalServer *m_localServer;
	QList<MainWindow*> m_windows;
};

}

#endif
