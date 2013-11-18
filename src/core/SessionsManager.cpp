#include "SessionsManager.h"
#include "Application.h"
#include "SettingsManager.h"
#include "WindowsManager.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

namespace Otter
{

SessionsManager* SessionsManager::m_instance = NULL;
QString SessionsManager::m_session;
QList<WindowsManager*> SessionsManager::m_windows;
QList<SessionEntry> SessionsManager::m_closedWindows;

SessionsManager::SessionsManager(QObject *parent) : QObject(parent)
{
}

void SessionsManager::createInstance(QObject *parent)
{
	m_instance = new SessionsManager(parent);
}

void SessionsManager::registerWindow(WindowsManager *manager)
{
	if (manager)
	{
		m_windows.append(manager);
	}
}

void SessionsManager::storeClosedWindow(WindowsManager *manager)
{
	if (!manager)
	{
		return;
	}

	SessionEntry session = manager->getSession();

	if (!session.windows.isEmpty())
	{
		m_closedWindows.prepend(session);

		emit m_instance->closedWindowsChanged();
	}
}

SessionsManager *SessionsManager::getInstance()
{
	return m_instance;
}

QString SessionsManager::getCurrentSession()
{
	return m_session;
}

QString SessionsManager::getSessionPath(const QString &path, bool bound)
{
	QString cleanPath = path;

	if (cleanPath.isEmpty())
	{
		cleanPath = "default.ini";
	}
	else
	{
		if (!cleanPath.endsWith(".ini"))
		{
			cleanPath += ".ini";
		}

		if (bound)
		{
			cleanPath = cleanPath.replace('/', QString()).replace('\\', QString());
		}
		else if (QFileInfo(cleanPath).isAbsolute())
		{
			return cleanPath;
		}
	}

	return SettingsManager::getPath() + "/sessions/" + cleanPath;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const SessionEntry window = m_closedWindows.at(i);
		const QString title = window.windows.value(window.index, SessionWindow()).title();

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	const QString sessionPath = getSessionPath(path);
	const QSettings sessionData(sessionPath, QSettings::IniFormat);
	const int windows = sessionData.value("Session/windows", 0).toInt();
	SessionInformation session;
	session.path = sessionPath;
	session.title = sessionData.value("Session/title", ((path == "default") ? tr("Default") : tr("(Untitled)"))).toString();
	session.index = (sessionData.value("Session/index", 1).toInt() - 1);

	for (int i = 1; i <= windows; ++i)
	{
		const int tabs = sessionData.value(QString("%1/Properties/windows").arg(i), 0).toInt();
		SessionEntry sessionEntry;
		sessionEntry.index = (sessionData.value(QString("%1/Properties/index").arg(i), 1).toInt() - 1);

		for (int j = 1; j <= tabs; ++j)
		{
			const int history = sessionData.value(QString("%1/%2/Properties/history").arg(i).arg(j), 0).toInt();
			SessionWindow sessionWindow;
			sessionWindow.group = sessionData.value(QString("%1/%2/Properties/group").arg(i).arg(j), 0).toInt();
			sessionWindow.index = (sessionData.value(QString("%1/%2/Properties/index").arg(i).arg(j), 1).toInt() - 1);
			sessionWindow.pinned = sessionData.value(QString("%1/%2/Properties/pinned").arg(i).arg(j), false).toBool();

			for (int k = 1; k <= history; ++k)
			{
				HistoryEntry historyEntry;
				historyEntry.url = sessionData.value(QString("%1/%2/History/%3/url").arg(i).arg(j).arg(k), 0).toString();
				historyEntry.title = sessionData.value(QString("%1/%2/History/%3/title").arg(i).arg(j).arg(k), 1).toString();
				historyEntry.position = sessionData.value(QString("%1/%2/History/%3/position").arg(i).arg(j).arg(k), 1).toPoint();

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
	const QString sessionsPath = SettingsManager::getPath() + "/sessions/";
	QStringList entries = QDir().entryList(QDir::Files);

	for (int i = 0; i < entries.count(); ++i)
	{
		entries[i] = QFileInfo(entries.at(i)).completeBaseName();
	}

	if (!m_session.isEmpty() && !entries.contains(m_session))
	{
		entries.append(m_session);
	}

	if (!entries.contains("default"))
	{
		entries.append("default");
	}

	entries.sort();

	return entries;
}

bool SessionsManager::restoreClosedWindow(int index)
{
	Application *application = qobject_cast<Application*>(QCoreApplication::instance());

	if (!application)
	{
		return false;
	}

	if (index < 0)
	{
		index = 0;
	}

	application->createWindow(false, m_closedWindows.value(index, SessionEntry()));

	m_closedWindows.removeAt(index);

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const QString &path)
{
	const SessionInformation session = getSession(path);

	if (session.windows.isEmpty())
	{
		return false;
	}

	m_session = path;
	m_closedWindows = session.windows;

	const int windows = m_closedWindows.count();

	for (int i = 0; i < windows; ++i)
	{
		restoreClosedWindow();
	}

	return true;
}

bool SessionsManager::saveSession(const QString &path)
{
	Q_UNUSED(path)
//TODO
	return false;
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

}
