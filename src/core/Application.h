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
	static Application* getInstance();
	MainWindow* createWindow(bool privateSession = false, bool background = false, const SessionEntry &windows = SessionEntry());
	MainWindow* getWindow();
	QCommandLineParser* getParser() const;
	QList<MainWindow*> getWindows();
	bool isRunning() const;

public slots:
	void newWindow(bool privateSession = false, bool background = false, const QUrl &url = QUrl());

protected slots:
	void newConnection();

private:
	static Application *m_instance;
	QLocalServer *m_localServer;
	QList<MainWindow*> m_windows;
};

}

#endif
