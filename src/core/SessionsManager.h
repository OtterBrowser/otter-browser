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

struct SessionEntry
{
	QList<HistoryEntry> history;
	int group;
	int window;
	int index;
	bool pinned;

	SessionEntry() : group(0), window(1), index(-1), pinned(false) {}

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

struct SessionInformation
{
	QString path;
	QString title;
	QList<SessionEntry> windows;
	int index;

	SessionInformation() : index(0) {}
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
	static QString getCurrentSession();
	static QString getSessionPath(QString path);
	static QStringList getClosedWindows();
	static QList<SessionInformation> getSesions();
	static bool modifySession(const SessionInformation &information, const QString &path = QString());
	static bool newSession(const QString &path = QString());
	static bool cloneSession(const QString &path = QString());
	static bool clearSession(const QString &path = QString());
	static bool deleteSession(const QString &path = QString());

private:
	explicit SessionsManager(QObject *parent = NULL);

	static SessionsManager *m_instance;
	static QString m_session;
	static QList<WindowsManager*> m_windows;
	static QList<SessionInformation > m_closedWindows;
};

}

#endif
