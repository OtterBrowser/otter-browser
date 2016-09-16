/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
	enum MainWindowFlag
	{
		NoFlags = 0,
		PrivateFlag = 1,
		SingleWindowFlag = 2,
		NoToolBarsFlag = 4
	};

	Q_DECLARE_FLAGS(MainWindowFlags, MainWindowFlag)

	explicit Application(int &argc, char **argv);
	~Application();

	static void openWindow(bool isPrivate = false, bool inBackground = false, const QUrl &url = QUrl());
	static void removeWindow(MainWindow* window);
	void showNotification(Notification *notification);
	void setLocale(const QString &locale);
	static MainWindow* createWindow(MainWindowFlags flags = NoFlags, bool inBackground = false, const SessionMainWindow &windows = SessionMainWindow());
	static Application* getInstance();
	MainWindow* getWindow();
	TrayIcon* getTrayIcon();
	PlatformIntegration* getPlatformIntegration();
	QCommandLineParser* getCommandLineParser();
	static QString createReport();
	QString getFullVersion() const;
	QString getLocalePath() const;
	QList<MainWindow*> getWindows() const;
	bool canClose();
	bool isHidden() const;
	bool isUpdating() const;
	bool isRunning() const;

public slots:
	void close();
	void setHidden(bool hidden);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void openUrl(const QUrl &url);
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
	bool m_isHidden;
	bool m_isUpdating;

	static Application *m_instance;
	static QList<MainWindow*> m_windows;

signals:
	void windowAdded(MainWindow *window);
	void windowRemoved(MainWindow *window);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Application::MainWindowFlags)

#endif
