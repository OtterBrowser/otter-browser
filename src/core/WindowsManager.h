#ifndef OTTER_WINDOWSMANAGER_H
#define OTTER_WINDOWSMANAGER_H

#include "SessionsManager.h"
#include "../backends/web/WebBackend.h"

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

	Window* getWindow(int index = -1) const;
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	SessionEntry getWindowInformation(int index) const;
	QList<SessionEntry> getClosedWindows() const;
	int getWindowCount() const;
	int getCurrentWindow() const;
	int getZoom() const;
	bool canUndo() const;
	bool canRedo() const;

public slots:
	void open(const QUrl &url = QUrl(), bool privateWindow = false);
	void close(int index = -1);
	void closeOther(int index = -1);
	void print(int index = -1);
	void printPreview(int index = -1);
	void triggerAction(WebAction action, bool checked = false);
	void setDefaultTextEncoding(const QString &encoding);
	void setZoom(int zoom);

protected:
	int getWindowIndex(Window *window) const;

protected slots:
	void printPreview(QPrinter *printer);
	void addWindow(Window *window);
	void cloneWindow(int index);
	void pinWindow(int index, bool pin);
	void closeWindow(int index);
	void setCurrentWindow(int index);
	void setTitle(const QString &title);

private:
	QMdiArea *m_area;
	TabBarWidget *m_tabBar;
	StatusBarWidget *m_statusBar;
	QList<SessionEntry> m_closedWindows;
	int m_currentWindow;
	int m_printedWindow;
	bool m_privateSession;

signals:
	void windowAdded(int index);
	void windowRemoved(int index);
	void currentWindowChanged(int index);
	void windowTitleChanged(QString title);
	void undoTextChanged(QString undoText);
	void redoTextChanged(QString redoText);
	void canUndoChanged(bool canUndo);
	void canRedoChanged(bool canRedo);
	void closedWindowsAvailableChanged(bool available);
};

}

#endif
