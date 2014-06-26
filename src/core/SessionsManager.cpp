/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SessionsManager.h"
#include "ActionsManager.h"
#include "Application.h"
#include "SettingsManager.h"
#include "Utils.h"
#include "WindowsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QSaveFile>
#include <QtCore/QSettings>

namespace Otter
{

SessionsManager* SessionsManager::m_instance = NULL;
QPointer<MainWindow> SessionsManager::m_activeWindow = NULL;
QString SessionsManager::m_session;
QString SessionsManager::m_cachePath;
QString SessionsManager::m_profilePath;
QList<WindowsManager*> SessionsManager::m_managers;
QList<SessionMainWindow> SessionsManager::m_closedWindows;
bool SessionsManager::m_dirty = false;

SessionsManager::SessionsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void SessionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		m_dirty = false;

		killTimer(m_saveTimer);

		m_saveTimer = 0;

		saveSession(QString(), QString(), NULL, false);
	}
}

void SessionsManager::createInstance(const QString &profilePath, const QString &cachePath, QObject *parent)
{
	m_instance = new SessionsManager(parent);
	m_cachePath = cachePath;
	m_profilePath = profilePath;
}

void SessionsManager::scheduleSave()
{
	if (m_saveTimer == 0)
	{
		m_saveTimer = startTimer(1000);
	}
}

void SessionsManager::connectActions()
{
	connect(ActionsManager::getAction(QuickFindAction), SIGNAL(triggered()), m_instance, SLOT(actionTriggered()));
	connect(ActionsManager::getAction(ActivateAddressFieldAction), SIGNAL(triggered()), m_instance, SLOT(actionTriggered()));
	connect(ActionsManager::getAction(PasteAndGoAction), SIGNAL(triggered()), m_instance, SLOT(actionTriggered()));
	connect(ActionsManager::getAction(ActivateTabOnLeftAction), SIGNAL(triggered()), m_instance, SLOT(actionTriggered()));
	connect(ActionsManager::getAction(ActivateTabOnRightAction), SIGNAL(triggered()), m_instance, SLOT(actionTriggered()));
}

void SessionsManager::actionTriggered()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (!action || !m_activeWindow)
	{
		return;
	}

	const ActionIdentifier windowAction = static_cast<ActionIdentifier>(action->data().toInt());

	if (windowAction != UnknownAction)
	{
		m_activeWindow->getWindowsManager()->triggerAction(windowAction);
	}
}

void SessionsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	emit m_instance->closedWindowsChanged();
}

void SessionsManager::registerWindow(WindowsManager *manager)
{
	if (manager)
	{
		m_managers.append(manager);
	}
}

void SessionsManager::storeClosedWindow(WindowsManager *manager)
{
	if (!manager)
	{
		return;
	}

	m_managers.removeAll(manager);

	SessionMainWindow session = manager->getSession();

	if (!session.windows.isEmpty())
	{
		m_closedWindows.prepend(session);

		emit m_instance->closedWindowsChanged();
	}
}

void SessionsManager::markSessionModified()
{
	if (m_session == QLatin1String("default") && !m_dirty)
	{
		m_dirty = true;

		m_instance->scheduleSave();
	}
}

void SessionsManager::removeStoredUrl(const QString &url)
{
	emit m_instance->requestedRemoveStoredUrl(url);
}

void SessionsManager::setActiveWindow(MainWindow *window)
{
	m_activeWindow = window;
}

SessionsManager* SessionsManager::getInstance()
{
	return m_instance;
}

WindowsManager* SessionsManager::getWindowsManager()
{
	return (m_activeWindow ? m_activeWindow->getWindowsManager() : NULL);
}

QWidget* SessionsManager::getActiveWindow()
{
	return m_activeWindow;
}

QString SessionsManager::getCurrentSession()
{
	return m_session;
}

QString SessionsManager::getCachePath()
{
	return m_cachePath;
}

QString SessionsManager::getProfilePath()
{
	return m_profilePath;
}

QString SessionsManager::getSessionPath(const QString &path, bool bound)
{
	QString cleanPath = path;

	if (cleanPath.isEmpty())
	{
		cleanPath = QLatin1String("default.ini");
	}
	else
	{
		if (!cleanPath.endsWith(QLatin1String(".ini")))
		{
			cleanPath += QLatin1String(".ini");
		}

		if (bound)
		{
			cleanPath = cleanPath.replace(QLatin1Char('/'), QString()).replace(QLatin1Char('\\'), QString());
		}
		else if (QFileInfo(cleanPath).isAbsolute())
		{
			return cleanPath;
		}
	}

	return m_profilePath + QLatin1String("/sessions/") + cleanPath;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const SessionMainWindow window = m_closedWindows.at(i);
		const QString title = window.windows.value(window.index, SessionWindow()).getTitle();

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	const QString sessionPath = getSessionPath(path);
	QSettings sessionData(sessionPath, QSettings::IniFormat);
	sessionData.setIniCodec("UTF-8");

	const int windows = sessionData.value(QLatin1String("Session/windows"), 0).toInt();
	SessionInformation session;
	session.path = path;
	session.title = sessionData.value(QLatin1String("Session/title"), ((path == QLatin1String("default")) ? tr("Default") : tr("(Untitled)"))).toString();
	session.index = (sessionData.value(QLatin1String("Session/index"), 1).toInt() - 1);
	session.clean = sessionData.value(QLatin1String("Session/clean"), true).toBool();

	for (int i = 1; i <= windows; ++i)
	{
		const int tabs = sessionData.value(QStringLiteral("%1/Properties/windows").arg(i), 0).toInt();
		SessionMainWindow sessionEntry;
		sessionEntry.index = (sessionData.value(QStringLiteral("%1/Properties/index").arg(i), 1).toInt() - 1);

		for (int j = 1; j <= tabs; ++j)
		{
			const int history = sessionData.value(QStringLiteral("%1/%2/Properties/history").arg(i).arg(j), 0).toInt();
			SessionWindow sessionWindow;
			sessionWindow.searchEngine = sessionData.value(QStringLiteral("%1/%2/Properties/searchEngine").arg(i).arg(j), QString()).toString();
			sessionWindow.userAgent = sessionData.value(QStringLiteral("%1/%2/Properties/userAgent").arg(i).arg(j), QString()).toString();
			sessionWindow.group = sessionData.value(QStringLiteral("%1/%2/Properties/group").arg(i).arg(j), 0).toInt();
			sessionWindow.index = (sessionData.value(QStringLiteral("%1/%2/Properties/index").arg(i).arg(j), 1).toInt() - 1);
			sessionWindow.pinned = sessionData.value(QStringLiteral("%1/%2/Properties/pinned").arg(i).arg(j), false).toBool();

			for (int k = 1; k <= history; ++k)
			{
				const QStringList position = sessionData.value(QStringLiteral("%1/%2/History/%3/position").arg(i).arg(j).arg(k), 1).toStringList();
				WindowHistoryEntry historyEntry;
				historyEntry.url = sessionData.value(QStringLiteral("%1/%2/History/%3/url").arg(i).arg(j).arg(k), 0).toString();
				historyEntry.title = sessionData.value(QStringLiteral("%1/%2/History/%3/title").arg(i).arg(j).arg(k), 1).toString();
				historyEntry.position = QPoint(position.value(0, QString::number(0)).toInt(), position.value(1, QString::number(0)).toInt());
				historyEntry.zoom = sessionData.value(QStringLiteral("%1/%2/History/%3/zoom").arg(i).arg(j).arg(k), 100).toInt();

				sessionWindow.history.append(historyEntry);
			}

			sessionEntry.windows.append(sessionWindow);
		}

		session.windows.append(sessionEntry);
	}

	return session;
}

QStringList SessionsManager::getSessions()
{
	QStringList entries = QDir(m_profilePath + QLatin1String("/sessions/")).entryList(QStringList(QLatin1String("*.ini")), QDir::Files);

	for (int i = 0; i < entries.count(); ++i)
	{
		entries[i] = QFileInfo(entries.at(i)).completeBaseName();
	}

	if (!m_session.isEmpty() && !entries.contains(m_session))
	{
		entries.append(m_session);
	}

	if (!entries.contains(QLatin1String("default")))
	{
		entries.append(QLatin1String("default"));
	}

	entries.sort();

	return entries;
}

bool SessionsManager::restoreClosedWindow(int index)
{
	if (index < 0)
	{
		index = 0;
	}

	Application::getInstance()->createWindow(false, false, m_closedWindows.value(index, SessionMainWindow()));

	m_closedWindows.removeAt(index);

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const SessionInformation &session, MainWindow *window, bool isPrivate)
{
	if (session.windows.isEmpty())
	{
		if (m_session.isEmpty() && session.path == QLatin1String("default"))
		{
			m_session = QLatin1String("default");
		}

		return false;
	}

	if (m_session.isEmpty())
	{
		m_session = session.path;
	}

	for (int i = 0; i < session.windows.count(); ++i)
	{
		if (window && i == 0)
		{
			window->getWindowsManager()->restore(session.windows.first());
		}
		else
		{
			Application::getInstance()->createWindow(isPrivate, false, session.windows.at(i));
		}
	}

	return true;
}

bool SessionsManager::saveSession(const QString &path, const QString &title, MainWindow *window, bool clean)
{
	QList<MainWindow*> windows;

	if (window)
	{
		windows.append(window);
	}
	else
	{
		windows = Application::getInstance()->getWindows();
	}

	if (windows.isEmpty())
	{
		return false;
	}

	QDir().mkpath(m_profilePath + QLatin1String("/sessions/"));

	const QString sessionPath = getSessionPath(path);
	QString sessionTitle = title;

	if (title.isEmpty())
	{
		QSettings sessionData(sessionPath, QSettings::IniFormat);
		sessionData.setIniCodec("UTF-8");

		sessionTitle = sessionData.value(QLatin1String("Session/title")).toString();;
	}

	QSaveFile file(sessionPath);

	if (!file.open(QIODevice::WriteOnly))
	{
		return false;
	}

	int tabs = 0;
	const QString defaultSearchEngine = SettingsManager::getValue(QLatin1String("Search/DefaultSearchEngine")).toString();
	const QString defaultUserAgent = SettingsManager::getValue(QLatin1String("Network/UserAgent")).toString();
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream << QLatin1String("[Session]\n");
	stream << Utils::formatConfigurationEntry(QLatin1String("title"), sessionTitle, true);

	if (!clean)
	{
		stream << QLatin1String("clean=false\n");
	}

	stream << QLatin1String("windows=") << windows.count() << QLatin1Char('\n');
	stream << QLatin1String("index=1\n\n");

	for (int i = 0; i < windows.count(); ++i)
	{
		const SessionMainWindow sessionEntry = windows.at(i)->getWindowsManager()->getSession();

		tabs += sessionEntry.windows.count();

		stream << QStringLiteral("[%1/Properties]\n").arg(i + 1);
		stream << QLatin1String("groups=0\n");
		stream << QLatin1String("windows=") << sessionEntry.windows.count() << QLatin1Char('\n');
		stream << QLatin1String("index=") << (sessionEntry.index + 1) << QLatin1String("\n\n");

		for (int j = 0; j < sessionEntry.windows.count(); ++j)
		{
			stream << QStringLiteral("[%1/%2/Properties]\n").arg(i + 1).arg(j + 1);

			if (sessionEntry.windows.at(j).searchEngine != defaultSearchEngine)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("searchEngine"), sessionEntry.windows.at(j).searchEngine);
			}

			if (sessionEntry.windows.at(j).userAgent != defaultUserAgent)
			{
				stream << Utils::formatConfigurationEntry(QLatin1String("userAgent"), sessionEntry.windows.at(j).userAgent);
			}

			if (sessionEntry.windows.at(j).pinned)
			{
				stream << QLatin1String("pinned=true\n");
			}

			stream << QLatin1String("history=") << sessionEntry.windows.at(j).history.count() << QLatin1Char('\n');
			stream << QLatin1String("index=") << (sessionEntry.windows.at(j).index +  1) << QLatin1String("\n\n");

			for (int k = 0; k < sessionEntry.windows.at(j).history.count(); ++k)
			{
				stream << QStringLiteral("[%1/%2/History/%3]\n").arg(i + 1).arg(j + 1).arg(k + 1);
				stream << Utils::formatConfigurationEntry(QLatin1String("url"), sessionEntry.windows.at(j).history.at(k).url, true);
				stream << Utils::formatConfigurationEntry(QLatin1String("title"), sessionEntry.windows.at(j).history.at(k).title, true);
				stream << QLatin1String("position=") << sessionEntry.windows.at(j).history.at(k).position.x() << ',' << sessionEntry.windows.at(j).history.at(k).position.y() << QLatin1Char('\n');
				stream << QLatin1String("zoom=") << sessionEntry.windows.at(j).history.at(k).zoom << QLatin1String("\n\n");
			}
		}
	}

	if (tabs == 0)
	{
		file.cancelWriting();

		return false;
	}

	return file.commit();
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString cleanPath = getSessionPath(path, true);

	if (QFile::exists(cleanPath))
	{
		return QFile::remove(cleanPath);
	}

	return false;
}

bool SessionsManager::moveSession(const QString &from, const QString &to)
{
	return QFile::rename(getSessionPath(from), getSessionPath(to));
}

bool SessionsManager::isLastWindow()
{
	return (m_managers.count() == 1);
}

bool SessionsManager::hasUrl(const QUrl &url, bool activate)
{
	for (int i = 0; i < m_managers.count(); ++i)
	{
		if (m_managers.at(i)->hasUrl(url, activate))
		{
			QWidget *window = qobject_cast<QWidget*>(m_managers.at(i)->parent());

			if (window)
			{
				window->raise();
				window->activateWindow();
			}

			return true;
		}
	}

	return false;
}

}
