#include "SessionsManager.h"
#include "SettingsManager.h"
#include "WindowsManager.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

namespace Otter
{

SessionsManager* SessionsManager::m_instance = NULL;
QString SessionsManager::m_session;
QList<WindowsManager*> SessionsManager::m_windows;
QList<SessionInformation> SessionsManager::m_closedWindows;

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

	SessionInformation session;

	for (int i = 0; i < manager->getWindowCount(); ++i)
	{
		session.windows.append(manager->getWindowInformation(i));
	}

	if (!session.windows.isEmpty())
	{
		session.title = session.windows.value(manager->getCurrentWindow()).title();

		m_closedWindows.prepend(session);
	}
}

void SessionsManager::restoreClosedWindow(int index)
{
	Q_UNUSED(index)
//TODO
}

void SessionsManager::restoreSession(const QString &path)
{
	Q_UNUSED(path)
//TODO
}

QString SessionsManager::getCurrentSession()
{
	return m_session;
}

QString SessionsManager::getSessionPath(QString path)
{
	return SettingsManager::getPath() + "/sessions/" + (path.isEmpty() ? "default.ini" : path.replace('/', QString()).replace('\\', QString()));
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		closedWindows.append(m_closedWindows.at(i).title);
	}

	return closedWindows;
}

QList<SessionInformation> SessionsManager::getSesions()
{
	const QString sessionsPath = SettingsManager::getPath() + "/sessions/";
	QStringList entries = QDir().entryList(QDir::Files);

	if (!m_session.isEmpty() && !entries.contains(m_session))
	{
		entries.append(m_session);
	}

	if (!entries.contains("default.ini"))
	{
		entries.append("default.ini");
	}

	QMultiMap<QString, SessionInformation> sessions;

	for (int i = 0; i < entries.count(); ++i)
	{
		QString path = entries.at(i);

		if (!QFileInfo(path).isAbsolute())
		{
			path = sessionsPath + path;
		}

		const QSettings sessionData(path, QSettings::IniFormat);
		const int windows = sessionData.value("Session/windows", 0).toInt();
		SessionInformation information;
		information.path = path;
		information.title = sessionData.value("Session/title", ((entries.at(i) == "default") ? tr("Default") : tr("(Untitled)"))).toString();
		information.index = sessionData.value("Session/index", 0).toInt();

		for (int j = 0; j < windows; ++j)
		{
			const int history = sessionData.value(QString("%1/Properties/history").arg(j), 0).toInt();
			SessionEntry sessionEntry;
			sessionEntry.group = sessionData.value(QString("%1/Properties/group").arg(j), 0).toInt();
			sessionEntry.window = sessionData.value(QString("%1/Properties/window").arg(j), 1).toInt();
			sessionEntry.index = sessionData.value(QString("%1/Properties/index").arg(j), 1).toInt();
			sessionEntry.pinned = sessionData.value(QString("%1/Properties/pinned").arg(j), false).toBool();

			for (int k = 0; k < history; ++k)
			{
				HistoryEntry historyEntry;
				historyEntry.url = sessionData.value(QString("%1/History/%2/url").arg(j).arg(k), 0).toString();
				historyEntry.title = sessionData.value(QString("%1/History/%2/title").arg(j).arg(k), 1).toString();
				historyEntry.position = sessionData.value(QString("%1/History/%2/position").arg(j).arg(k), 1).toPoint();

				sessionEntry.history.append(historyEntry);
			}

			information.windows.append(sessionEntry);
		}

		sessions.insert(information.title, information);
	}

	return sessions.values();
}

bool SessionsManager::saveSession(const QString &path)
{
	Q_UNUSED(path)
//TODO
	return false;
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString cleanPath = getSessionPath(path);

	if (QFile::exists(cleanPath))
	{
		return QFile::remove(cleanPath);
	}

	return false;
}

bool SessionsManager::moveSession(const QString &from, const QString &to)
{
	Q_UNUSED(from)
	Q_UNUSED(to)
//TODO
	return false;
}

}
