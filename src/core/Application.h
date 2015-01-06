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
class PlatformIntegration;

class Application : public QApplication
{
	Q_OBJECT

public:
	explicit Application(int &argc, char **argv);
	~Application();

	void removeWindow(MainWindow* window);
	void setLocale(const QString &locale);
	QCommandLineParser* createCommandLineParser() const;
	MainWindow* createWindow(bool isPrivate = false, bool inBackground = false, const SessionMainWindow &windows = SessionMainWindow());
	static Application* getInstance();
	MainWindow* getWindow();
	PlatformIntegration* getPlatformIntegration();
	QString getFullVersion() const;
	QString getLocalePath() const;
	QList<MainWindow*> getWindows() const;
	bool canClose();
	bool isHidden() const;
	bool isRunning() const;

public slots:
	void close();
	void newWindow(bool isPrivate = false, bool inBackground = false, const QUrl &url = QUrl());
	void setHidden(bool hidden);

protected slots:
	void newConnection();
	void clearHistory();

private:
	PlatformIntegration *m_platformIntegration;
	QTranslator *m_qtTranslator;
	QTranslator *m_applicationTranslator;
	QLocalServer *m_localServer;
	QString m_localePath;
	QList<MainWindow*> m_windows;
	bool m_isHidden;

	static Application *m_instance;

signals:
	void windowAdded(MainWindow *window);
	void windowRemoved(MainWindow *window);
};

}

#endif
