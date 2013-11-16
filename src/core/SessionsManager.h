#ifndef OTTER_SESSIONSMANAGER_H
#define OTTER_SESSIONSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QPoint>

namespace Otter
{

struct HistoryEntry
{
	QString url;
	QString title;
	QPoint position;
};

struct SessionEntry
{
	int group;
	int window;
	int index;
	bool pinned;
	QList<HistoryEntry> history;

	SessionEntry() : group(0), window(1), index(0), pinned(false) {}
};

struct SessionInformation
{
	QString path;
	QString title;
	int windows;

	SessionInformation() : windows(0) {}
};

class WindowsManager;

class SessionsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void registerWindow(WindowsManager *manager);
	static void storeClosedWindow(WindowsManager *manager);
	static void storeSession(const QString &path = QString());
	static void restoreClosedWindow(int index = -1);
	static void restoreSession(const QString &path = QString());
	static void modifySession(const SessionInformation &information, const QString &path = QString());
	static void cloneSession(const QString &path = QString());
	static void clearSession(const QString &path = QString());
	static void deleteSession(const QString &path = QString());
	static QString getCurrentSession();
	static QStringList getClosedWindows();
	static QList<SessionInformation> getSesions();

private:
	explicit SessionsManager(QObject *parent = NULL);

	static SessionsManager *m_instance;
	static QString m_session;
	static QList<WindowsManager*> m_windows;
	static QList<QPair<QString, QList<SessionEntry> > > m_closedWindows;
};

}

#endif
