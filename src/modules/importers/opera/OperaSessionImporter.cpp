/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaSessionImporter.h"
#include "../../../core/IniSettings.h"
#include "../../../core/SessionsManager.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace Otter
{

OperaSessionImporter::OperaSessionImporter(QObject *parent) : Importer(parent)
{
}

QWidget* OperaSessionImporter::createOptionsWidget(QWidget *parent)
{
	Q_UNUSED(parent)

	return nullptr;
}

QString OperaSessionImporter::getTitle() const
{
	return tr("Opera Session");
}

QString OperaSessionImporter::getDescription() const
{
	return tr("Imports session from Opera Browser version 12 or earlier");
}

QString OperaSessionImporter::getVersion() const
{
	return QLatin1String("0.2");
}

QString OperaSessionImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("sessions/autosave.win"));
		}
		else
		{
			return path;
		}
	}
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	const QString homePath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/sessions/autosave.win"));
	}
#endif

	return path;
}

QString OperaSessionImporter::getBrowser() const
{
	return QLatin1String("opera");
}

QUrl OperaSessionImporter::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OperaSessionImporter::getFileFilters() const
{
	return {tr("Opera session files (*.win)")};
}

Importer::ImportType OperaSessionImporter::getImportType() const
{
	return SessionsImport;
}

bool OperaSessionImporter::import(const QString &path)
{
	QHash<int, SessionMainWindow*> mainWindows;
	IniSettings originalSession(getSuggestedPath(path));
	originalSession.beginGroup(QLatin1String("session"));

	if (originalSession.getValue(QLatin1String("version")).toInt() == 0)
	{
		emit importFinished(SessionsImport, FailedImport, 0);

		return false;
	}

	emit importStarted(SessionsImport, 1);

	SessionInformation session;
	session.title = QFileInfo(path).completeBaseName();

	const int windowCount(originalSession.getValue(QLatin1String("window count")).toInt());

	for (int i = 1; i <= windowCount; ++i)
	{
		originalSession.beginGroup(QString::number(i));

		if (originalSession.getValue(QLatin1String("type")).toInt() == 0)
		{
			SessionMainWindow *mainWindow(new SessionMainWindow());

			mainWindows.insert(originalSession.getValue(QLatin1String("id")).toInt(), mainWindow);

			continue;
		}

		SessionWindow window;
		WindowState windowState;
		windowState.geometry = QRect(originalSession.getValue(QLatin1String("x")).toInt(), originalSession.getValue(QLatin1String("y")).toInt(), originalSession.getValue(QLatin1String("w")).toInt(), originalSession.getValue(QLatin1String("h")).toInt());

		switch (originalSession.getValue(QLatin1String("state")).toInt())
		{
			case 0:
				windowState.state = Qt::WindowNoState;

				break;
			case 1:
				windowState.state = Qt::WindowMinimized;

				break;
			default:
				windowState.state = Qt::WindowMaximized;

				break;
		}

		window.state = windowState;
		window.historyIndex = (originalSession.getValue(QLatin1String("current history")).toInt() - 1);
		window.isPinned = originalSession.getValue(QLatin1String("locked")).toInt();

		if (originalSession.getValue(QLatin1String("user auto reload enable")).toInt() == 1)
		{
			window.options[SettingsManager::Content_PageReloadTimeOption] = originalSession.getValue(QLatin1String("user auto reload sec user setting")).toInt();
		}

		const int zoom(originalSession.getValue(QLatin1String("scale")).toInt());

		originalSession.beginGroup(QString::number(i) + QLatin1String("history url"));

		const int historyCount(originalSession.getValue(QLatin1String("count")).toInt());

		for (int j = 0; j < historyCount; ++j)
		{
			WindowHistoryEntry entry;
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

			window.history.append(entry);
		}

		if (historyCount == 0)
		{
			originalSession.beginGroup(QString::number(i));

			const QString panel(originalSession.getValue(QLatin1String("panel type")).toString());

			if (!panel.isEmpty())
			{
				WindowHistoryEntry entry;
				entry.url = QLatin1String("about:") + panel.toLower();

				window.history.append(entry);

				window.historyIndex = 0;
			}
		}

		originalSession.beginGroup(QString::number(i) + QLatin1String("history title"));

		for (int j = 0; j < historyCount; ++j)
		{
			WindowHistoryEntry entry = window.history.at(j);
			entry.title = originalSession.getValue(QString::number(j)).toString();

			window.history.replace(j, entry);
		}

		originalSession.beginGroup(QString::number(i) + QLatin1String("history scrollpos list"));

		for (int j = 0; j < historyCount; ++j)
		{
			WindowHistoryEntry entry = window.history.at(j);
			entry.position.setY(originalSession.getValue(QString::number(j)).toInt());

			window.history.replace(j, entry);
		}

		originalSession.beginGroup(QString::number(i));

		if (originalSession.getValue(QLatin1String("has speeddial in history")).toInt())
		{
			window.history.prepend(WindowHistoryEntry());

			window.historyIndex = (window.historyIndex + 1);
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

	QHash<int, SessionMainWindow*>::iterator iterator;

	for (iterator = mainWindows.begin(); iterator != mainWindows.end(); ++iterator)
	{
		session.windows.append(**iterator);
	}

	const bool result(SessionsManager::saveSession(session));

	qDeleteAll(mainWindows);

	emit importFinished(SessionsImport, (result ? SuccessfullImport : FailedImport), 1);

	return result;
}

}
