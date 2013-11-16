#include "SessionsManager.h"
#include "WindowsManager.h"

namespace Otter
{

SessionsManager* SessionsManager::m_instance = NULL;
QString SessionsManager::m_session;
QList<WindowsManager*> SessionsManager::m_windows;
QList<QPair<QString, QList<SessionEntry> > > SessionsManager::m_closedWindows;

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

QString SessionsManager::getCurrentSession()
{
	return m_session;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		closedWindows.append(m_closedWindows.at(i).first);
	}

	return closedWindows;
}

}
