/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaSessionImportDataExchanger.h"
#include "../../../core/IniSettings.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/Utils.h"

#include <QtCore/QDir>

namespace Otter
{

OperaSessionImportDataExchanger::OperaSessionImportDataExchanger(QObject *parent) : ImportDataExchanger(parent)
{
}

QString OperaSessionImportDataExchanger::getName() const
{
	return QLatin1String("opera-session");
}

QString OperaSessionImportDataExchanger::getTitle() const
{
	return tr("Import Opera Session");
}

QString OperaSessionImportDataExchanger::getDescription() const
{
	return tr("Imports session from Opera Browser version 12 or earlier");
}

QString OperaSessionImportDataExchanger::getVersion() const
{
	return QLatin1String("0.2");
}

QString OperaSessionImportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("sessions/autosave.win"));
		}

		return path;
	}
#if !defined(Q_OS_DARWIN) && defined(Q_OS_UNIX)
	const QString homePath(Utils::getStandardLocation(QStandardPaths::HomeLocation));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/sessions/autosave.win"));
	}
#endif

	return path;
}

QString OperaSessionImportDataExchanger::getGroup() const
{
	return QLatin1String("opera");
}

QUrl OperaSessionImportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OperaSessionImportDataExchanger::getFileFilters() const
{
	return {tr("Opera session files (*.win)")};
}

DataExchanger::ExchangeType OperaSessionImportDataExchanger::getExchangeType() const
{
	return SessionsExchange;
}

bool OperaSessionImportDataExchanger::importData(const QString &path)
{
	QHash<int, Session::MainWindow*> mainWindows;
	IniSettings originalSession(getSuggestedPath(path));
	originalSession.beginGroup(QLatin1String("session"));

	if (originalSession.getValue(QLatin1String("version")).toInt() == 0)
	{
		emit exchangeFinished(SessionsExchange, FailedOperation, 0);

		return false;
	}

	emit exchangeStarted(SessionsExchange, 1);

	SessionInformation session;
	session.title = QFileInfo(path).completeBaseName();

	const int windowCount(originalSession.getValue(QLatin1String("window count")).toInt());

	for (int i = 1; i <= windowCount; ++i)
	{
		originalSession.beginGroup(QString::number(i));

		if (originalSession.getValue(QLatin1String("type")).toInt() == 0)
		{
			Session::MainWindow *mainWindow(new Session::MainWindow());

			mainWindows.insert(originalSession.getValue(QLatin1String("id")).toInt(), mainWindow);

			continue;
		}

		Session::Window window;
		window.state.geometry = {originalSession.getValue(QLatin1String("x")).toInt(), originalSession.getValue(QLatin1String("y")).toInt(), originalSession.getValue(QLatin1String("w")).toInt(), originalSession.getValue(QLatin1String("h")).toInt()};

		switch (originalSession.getValue(QLatin1String("state")).toInt())
		{
			case 0:
				window.state.state = Qt::WindowNoState;

				break;
			case 1:
				window.state.state = Qt::WindowMinimized;

				break;
			default:
				window.state.state = Qt::WindowMaximized;

				break;
		}

		window.isPinned = originalSession.getValue(QLatin1String("locked")).toInt();

		if (originalSession.getValue(QLatin1String("user auto reload enable")).toInt() == 1)
		{
			window.options[SettingsManager::Content_PageReloadTimeOption] = originalSession.getValue(QLatin1String("user auto reload sec user setting")).toInt();
		}

		const int zoom(originalSession.getValue(QLatin1String("scale")).toInt());

		originalSession.beginGroup(QString::number(i) + QLatin1String("history url"));

		Session::Window::History history;
		history.index = (originalSession.getValue(QLatin1String("current history")).toInt() - 1);

		const int historyCount(originalSession.getValue(QLatin1String("count")).toInt());

		for (int j = 0; j < historyCount; ++j)
		{
			Session::Window::History::Entry entry;
			entry.url = originalSession.getValue(QString::number(j)).toString();

			QUrl url(entry.url);

			if (url.scheme() == QLatin1String("opera"))
			{
				url.setScheme(QLatin1String("about"));

				if (url.path() == QLatin1String("speeddial"))
				{
					url.setPath(QLatin1String("start"));
				}

				entry.url = url.url();
			}

			entry.zoom = zoom;

			history.entries.append(entry);
		}

		window.history = history;

		if (historyCount == 0)
		{
			originalSession.beginGroup(QString::number(i));

			const QString panel(originalSession.getValue(QLatin1String("panel type")).toString());

			if (!panel.isEmpty())
			{
				Session::Window::History::Entry entry;
				entry.url = QLatin1String("about:") + panel.toLower();

				window.history.entries.append(entry);
				window.history.index = 0;
			}
		}

		originalSession.beginGroup(QString::number(i) + QLatin1String("history title"));

		for (int j = 0; j < historyCount; ++j)
		{
			window.history.entries[j].title = originalSession.getValue(QString::number(j)).toString();
		}

		originalSession.beginGroup(QString::number(i) + QLatin1String("history scrollpos list"));

		for (int j = 0; j < historyCount; ++j)
		{
			window.history.entries[j].position.setY(originalSession.getValue(QString::number(j)).toInt());
		}

		originalSession.beginGroup(QString::number(i));

		if (originalSession.getValue(QLatin1String("has speeddial in history")).toInt())
		{
			window.history.entries.prepend(Session::Window::History::Entry());

			window.history.index = (window.history.index + 1);
		}

		const int parent(originalSession.getValue(QLatin1String("parent")).toInt());

		if (parent > 0 && mainWindows.contains(parent))
		{
			mainWindows[parent]->windows.append(window);

			if (originalSession.getValue(QLatin1String("active")).toInt() == 1)
			{
				mainWindows[parent]->index = mainWindows[parent]->windows.count() - 1;
			}
		}
	}

	QHash<int, Session::MainWindow*>::iterator iterator;

	for (iterator = mainWindows.begin(); iterator != mainWindows.end(); ++iterator)
	{
		session.windows.append(**iterator);
	}

	const bool result(SessionsManager::saveSession(session));

	qDeleteAll(mainWindows);

	emit exchangeFinished(SessionsExchange, (result ? SuccessfullOperation : FailedOperation), 1);

	return result;
}

}
