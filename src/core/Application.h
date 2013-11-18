#ifndef OTTER_APPLICATION_H
#define OTTER_APPLICATION_H

#include "SessionsManager.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QPointer>
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

	MainWindow* createWindow(bool privateSession = false, const SessionEntry &windows = SessionEntry());
	MainWindow* getWindow();
	QCommandLineParser* getParser() const;
	bool isRunning() const;

public slots:
	void newWindow();
	void newWindowPrivate();

protected:
	void cleanup();

protected slots:
	void newConnection();

private:
	QLocalServer *m_localServer;
	QList<QPointer<MainWindow> > m_windows;
};

}

#endif
