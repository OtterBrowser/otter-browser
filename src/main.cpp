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

int main(int argc, char *argv[])
{
	Otter::Application application(argc, argv);

	if (application.isRunning())
	{
		return 0;
	}

	QCommandLineParser *parser = application.getParser();
	parser->process(application);

	Otter::MainWindow *window = NULL;
	const QString session = ((Otter::SessionsManager::getCurrentSession().isEmpty() || !parser->value(QLatin1String("session")).isEmpty()) ? (parser->value(QLatin1String("session")).isEmpty() ? QLatin1String("default") : parser->value(QLatin1String("session"))) : QString());

	if (Otter::SessionsManager::getCurrentSession().isEmpty() && (Otter::SettingsManager::getValue(QLatin1String("Browser/StartupBehavior")).toString() == QLatin1String("showDialog") || !Otter::SessionsManager::getSession(session.isEmpty() ? QLatin1String("default") : session).clean))
	{
		Otter::StartupDialog dialog(session.isEmpty() ? QLatin1String("default") : session);

		if (dialog.exec() == QDialog::Rejected)
		{
			delete parser;

			return 0;
		}
	}
	else
	{
		if (session.isEmpty() || !Otter::SessionsManager::restoreSession(session))
		{
			 window = application.createWindow(parser->isSet(QLatin1String("privatesession")));
		}
		else
		{
			window = application.getWindow();
		}
	}

	if (window && !parser->positionalArguments().isEmpty())
	{
		QStringList urls = parser->positionalArguments();

		for (int i = 0; i < urls.count(); ++i)
		{
			window->openUrl(QUrl(urls.at(i)));
		}
	}

	delete parser;

	return application.exec();
}
