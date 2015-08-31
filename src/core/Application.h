/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "UpdateChecker.h"
#include "SessionsManager.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QUrl>
#include <QtWidgets/QApplication>
#include <QtNetwork/QLocalServer>

namespace Otter
{

class MainWindow;
class Notification;
class PlatformIntegration;
class TrayIcon;

class Application : public QApplication
{
	Q_OBJECT

public:
	explicit Application(int &argc, char **argv);
	~Application();

	void removeWindow(MainWindow* window);
	void showNotification(Notification *notification);
	void setLocale(const QString &locale);
	MainWindow* createWindow(bool isPrivate = false, bool inBackground = false, const SessionMainWindow &windows = SessionMainWindow());
	static Application* getInstance();
	MainWindow* getWindow();
	TrayIcon* getTrayIcon();
	PlatformIntegration* getPlatformIntegration();
	QCommandLineParser* getCommandLineParser();
	QString getFullVersion() const;
	QString getLocalePath() const;
	QList<MainWindow*> getWindows() const;
	bool canClose();
	bool isHidden() const;
	bool isUpdating() const;
	bool isRunning() const;

public slots:
	void close();
	void newWindow(bool isPrivate = false, bool inBackground = false, const QUrl &url = QUrl());
	void setHidden(bool hidden);

protected slots:
	void updateCheckFinished(const QList<UpdateInformation> &availableUpdates);
	void newConnection();
	void clearHistory();
	void periodicUpdateCheck();
	void showUpdateDetails();

private:
	PlatformIntegration *m_platformIntegration;
	TrayIcon *m_trayIcon;
	QTranslator *m_qtTranslator;
	QTranslator *m_applicationTranslator;
	QLocalServer *m_localServer;
	QString m_localePath;
	QCommandLineParser m_commandLineParser;
	QList<MainWindow*> m_windows;
	bool m_isHidden;
	bool m_isUpdating;

	static Application *m_instance;

signals:
	void windowAdded(MainWindow *window);
	void windowRemoved(MainWindow *window);
};

}

#endif
