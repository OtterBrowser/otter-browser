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

#include "core/Application.h"
#include "core/SessionsManager.h"
#include "core/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/StartupDialog.h"

#include <QtCore/QUrl>

using namespace Otter;

void otterMessageHander(QtMsgType type, const QMessageLogContext &context, const QString &mesage)
{
	const QByteArray localMessage = mesage.toLocal8Bit();

	if (localMessage.startsWith(QLatin1String("libpng warning: iCCP: Not recognizing known sRGB profile that has been edited").data()) || localMessage.startsWith(QLatin1String("OpenType support missing for script").data()) || localMessage.startsWith(QLatin1String("QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once").data()) || localMessage.startsWith(QLatin1String("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread").data()))
	{
		return;
	}

	switch (type)
	{
		case QtDebugMsg:
			fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMessage.constData(), context.file, context.line, context.function);

			break;
		case QtWarningMsg:
			fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMessage.constData(), context.file, context.line, context.function);

			break;
		case QtCriticalMsg:
			fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMessage.constData(), context.file, context.line, context.function);

			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMessage.constData(), context.file, context.line, context.function);
			abort();
	}
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(otterMessageHander);

	Application application(argc, argv);

	if (application.isRunning())
	{
		return 0;
	}

	QCommandLineParser *parser = application.getParser();
	parser->process(application);

	const QString session = (parser->value(QLatin1String("session")).isEmpty() ? QLatin1String("default") : parser->value(QLatin1String("session")));
	const QString startupBehavior = SettingsManager::getValue(QLatin1String("Browser/StartupBehavior")).toString();
	const bool isPrivate = parser->isSet(QLatin1String("privatesession"));

	if (!parser->value(QLatin1String("session")).isEmpty() && SessionsManager::getSession(session).clean)
	{
		SessionsManager::restoreSession(SessionsManager::getSession(session), NULL, isPrivate);
	}
	else if (startupBehavior == QLatin1String("showDialog") || !SessionsManager::getSession(session).clean)
	{
		StartupDialog dialog(session);

		if (dialog.exec() == QDialog::Rejected)
		{
			delete parser;

			return 0;
		}

		SessionsManager::restoreSession(dialog.getSession(), NULL, isPrivate);
	}
	else if (startupBehavior == QLatin1String("startHomePage") || startupBehavior == QLatin1String("startEmpty"))
	{
		WindowHistoryEntry entry;
		entry.url = ((startupBehavior == QLatin1String("startHomePage")) ? SettingsManager::getValue(QLatin1String("Browser/HomePage")).toString() : QString());

		SessionWindow tab;
		tab.history.append(entry);
		tab.index = 0;

		SessionMainWindow window;
		window.windows.append(tab);

		SessionInformation sessionData;
		sessionData.path = QLatin1String("default");
		sessionData.title = QCoreApplication::translate("main", "Default");
		sessionData.windows.append(window);
		sessionData.index = 0;

		SessionsManager::restoreSession(sessionData, NULL, isPrivate);
	}
	else
	{
		SessionsManager::restoreSession(SessionsManager::getSession(QLatin1String("default")), NULL, isPrivate);
	}

	if (!parser->positionalArguments().isEmpty())
	{
		MainWindow *window = application.getWindow();

		if (window)
		{
			QStringList urls = parser->positionalArguments();

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

	delete parser;

	return application.exec();
}
