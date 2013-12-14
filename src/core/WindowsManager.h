#ifndef OTTER_WINDOWSMANAGER_H
#define OTTER_WINDOWSMANAGER_H

#include "SessionsManager.h"
#include "../ui/Window.h"

#include <QtCore/QUrl>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMdiArea>

namespace Otter
{

class StatusBarWidget;
class TabBarWidget;
class Window;

class WindowsManager : public QObject
{
	Q_OBJECT

public:
	explicit WindowsManager(QMdiArea *area, TabBarWidget *tabBar, StatusBarWidget *statusBar, bool privateSession = false);

	QAction* getAction(WindowAction action);
	Window* getWindow(int index = -1) const;
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QUrl getUrl() const;
	SessionEntry getSession() const;
	QList<SessionWindow> getClosedWindows() const;
	int getWindowCount() const;
	int getCurrentWindow() const;
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
	void restore(const QList<SessionWindow> &windows);
	void print(int index = -1);
	void printPreview(int index = -1);
	void triggerAction(WindowAction action, bool checked = false);
	void clearClosedWindows();
	void setCurrentWindow(int index);
	void setDefaultTextEncoding(const QString &encoding);
	void setZoom(int zoom);

protected:
	int getWindowIndex(Window *window) const;

protected slots:
	void printPreview(QPrinter *printer);
	void addWindow(Window *window, bool background = false);
	void addWindow(ContentsWidget *widget);
	void cloneWindow(int index);
	void pinWindow(int index, bool pin);
	void closeWindow(int index);
	void closeWindow(Window *window);
	void setTitle(const QString &title);

private:
	QMdiArea *m_area;
	TabBarWidget *m_tabBar;
	StatusBarWidget *m_statusBar;
	QList<SessionWindow> m_closedWindows;
	int m_currentWindow;
	int m_printedWindow;
	bool m_privateSession;

signals:
	void requestedAddBookmark(QUrl url);
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
