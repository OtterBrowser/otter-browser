/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SessionsManager.h"
#include "ActionsManager.h"
#include "Application.h"
#include "JsonSettings.h"
#include "WindowsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

namespace Otter
{

SessionsManager* SessionsManager::m_instance(nullptr);
QString SessionsManager::m_sessionPath;
QString SessionsManager::m_sessionTitle;
QString SessionsManager::m_cachePath;
QString SessionsManager::m_profilePath;
QList<SessionMainWindow> SessionsManager::m_closedWindows;
bool SessionsManager::m_isDirty(false);
bool SessionsManager::m_isPrivate(false);
bool SessionsManager::m_isReadOnly(false);

SessionsManager::SessionsManager(QObject *parent) : QObject(parent),
	m_saveTimer(0)
{
}

void SessionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_saveTimer)
	{
		m_isDirty = false;

		killTimer(m_saveTimer);

		m_saveTimer = 0;

		if (!m_isPrivate)
		{
			saveSession(QString(), QString(), nullptr, false);
		}
	}
}

void SessionsManager::createInstance(const QString &profilePath, const QString &cachePath, bool isPrivate, bool isReadOnly, QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new SessionsManager(parent);
		m_cachePath = cachePath;
		m_profilePath = profilePath;
		m_isPrivate = isPrivate;
		m_isReadOnly = isReadOnly;
	}
}

void SessionsManager::scheduleSave()
{
	if (m_saveTimer == 0 && !m_isPrivate)
	{
		m_saveTimer = startTimer(1000);
	}
}

void SessionsManager::clearClosedWindows()
{
	m_closedWindows.clear();

	emit m_instance->closedWindowsChanged();
}

void SessionsManager::storeClosedWindow(MainWindow *window)
{
	if (!window)
	{
		return;
	}

	SessionMainWindow session(window->getWindowsManager()->getSession());
	session.geometry = window->saveGeometry();

	if (!session.windows.isEmpty())
	{
		m_closedWindows.prepend(session);

		emit m_instance->closedWindowsChanged();
	}
}

void SessionsManager::markSessionModified()
{
	if (!m_isPrivate && !m_isDirty && m_sessionPath == QLatin1String("default"))
	{
		m_isDirty = true;

		m_instance->scheduleSave();
	}
}

void SessionsManager::removeStoredUrl(const QString &url)
{
	emit m_instance->requestedRemoveStoredUrl(url);
}

SessionsManager* SessionsManager::getInstance()
{
	return m_instance;
}

QString SessionsManager::getCurrentSession()
{
	return m_sessionPath;
}

QString SessionsManager::getCachePath()
{
	return (m_isReadOnly ? QString() : m_cachePath);
}

QString SessionsManager::getProfilePath()
{
	return m_profilePath;
}

QString SessionsManager::getReadableDataPath(const QString &path, bool forceBundled)
{
	const QString writablePath(getWritableDataPath(path));

	return ((!forceBundled && QFile::exists(writablePath)) ? writablePath : QLatin1String(":/") + (path.contains(QLatin1Char('/')) ? QString() : QLatin1String("other")) + QDir::separator() + path);
}

QString SessionsManager::getWritableDataPath(const QString &path)
{
	return QDir::toNativeSeparators(m_profilePath + QDir::separator() + path);
}

QString SessionsManager::getSessionPath(const QString &path, bool isBound)
{
	QString cleanPath(path);

	if (cleanPath.isEmpty())
	{
		cleanPath = QLatin1String("default.json");
	}
	else
	{
		if (!cleanPath.endsWith(QLatin1String(".json")))
		{
			cleanPath += QLatin1String(".json");
		}

		if (isBound)
		{
			cleanPath = cleanPath.replace(QLatin1Char('/'), QString()).replace(QLatin1Char('\\'), QString());
		}
		else if (QFileInfo(cleanPath).isAbsolute())
		{
			return cleanPath;
		}
	}

	return QDir::toNativeSeparators(m_profilePath + QLatin1String("/sessions/") + cleanPath);
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	SessionInformation session;
	const JsonSettings settings(getSessionPath(path));

	if (settings.isNull())
	{
		return session;
	}

	const int defaultZoom(SettingsManager::getValue(SettingsManager::Content_DefaultZoomOption).toInt());
	const QJsonArray mainWindowsArray(settings.object().value(QLatin1String("windows")).toArray());

	session.path = path;
	session.title = settings.object().value(QLatin1String("title")).toString((path == QLatin1String("default")) ? tr("Default") : tr("(Untitled)"));
	session.index = (settings.object().value(QLatin1String("currentIndex")).toInt(1) - 1);
	session.isClean = settings.object().value(QLatin1String("isClean")).toBool(true);

	for (int i = 0; i < mainWindowsArray.count(); ++i)
	{
		const QJsonObject mainWindowObject(mainWindowsArray.at(i).toObject());
		const QJsonArray windowsArray(mainWindowObject.value(QLatin1String("windows")).toArray());
		SessionMainWindow sessionMainWindow;
		sessionMainWindow.geometry = QByteArray::fromBase64(mainWindowObject.value(QLatin1String("geometry")).toString().toLatin1());
		sessionMainWindow.index = (mainWindowObject.value(QLatin1String("currentIndex")).toInt(1) - 1);

		for (int j = 0; j < windowsArray.count(); ++j)
		{
			const QJsonObject windowObject(windowsArray.at(j).toObject());
			const QJsonArray windowHistoryArray(windowObject.value(QLatin1String("history")).toArray());
			const QString state(windowObject.value(QLatin1String("state")).toString());
			const QStringList geometry(windowObject.value(QLatin1String("geometry")).toString().split(QLatin1Char(',')));
			SessionWindow sessionWindow;
			sessionWindow.geometry = ((geometry.count() == 4) ? QRect(geometry.at(0).simplified().toInt(), geometry.at(1).simplified().toInt(), geometry.at(2).simplified().toInt(), geometry.at(3).simplified().toInt()) : QRect());
			sessionWindow.state = ((state == QLatin1String("maximized")) ? MaximizedWindowState : ((state == QLatin1String("minimized")) ? MinimizedWindowState : NormalWindowState));
			sessionWindow.historyIndex = (windowObject.value(QLatin1String("currentIndex")).toInt(1) - 1);
			sessionWindow.isAlwaysOnTop = windowObject.value(QLatin1String("isAlwaysOnTop")).toBool(false);
			sessionWindow.isPinned = windowObject.value(QLatin1String("isPinned")).toBool(false);

			if (windowObject.contains(QLatin1String("options")))
			{
				const QJsonObject optionsObject(windowObject.value(QLatin1String("options")).toObject());
				QJsonObject::const_iterator iterator;

				for (iterator = optionsObject.constBegin(); iterator != optionsObject.constEnd(); ++iterator)
				{
					const int optionIdentifier(SettingsManager::getOptionIdentifier(iterator.key()));

					if (optionIdentifier >= 0)
					{
						sessionWindow.overrides[optionIdentifier] = iterator.value().toVariant();
					}
				}
			}

			for (int k = 0; k < windowHistoryArray.count(); ++k)
			{
				const QJsonObject historyEntryObject(windowHistoryArray.at(k).toObject());
				const QStringList position(historyEntryObject.value(QLatin1String("position")).toString().split(QLatin1Char(',')));
				WindowHistoryEntry historyEntry;
				historyEntry.url = historyEntryObject.value(QLatin1String("url")).toString();
				historyEntry.title = historyEntryObject.value(QLatin1String("title")).toString();
				historyEntry.position = ((position.count() == 2) ? QPoint(position.at(0).simplified().toInt(), position.at(1).simplified().toInt()) : QPoint(0, 0));
				historyEntry.zoom = historyEntryObject.value(QLatin1String("zoom")).toInt(defaultZoom);

				sessionWindow.history.append(historyEntry);
			}

			if (sessionWindow.historyIndex < 0 || sessionWindow.historyIndex >= sessionWindow.history.count())
			{
				sessionWindow.historyIndex = (sessionWindow.history.count() - 1);
			}

			sessionMainWindow.windows.append(sessionWindow);
		}

		if (sessionMainWindow.index < 0 || sessionMainWindow.index >= sessionMainWindow.windows.count())
		{
			sessionMainWindow.index = (sessionMainWindow.windows.count() - 1);
		}

		session.windows.append(sessionMainWindow);
	}

	if (session.index < 0 || session.index >= session.windows.count())
	{
		session.index = (session.windows.count() - 1);
	}

	return session;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const SessionMainWindow window(m_closedWindows.at(i));
		const QString title(window.windows.value(window.index, SessionWindow()).getTitle());

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

QStringList SessionsManager::getSessions()
{
	QStringList entries(QDir(m_profilePath + QLatin1String("/sessions/")).entryList(QStringList(QLatin1String("*.json")), QDir::Files));

	for (int i = 0; i < entries.count(); ++i)
	{
		entries[i] = QFileInfo(entries.at(i)).completeBaseName();
	}

	if (!m_sessionPath.isEmpty() && !entries.contains(m_sessionPath))
	{
		entries.append(m_sessionPath);
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

	Application::createWindow(Application::NoFlags, false, m_closedWindows.value(index, SessionMainWindow()));

	m_closedWindows.removeAt(index);

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const SessionInformation &session, MainWindow *window, bool isPrivate)
{
	if (session.windows.isEmpty())
	{
		if (m_sessionPath.isEmpty() && session.path == QLatin1String("default"))
		{
			m_sessionPath = QLatin1String("default");
		}

		return false;
	}

	if (m_sessionPath.isEmpty())
	{
		m_sessionPath = session.path;
		m_sessionTitle = session.title;
	}

	for (int i = 0; i < session.windows.count(); ++i)
	{
		if (window && i == 0)
		{
			window->getWindowsManager()->restore(session.windows.first());
		}
		else
		{
			Application::createWindow((isPrivate ? Application::PrivateFlag : Application::NoFlags), false, session.windows.at(i));
		}
	}

	return true;
}

bool SessionsManager::saveSession(const QString &path, const QString &title, MainWindow *window, bool isClean)
{
	if (m_isPrivate && path.isEmpty())
	{
		return false;
	}

	SessionInformation session;
	session.path = getSessionPath(path);
	session.title = (title.isEmpty() ? m_sessionTitle : title);
	session.isClean = isClean;

	QList<MainWindow*> windows;

	if (window)
	{
		windows.append(window);
	}
	else
	{
		windows = Application::getWindows();
	}

	for (int i = 0; i < windows.count(); ++i)
	{
		session.windows.append(windows.at(i)->getWindowsManager()->getSession());
		session.windows.last().geometry = windows.at(i)->saveGeometry();
	}

	return saveSession(session);
}

bool SessionsManager::saveSession(const SessionInformation &session)
{
	QDir().mkpath(m_profilePath + QLatin1String("/sessions/"));

	if (session.windows.isEmpty())
	{
		return false;
	}

	QString path(session.path);

	if (path.isEmpty())
	{
		path = m_profilePath + QLatin1String("/sessions/") + session.title + QLatin1String(".json");

		if (QFileInfo(path).exists())
		{
			int i = 1;

			while (QFileInfo(m_profilePath + QLatin1String("/sessions/") + session.title + QString::number(i) + QLatin1String(".json")).exists())
			{
				++i;
			}

			path = m_profilePath + QLatin1String("/sessions/") + session.title + QString::number(i) + QLatin1String(".json");
		}
	}

	const QStringList excludedOptions(SettingsManager::getValue(SettingsManager::Sessions_OptionsExludedFromSavingOption).toStringList());
	QJsonArray mainWindowsArray;
	QJsonObject sessionObject;
	sessionObject.insert(QLatin1String("title"), session.title);
	sessionObject.insert(QLatin1String("currentIndex"), 1);

	if (!session.isClean)
	{
		sessionObject.insert(QLatin1String("isClean"), false);
	}

	for (int i = 0; i < session.windows.count(); ++i)
	{
		const SessionMainWindow sessionEntry(session.windows.at(i));
		QJsonObject mainWindowObject;
		mainWindowObject.insert(QLatin1String("currentIndex"), (sessionEntry.index + 1));
		mainWindowObject.insert(QLatin1String("geometry"), QString(sessionEntry.geometry.toBase64()));

		QJsonArray windowsArray;

		for (int j = 0; j < sessionEntry.windows.count(); ++j)
		{
			QJsonObject windowObject;

			if (!sessionEntry.windows.at(j).overrides.isEmpty())
			{
				const QHash<int, QVariant> windowOptions(sessionEntry.windows.at(j).overrides);
				QHash<int, QVariant>::const_iterator optionsIterator;
				QJsonObject optionsObject;

				for (optionsIterator = windowOptions.constBegin(); optionsIterator != windowOptions.constEnd(); ++optionsIterator)
				{
					const QString optionName(SettingsManager::getOptionName(optionsIterator.key()));

					if (!optionName.isEmpty() && !excludedOptions.contains(optionName))
					{
						optionsObject.insert(optionName, QJsonValue::fromVariant(optionsIterator.value()));
					}
				}

				windowObject.insert(QLatin1String("options"), optionsObject);
			}

			windowObject.insert(QLatin1String("geometry"), QStringLiteral("%1, %2, %3, %4").arg(sessionEntry.windows.at(j).geometry.x()).arg(sessionEntry.windows.at(j).geometry.y()).arg(sessionEntry.windows.at(j).geometry.width()).arg(sessionEntry.windows.at(j).geometry.height()));
			windowObject.insert(QLatin1String("currentIndex"), (sessionEntry.windows.at(j).historyIndex + 1));

			if (sessionEntry.windows.at(j).state == MaximizedWindowState)
			{
				windowObject.insert(QLatin1String("state"), QLatin1String("maximized"));
			}
			else if (sessionEntry.windows.at(j).state == MinimizedWindowState)
			{
				windowObject.insert(QLatin1String("state"), QLatin1String("minimized"));
			}
			else
			{
				windowObject.insert(QLatin1String("state"), QLatin1String("normal"));
			}

			if (sessionEntry.windows.at(j).isAlwaysOnTop)
			{
				windowObject.insert(QLatin1String("isAlwaysOnTop"), true);
			}

			if (sessionEntry.windows.at(j).isPinned)
			{
				windowObject.insert(QLatin1String("isPinned"), true);
			}

			QJsonArray windowHistoryArray;

			for (int k = 0; k < sessionEntry.windows.at(j).history.count(); ++k)
			{
				QJsonObject historyEntryObject;
				historyEntryObject.insert(QLatin1String("url"), sessionEntry.windows.at(j).history.at(k).url);
				historyEntryObject.insert(QLatin1String("title"), sessionEntry.windows.at(j).history.at(k).title);
				historyEntryObject.insert(QLatin1String("position"), QString::number(sessionEntry.windows.at(j).history.at(k).position.x()) + QLatin1String(", ") + QString::number(sessionEntry.windows.at(j).history.at(k).position.y()));
				historyEntryObject.insert(QLatin1String("zoom"), sessionEntry.windows.at(j).history.at(k).zoom);

				windowHistoryArray.append(historyEntryObject);
			}

			windowObject.insert(QLatin1String("history"), windowHistoryArray);

			windowsArray.append(windowObject);
		}

		mainWindowObject.insert(QLatin1String("windows"), windowsArray);

		mainWindowsArray.append(mainWindowObject);
	}

	sessionObject.insert(QLatin1String("windows"), mainWindowsArray);

	JsonSettings settings;
	settings.setObject(sessionObject);

	return settings.save(path);
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString cleanPath(getSessionPath(path, true));

	if (QFile::exists(cleanPath))
	{
		return QFile::remove(cleanPath);
	}

	return false;
}

bool SessionsManager::isPrivate()
{
	return m_isPrivate;
}

bool SessionsManager::isReadOnly()
{
	return m_isReadOnly;
}

bool SessionsManager::hasUrl(const QUrl &url, bool activate)
{
	const QList<MainWindow*> windows(Application::getWindows());

	for (int i = 0; i < windows.count(); ++i)
	{
		if (windows.at(i)->getWindowsManager()->hasUrl(url, activate))
		{
			QWidget *window(qobject_cast<QWidget*>(windows.at(i)->parent()));

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
