/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionsManager.h"
#include "SessionsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QStandardItem>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMdiArea>

namespace Otter
{

enum OpenHint
{
	DefaultOpen = 0,
	PrivateOpen = 1,
	CurrentTabOpen = 2,
	NewTabOpen = 4,
	NewWindowOpen = 8,
	BackgroundOpen = 16,
	EndOpen = 32,
	NewBackgroundTabOpen = (NewTabOpen | BackgroundOpen),
	NewBackgroundTabEndOpen = (NewTabOpen | EndOpen | BackgroundOpen),
	NewBackgroundWindowOpen = (NewWindowOpen | BackgroundOpen),
	NewBackgroundWindowEndOpen = (NewWindowOpen | EndOpen | BackgroundOpen),
	NewPrivateTabOpen = (NewTabOpen | PrivateOpen),
	NewPrivateBackgroundTabOpen = (NewBackgroundTabOpen | PrivateOpen),
	NewPrivateWindowOpen = (NewWindowOpen | PrivateOpen),
	NewPrivateBackgroundWindowOpen = (NewBackgroundWindowOpen | PrivateOpen)
};

Q_DECLARE_FLAGS(OpenHints, OpenHint)

struct ClosedWindow
{
	SessionWindow window;
	quint64 nextWindow;
	quint64 previousWindow;

	ClosedWindow() : nextWindow(0), previousWindow(0) {}
};

class BookmarksItem;
class ContentsWidget;
class MainWindow;
class Window;

class WindowsManager : public QObject
{
	Q_OBJECT

public:
	explicit WindowsManager(bool isPrivate, MainWindow *parent);

	Action* getAction(int identifier);
	Window* getWindowByIndex(int index) const;
	Window* getWindowByIdentifier(quint64 identifier) const;
	QVariant getOption(const QString &key) const;
	QString getTitle() const;
	QUrl getUrl() const;
	SessionMainWindow getSession() const;
	QList<ClosedWindow> getClosedWindows() const;
	static OpenHints calculateOpenHints(Qt::KeyboardModifiers modifiers = Qt::NoModifier, Qt::MouseButton button = Qt::LeftButton, OpenHints hints = DefaultOpen);
	int getWindowCount(bool onlyPrivate = false) const;
	int getWindowIndex(quint64 identifier) const;
	int getZoom() const;
	bool canZoom() const;
	bool isPrivate() const;
	bool hasUrl(const QUrl &url, bool activate = false);

public slots:
	void triggerAction(int identifier, bool checked = false);
	void open(const QUrl &url = QUrl(), OpenHints hints = DefaultOpen);
	void open(BookmarksItem *bookmark, OpenHints hints = DefaultOpen);
	void search(const QString &query, const QString &engine, OpenHints hints = DefaultOpen);
	void close(int index);
	void closeOther(int index = -1);
	void closeAll();
	void restore(int index = 0);
	void restore(const SessionMainWindow &session);
	void clearClosedWindows();
	void setActiveWindowByIndex(int index);
	void setActiveWindowByIdentifier(quint64 identifier);
	void setOption(const QString &key, const QVariant &value);
	void setZoom(int zoom);

protected:
	void openTab(const QUrl &url, OpenHints hints = DefaultOpen);
	void gatherBookmarks(QStandardItem *branch);
	bool event(QEvent *event);

protected slots:
	void addWindow(Window *window, OpenHints hints = DefaultOpen, int index = -1);
	void openWindow(ContentsWidget *widget, OpenHints hints = DefaultOpen);
	void cloneWindow(int index);
	void detachWindow(int index);
	void pinWindow(int index, bool pin);
	void removeStoredUrl(const QString &url);
	void handleWindowClose(Window *window);
	void setTitle(const QString &title);
	void setStatusMessage(const QString &message);

private:
	MainWindow *m_mainWindow;
	QList<ClosedWindow> m_closedWindows;
	QList<QUrl> m_bookmarksToOpen;
	bool m_isPrivate;
	bool m_isRestored;

signals:
	void requestedAddBookmark(QUrl url, QString title, QString description);
	void requestedNewWindow(bool isPrivate, bool inBackground, QUrl url);
	void canZoomChanged(bool can);
	void zoomChanged(int zoom);
	void windowAdded(qint64 identifier);
	void windowRemoved(qint64 identifier);
	void currentWindowChanged(qint64 identifier);
	void windowTitleChanged(QString title);
	void closedWindowsAvailableChanged(bool available);
};

}

#endif
