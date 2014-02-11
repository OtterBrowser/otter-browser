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

#ifndef OTTER_WINDOWSMANAGER_H
#define OTTER_WINDOWSMANAGER_H

#include "SessionsManager.h"
#include "../ui/Window.h"

#include <QtCore/QUrl>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMdiArea>

namespace Otter
{

class MdiWidget;
class StatusBarWidget;
class TabBarWidget;
class Window;

class WindowsManager : public QObject
{
	Q_OBJECT

public:
	explicit WindowsManager(MdiWidget *mdi, TabBarWidget *tabBar, StatusBarWidget *statusBar, bool privateSession = false);

	QAction* getAction(WindowAction action);
	Window* getWindow(int index) const;
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QUrl getUrl() const;
	SessionMainWindow getSession() const;
	QList<SessionWindow> getClosedWindows() const;
	int getWindowCount() const;
	int getZoom() const;
	bool canZoom() const;
	bool hasUrl(const QUrl &url, bool activate = false);

public slots:
	void open(const QUrl &url = QUrl(), bool privateWindow = false, bool background = false, bool newWindow = false);
	void search(const QString &query, const QString &engine);
	void close(int index = -1);
	void closeAll();
	void closeOther(int index = -1);
	void restore(int index = 0);
	void restore(const SessionMainWindow &session);
	void print(int index = -1);
	void printPreview(int index = -1);
	void triggerAction(WindowAction action, bool checked = false);
	void clearClosedWindows();
	void setActiveWindow(int index);
	void setDefaultTextEncoding(const QString &encoding);
	void setZoom(int zoom);

protected:
	int getWindowIndex(Window *window) const;

protected slots:
	void printPreview(QPrinter *printer);
	void addWindow(Window *window, bool background = false);
	void addWindow(ContentsWidget *widget, bool detached = false);
	void cloneWindow(int index);
	void detachWindow(int index);
	void pinWindow(int index, bool pin);
	void closeWindow(int index);
	void closeWindow(Window *window);
	void removeStoredUrl(const QString &url);
	void setTitle(const QString &title);

private:
	MdiWidget *m_mdi;
	TabBarWidget *m_tabBar;
	StatusBarWidget *m_statusBar;
	QList<SessionWindow> m_closedWindows;
	int m_printedWindow;
	bool m_isPrivate;
	bool m_isRestored;

signals:
	void requestedAddBookmark(QUrl url, QString title);
	void requestedNewWindow(bool privateSession, bool background, QUrl url);
	void actionsChanged();
	void canZoomChanged(bool can);
	void windowAdded(int index);
	void windowRemoved(int index);
	void currentWindowChanged(int index);
	void windowTitleChanged(QString title);
	void closedWindowsAvailableChanged(bool available);
};

}

#endif
