/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "core/Application.h"
#include "core/SessionsManager.h"
#include "core/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/StartupDialog.h"
#ifdef OTTER_ENABLE_CRASH_REPORTS
#ifdef Q_OS_WIN32
#include "../3rdparty/breakpad/src/client/windows/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "../3rdparty/breakpad/src/client/linux/handler/exception_handler.h"
#endif

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#endif
#include <QtCore/QUrl>
#include <QtWidgets/QMessageBox>

using namespace Otter;

#if !defined(Q_OS_WIN32)
void otterMessageHander(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
	if (message.trimmed().startsWith(QLatin1String("OpenType support missing")) || message.startsWith(QLatin1String("libpng warning: iCCP:")) || message.startsWith(QLatin1String("OpenType support missing for script")) || message.startsWith(QLatin1String("QCoreApplication::postEvent: Unexpected null receiver")) || message.startsWith(QLatin1String("QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once")) || message.startsWith(QLatin1String("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread")) || message.startsWith(QLatin1String("QFSFileEngine::open")) || message.contains(QLatin1String("::_q_startOperation was called more than once")))
	{
		return;
	}

	fputs(qFormatLogMessage(type, context, message).toLocal8Bit().constData(), stderr);

	if (type == QtFatalMsg)
	{
		abort();
	}
}
#endif

#ifdef OTTER_ENABLE_CRASH_REPORTS
#ifdef Q_OS_WIN32
bool otterCrashDumpHandler(const wchar_t *dumpDirectory, const wchar_t *dumpIdentifier, void *context, EXCEPTION_POINTERS *exceptionInformation, MDRawAssertionInfo *assertionInformation, bool hasDumpFile)
{
	Q_UNUSED(context)
	Q_UNUSED(exceptionInformation)
	Q_UNUSED(assertionInformation)

	if (hasDumpFile)
	{
		const QString dumpPath(QDir::toNativeSeparators(QString::fromWCharArray(dumpDirectory) + QDir::separator() + QString::fromWCharArray(dumpIdentifier) + QLatin1String(".dmp")));

		qDebug("Crash dump saved to: %s", dumpPath.toLocal8Bit().constData());

		const MainWindow *mainWindow(Application::getActiveWindow());

		QProcess::startDetached(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String("crash-reporter.exe")), {dumpPath, (mainWindow ? mainWindow->getUrl().toDisplayString() : QString())});
	}

	return hasDumpFile;
}
#elif defined(Q_OS_LINUX)
bool otterCrashDumpHandler(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool hasDumpFile)
{
	Q_UNUSED(context)

	if (hasDumpFile)
	{
		qDebug("Crash dump saved to: %s", descriptor.path());

		const MainWindow *mainWindow(Application::getActiveWindow());

		QProcess::startDetached(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String("crash-reporter")), {descriptor.path(), (mainWindow ? mainWindow->getUrl().toDisplayString() : QString())});
	}

	return hasDumpFile;
}
#endif
#endif

int main(int argc, char *argv[])
{
	QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR)

#if !defined(Q_OS_WIN32)
	qSetMessagePattern(QLatin1String("%{if-category}%{category}: %{endif}%{message}\n"));
	qInstallMessageHandler(otterMessageHander);
#endif

#ifdef OTTER_ENABLE_CRASH_REPORTS
#ifdef Q_OS_WIN32
	new google_breakpad::ExceptionHandler(reinterpret_cast<const wchar_t*>(QStandardPaths::writableLocation(QStandardPaths::TempLocation).utf16()), 0, otterCrashDumpHandler, 0, true);
#elif defined(Q_OS_LINUX)
	new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString()), 0, otterCrashDumpHandler, 0, true, -1);
#endif
#endif

	// Enable automatic High-DPI scaling support. This could be done with earlier Qt
	// versions as well (down to 5.6), but it was rather buggy before 5.9, so we
	// restrict to 5.9 and higher.
	// Users can force-enable this on olde Qt versions by setting the environment
	// variable `QT_AUTO_SCREEN_SCALE_FACTOR=1`.
	// Note that this attribute must be enabled before the QApplication is
	// constructed, hence the use of the static version of setAttribute().
//	Application::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#if QT_VERSION < 0x060000
	// Use static version for this attribute too, for consistency with the above.
	Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif

	Application application(argc, argv);

#ifdef OTTER_NO_WEB_BACKENDS
	QMessageBox::critical(nullptr, QLatin1String("Error"), QLatin1String("No web backends available."), QMessageBox::Close);

	return 0;
#endif

	QCommandLineParser *commandLineParser(Application::getCommandLineParser());

	if (Application::isAboutToQuit() || Application::isRunning() || Application::isUpdating() || commandLineParser->isSet(QLatin1String("report")))
	{
		return 0;
	}

	const QString session(commandLineParser->value(QLatin1String("session")).isEmpty() ? QLatin1String("default") : commandLineParser->value(QLatin1String("session")));
	const QString startupBehavior(SettingsManager::getOption(SettingsManager::Browser_StartupBehaviorOption).toString());
	const bool isPrivate(commandLineParser->isSet(QLatin1String("private-session")));

	if (!commandLineParser->value(QLatin1String("session")).isEmpty() && SessionsManager::getSession(session).isClean)
	{
		SessionsManager::restoreSession(SessionsManager::getSession(session), nullptr, isPrivate);
	}
	else if (startupBehavior == QLatin1String("showDialog") || commandLineParser->isSet(QLatin1String("session-chooser")) || !SessionsManager::getSession(session).isClean)
	{
		StartupDialog dialog(session);

		if (dialog.exec() == QDialog::Rejected)
		{
			return 0;
		}

		SessionsManager::restoreSession(dialog.getSession(), nullptr, isPrivate);
	}
	else if (startupBehavior == QLatin1String("continuePrevious"))
	{
		SessionsManager::restoreSession(SessionsManager::getSession(QLatin1String("default")), nullptr, isPrivate);
	}
	else
	{
		SessionInformation sessionData(SessionsManager::getSession(QLatin1String("default")));
		sessionData.path = QLatin1String("default");
		sessionData.title = QCoreApplication::translate("main", "Default");

		Session::MainWindow window;

		if (sessionData.isValid())
		{
			window.geometry = sessionData.windows.value(0).geometry;
		}

		if (startupBehavior != QLatin1String("startEmpty"))
		{
			Session::Window::History::Entry entry;

			if (startupBehavior == QLatin1String("startHomePage"))
			{
				entry.url = SettingsManager::getOption(SettingsManager::Browser_HomePageOption).toString();
			}
			else if (startupBehavior == QLatin1String("startStartPage"))
			{
				entry.url = QLatin1String("about:start");
			}
			else
			{
				entry.url = QLatin1String("about:blank");
			}

			Session::Window::History history;
			history.entries = {entry};
			history.index = 0;

			Session::Window tab;
			tab.history = history;

			window.windows = {tab};
			window.index = 0;
		}

		sessionData.windows = {window};
		sessionData.index = 0;

		SessionsManager::restoreSession(sessionData, nullptr, isPrivate);
	}

	Application::handlePositionalArguments(commandLineParser);

	if (Application::getWindows().isEmpty())
	{
		Application::createWindow({{QLatin1String("hints"), (isPrivate ? SessionsManager::PrivateOpen : SessionsManager::DefaultOpen)}});
	}

	return Application::exec();
}
