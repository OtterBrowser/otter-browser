/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WINDOWSMANAGER_H
#define OTTER_WINDOWSMANAGER_H

#include "SessionsManager.h"

#include <QtCore/QUrl>

namespace Otter
{

struct ClosedWindow
{
	SessionWindow window;
	quint64 nextWindow = 0;
	quint64 previousWindow = 0;
	bool isPrivate = false;
};

class Action;
class BookmarksItem;
class ContentsWidget;
class MainWindow;
class Window;

class WindowsManager : public QObject
{
	Q_OBJECT

public:
	explicit WindowsManager(MainWindow *parent);

	void moveWindow(Window *window, MainWindow *mainWindow = nullptr, int index = -1);
	Action* getAction(int identifier);
	Window* getWindowByIndex(int index) const;
	Window* getWindowByIdentifier(quint64 identifier) const;
	QVariant getOption(int identifier) const;
	QString getTitle() const;
	QUrl getUrl() const;
	SessionMainWindow getSession() const;
	QVector<ClosedWindow> getClosedWindows() const;
	int getCurrentWindowIndex() const;
	int getWindowCount(bool onlyPrivate = false) const;
	int getWindowIndex(quint64 identifier) const;
	bool hasUrl(const QUrl &url, bool activate = false);

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void search(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen);
	void restore(int index = 0);
	void restore(const SessionMainWindow &session);
	void clearClosedWindows();
	void setActiveWindowByIndex(int index);
	void setActiveWindowByIdentifier(quint64 identifier);
	void setOption(int identifier, const QVariant &value);

protected:
	void closeAll();
	bool event(QEvent *event) override;

protected slots:
	void open(const QUrl &url = QUrl(), SessionsManager::OpenHints hints = SessionsManager::DefaultOpen);
	void open(BookmarksItem *bookmark, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen);
	void addWindow(Window *window, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen, int index = -1, const QRect &geometry = QRect(), WindowState state = NormalWindowState, bool isAlwaysOnTop = false);
	void removeStoredUrl(const QString &url);
	void handleWindowClose(Window *window);
	void handleWindowIsPinnedChanged(bool isPinned);
	void setTitle(const QString &title);
	void setStatusMessage(const QString &message);
	Window* openWindow(ContentsWidget *widget, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen, int index = -1);

private:
	MainWindow *m_mainWindow;
	QVector<ClosedWindow> m_closedWindows;
	QHash<quint64, Window*> m_windows;
	bool m_isRestored;

signals:
	void titleChanged(const QString &title);
	void windowAdded(quint64 identifier);
	void windowRemoved(quint64 identifier);
	void currentWindowChanged(quint64 identifier);
	void closedWindowsAvailableChanged(bool available);

friend class MainWindow;
};

}

#endif
