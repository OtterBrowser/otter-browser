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

#include "core/Application.h"
#include "core/SessionsManager.h"
#include "core/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/StartupDialog.h"

#include <QtCore/QUrl>

using namespace Otter;

void otterMessageHander(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
	if (message.trimmed().startsWith(QLatin1String("OpenType support missing")) || message.startsWith(QLatin1String("libpng warning: iCCP:")) || message.startsWith(QLatin1String("OpenType support missing for script")) || message.startsWith(QLatin1String("QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once")) || message.startsWith(QLatin1String("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread")))
	{
		return;
	}

	switch (type)
	{
		case QtDebugMsg:
			fprintf(stderr, "Debug: %s (%s:%u, %s)\n", message.toLocal8Bit().constData(), context.file, context.line, context.function);

			break;
#if QT_VERSION >= 0x050500
		case QtInfoMsg:
			fprintf(stderr, "Info: %s (%s:%u, %s)\n", message.toLocal8Bit().constData(), context.file, context.line, context.function);

			break;
#endif
		case QtWarningMsg:
			fprintf(stderr, "Warning: %s (%s:%u, %s)\n", message.toLocal8Bit().constData(), context.file, context.line, context.function);

			break;
		case QtCriticalMsg:
			fprintf(stderr, "Critical: %s (%s:%u, %s)\n", message.toLocal8Bit().constData(), context.file, context.line, context.function);

			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", message.toLocal8Bit().constData(), context.file, context.line, context.function);
			abort();
	}
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(otterMessageHander);

	Application application(argc, argv);
	application.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

	if (application.isRunning() || application.isUpdating() || application.getCommandLineParser()->isSet(QLatin1String("report")))
	{
		return 0;
	}

	const QString session = (application.getCommandLineParser()->value(QLatin1String("session")).isEmpty() ? QLatin1String("default") : application.getCommandLineParser()->value(QLatin1String("session")));
	const QString startupBehavior = SettingsManager::getValue(QLatin1String("Browser/StartupBehavior")).toString();
	const bool isPrivate = application.getCommandLineParser()->isSet(QLatin1String("privatesession"));

	if (!application.getCommandLineParser()->value(QLatin1String("session")).isEmpty() && SessionsManager::getSession(session).isClean)
	{
		SessionsManager::restoreSession(SessionsManager::getSession(session), NULL, isPrivate);
	}
	else if (startupBehavior == QLatin1String("showDialog") || application.getCommandLineParser()->isSet(QLatin1String("sessionchooser")) || !SessionsManager::getSession(session).isClean)
	{
		StartupDialog dialog(session);

		if (dialog.exec() == QDialog::Rejected)
		{
			return 0;
		}

		SessionsManager::restoreSession(dialog.getSession(), NULL, isPrivate);
	}
	else if (startupBehavior == QLatin1String("continuePrevious"))
	{
		SessionsManager::restoreSession(SessionsManager::getSession(QLatin1String("default")), NULL, isPrivate);
	}
	else if (startupBehavior != QLatin1String("startEmpty"))
	{
		WindowHistoryEntry entry;

		if (startupBehavior == QLatin1String("startHomePage"))
		{
			entry.url = SettingsManager::getValue(QLatin1String("Browser/HomePage")).toString();
		}
		else if (startupBehavior == QLatin1String("startStartPage"))
		{
			entry.url = QLatin1String("about:start");
		}
		else
		{
			entry.url = QLatin1String("about:blank");
		}

		SessionWindow tab;
		tab.history.append(entry);
		tab.historyIndex = 0;

		SessionMainWindow window;
		window.windows.append(tab);

		SessionInformation sessionData;
		sessionData.path = QLatin1String("default");
		sessionData.title = QCoreApplication::translate("main", "Default");
		sessionData.windows.append(window);
		sessionData.index = 0;

		SessionsManager::restoreSession(sessionData, NULL, isPrivate);
	}

	if (!application.getCommandLineParser()->positionalArguments().isEmpty())
	{
		MainWindow *window = application.getWindow();

		if (window)
		{
			const QStringList urls = application.getCommandLineParser()->positionalArguments();

			for (int i = 0; i < urls.count(); ++i)
			{
				window->openUrl(urls.at(i));
			}
		}
	}

	if (application.getWindows().isEmpty())
	{
		application.createWindow(isPrivate);
	}

	return application.exec();
}
