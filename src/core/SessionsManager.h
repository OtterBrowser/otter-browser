/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SESSIONSMANAGER_H
#define OTTER_SESSIONSMANAGER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QPoint>
#include <QtCore/QPointer>

namespace Otter
{

struct WindowHistoryEntry
{
	QString url;
	QString title;
	QPoint position;
	int zoom;

	WindowHistoryEntry() : zoom(100) {}
};

struct WindowHistoryInformation
{
	QList<WindowHistoryEntry> entries;
	int index;

	WindowHistoryInformation() : index(-1) {}
};

struct SessionWindow
{
	QString searchEngine;
	QString userAgent;
	QList<WindowHistoryEntry> history;
	int group;
	int index;
	bool pinned;

	SessionWindow() : group(0), index(-1), pinned(false) {}

	QString getUrl() const
	{
		if (index >= 0 && index < history.count())
		{
			return history.at(index).url;
		}

		return QString();
	}

	QString getTitle() const
	{
		if (index >= 0 && index < history.count())
		{
			return (history.at(index).title.isEmpty() ? QCoreApplication::translate("main", "(Untitled)") : history.at(index).title);
		}

		return QCoreApplication::translate("main", "(Untitled)");
	}

	int getZoom() const
	{
		if (index >= 0 && index < history.count())
		{
			return history.at(index).zoom;
		}

		return 100;
	}
};

struct SessionMainWindow
{
	QList<SessionWindow> windows;
	int index;

	SessionMainWindow() : index(-1) {}
};

struct SessionInformation
{
	QString path;
	QString title;
	QList<SessionMainWindow> windows;
	int index;
	bool clean;

	SessionInformation() : index(-1), clean(true) {}
};

class MainWindow;
class WindowsManager;

class SessionsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(const QString &profilePath, const QString &cachePath, QObject *parent = NULL);
	static void clearClosedWindows();
	static void registerWindow(WindowsManager *manager);
	static void storeClosedWindow(WindowsManager *manager);
	static void markSessionModified();
	static void removeStoredUrl(const QString &url);
	static void setActiveWindow(MainWindow *window);
	static SessionsManager* getInstance();
	static WindowsManager* getWindowsManager();
	static QWidget* getActiveWindow();
	static QString getCurrentSession();
	static QString getCachePath();
	static QString getProfilePath();
	static QString getSessionPath(const QString &path, bool bound = false);
	static QStringList getClosedWindows();
	static SessionInformation getSession(const QString &path);
	static QStringList getSessions();
	static bool restoreClosedWindow(int index = -1);
	static bool restoreSession(const SessionInformation &session, MainWindow *window = NULL, bool privateSession = false);
	static bool saveSession(const QString &path = QString(), const QString &title = QString(), MainWindow *window = NULL, bool clean = true);
	static bool deleteSession(const QString &path = QString());
	static bool moveSession(const QString &from, const QString &to);
	static bool isLastWindow();
	static bool hasUrl(const QUrl &url, bool activate = false);

protected:
	explicit SessionsManager(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void scheduleSave();

private:
	int m_saveTimer;

	static SessionsManager *m_instance;
	static QPointer<MainWindow> m_activeWindow;
	static QString m_session;
	static QString m_cachePath;
	static QString m_profilePath;
	static QList<WindowsManager*> m_managers;
	static QList<SessionMainWindow> m_closedWindows;
	static bool m_dirty;

signals:
	void closedWindowsChanged();
	void requestedRemoveStoredUrl(QString url);
};

}

#endif
