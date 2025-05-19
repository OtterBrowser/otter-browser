/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_APPLICATION_H
#define OTTER_APPLICATION_H

#include "ActionExecutor.h"
#include "UpdateChecker.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QUrl>
#include <QtWidgets/QApplication>
#include <QtNetwork/QLocalServer>

namespace Otter
{

class LongTermTimer;
class MainWindow;
class Notification;
class PlatformIntegration;
class Style;
class TrayIcon;

class Application final : public QApplication, public ActionExecutor
{
	Q_OBJECT

public:
	enum ReportOption
	{
		BasicReport = 0,
		EnvironmentReport = 1,
		KeyboardShortcutsReport = 2,
		PathsReport = 4,
		SettingsReport = 8,
		StandardReport = (EnvironmentReport | PathsReport | SettingsReport),
		FullReport = (EnvironmentReport | KeyboardShortcutsReport | PathsReport | SettingsReport)
	};

	Q_DECLARE_FLAGS(ReportOptions, ReportOption)

	struct ReportSection final
	{
		QString label;
		QVector<QStringList> records;
	};

	explicit Application(int &argc, char **argv);
	~Application();

	static void triggerAction(int identifier, const QVariantMap &parameters, QObject *target, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger);
	static void removeWindow(MainWindow *mainWindow);
	static void showNotification(Notification *notification);
	static void handlePositionalArguments(QCommandLineParser *parser, bool forceOpen = false);
	static void setHidden(bool isHidden);
	static MainWindow* createWindow(const QVariantMap &parameters = {}, const Session::MainWindow &session = {});
	static Application* getInstance();
	static MainWindow* getActiveWindow();
	static QObject* getFocusObject(bool ignoreMenus);
	static TrayIcon* getTrayIcon();
	static PlatformIntegration* getPlatformIntegration();
	static QCommandLineParser* getCommandLineParser();
	static QString createReport(ReportOptions options = StandardReport);
	static QString getFullVersion();
	static QString getLocalePath();
	static QString getApplicationDirectoryPath();
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	static QVector<MainWindow*> getWindows();
	static bool canClose();
	static bool isAboutToQuit();
	static bool isFirstRun();
	static bool isHidden();
	static bool isUpdating();
	static bool isRunning();

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void scheduleUpdateCheck(quint64 interval);
	static void setLocale(const QString &locale);

protected slots:
	void openUrl(const QUrl &url);
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleAboutToQuit();
	void handleNewConnection();
	void handleUpdateCheckResult(const QVector<UpdateChecker::UpdateInformation> &availableUpdates, int latestVersionIndex);

private:
	Q_DISABLE_COPY(Application)

	LongTermTimer *m_updateCheckTimer;

	static Application *m_instance;
	static PlatformIntegration *m_platformIntegration;
	static TrayIcon *m_trayIcon;
	static QTranslator *m_qtTranslator;
	static QTranslator *m_applicationTranslator;
	static QLocalServer *m_localServer;
	static QPointer<MainWindow> m_activeWindow;
	static QPointer<QObject> m_nonMenuFocusObject;
	static QString m_localePath;
	static QCommandLineParser m_commandLineParser;
	static QVector<MainWindow*> m_windows;
	static bool m_isAboutToQuit;
	static bool m_isFirstRun;
	static bool m_isHidden;
	static bool m_isUpdating;

signals:
	void windowAdded(MainWindow *mainWindow);
	void windowRemoved(MainWindow *mainWindow);
	void activeWindowChanged(MainWindow *mainWindow);
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Application::ReportOptions)

#endif
