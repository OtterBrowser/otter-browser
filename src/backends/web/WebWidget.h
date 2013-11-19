#ifndef OTTER_WEBWIDGET_H
#define OTTER_WEBWIDGET_H

#include "WebBackend.h"
#include "../../core/SessionsManager.h"

#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QWidget>

namespace Otter
{

enum MenuFlag
{
	NoMenu = 0,
	StandardMenu = 1,
	LinkMenu = 2,
	ImageMenu = 4,
	SelectionMenu = 8,
	EditMenu = 16,
	FrameMenu = 32,
	FormMenu = 64
};

Q_DECLARE_FLAGS(MenuFlags, MenuFlag)

class WebWidget : public QWidget
{
	Q_OBJECT

public:
	explicit WebWidget(bool privateWindow, QWidget *parent = NULL);

	virtual void print(QPrinter *printer) = 0;
	virtual WebWidget* clone(QWidget *parent = NULL) = 0;
	virtual QAction* getAction(WebAction action) = 0;
	virtual QUndoStack* getUndoStack() = 0;
	virtual QString getDefaultTextEncoding() const = 0;
	virtual QString getTitle() const = 0;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual HistoryInformation getHistory() const = 0;
	virtual int getZoom() const = 0;
	virtual bool isLoading() const = 0;
	virtual bool isPrivate() const = 0;

public slots:
	virtual void triggerAction(WebAction action, bool checked = false) = 0;
	virtual void setDefaultTextEncoding(const QString &encoding) = 0;
	virtual void setHistory(const HistoryInformation &history) = 0;
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url) = 0;
	void showMenu(const QPoint &position, MenuFlags flags);

signals:
	void statusMessageChanged(const QString &message, int timeout = 5);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
};

}

#endif
