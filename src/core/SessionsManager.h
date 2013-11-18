#ifndef OTTER_SESSIONSMANAGER_H
#define OTTER_SESSIONSMANAGER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPoint>

namespace Otter
{

struct HistoryEntry
{
	QString url;
	QString title;
	QPoint position;
	int zoom;

	HistoryEntry() : zoom(100) {}
};

struct HistoryInformation
{
	QList<HistoryEntry> entries;
	int index;

	HistoryInformation() : index(-1) {}
};

struct SessionWindow
{
	QList<HistoryEntry> history;
	int group;
	int index;
	bool pinned;

	SessionWindow() : group(0), index(-1), pinned(false) {}

	QString url() const
	{
		if (index >= 0 && index < history.count())
		{
			return history.at(index).url;
		}

		return QString();
	}

	QString title() const
	{
		if (index >= 0 && index < history.count())
		{
			return history.at(index).title;
		}

		return QCoreApplication::translate("main", "(Untitled)");
	}

	int zoom() const
	{
		if (index >= 0 && index < history.count())
		{
			return history.at(index).zoom;
		}

		return 100;
	}
};

struct SessionEntry
{
	QList<SessionWindow> windows;
	int index;

	SessionEntry() : index(-1) {}
};

struct SessionInformation
{
	QString path;
	QString title;
	QList<SessionEntry> windows;
	int index;

	SessionInformation() : index(-1) {}
};

class WindowsManager;

class SessionsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void registerWindow(WindowsManager *manager);
	static void storeClosedWindow(WindowsManager *manager);
	static SessionsManager* getInstance();
	static QString getCurrentSession();
	static QString getSessionPath(const QString &path, bool bound = false);
	static QStringList getClosedWindows();
	static SessionInformation getSession(const QString &path);
	static QStringList getSessions();
	static bool restoreClosedWindow(int index = -1);
	static bool restoreSession(const QString &path = QString());
	static bool saveSession(const QString &path = QString());
	static bool deleteSession(const QString &path = QString());
	static bool moveSession(const QString &from, const QString &to);

private:
	explicit SessionsManager(QObject *parent = NULL);

	static SessionsManager *m_instance;
	static QString m_session;
	static QList<WindowsManager*> m_windows;
	static QList<SessionEntry> m_closedWindows;

signals:
	void closedWindowsChanged();
};

}

#endif
