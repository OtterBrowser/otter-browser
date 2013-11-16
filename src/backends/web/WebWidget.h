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
	explicit WebWidget(QWidget *parent = NULL);

	virtual void print(QPrinter *printer);
	virtual WebWidget* clone(QWidget *parent = NULL);
	virtual QAction* getAction(WebAction action);
	virtual QUndoStack* getUndoStack();
	virtual QString getDefaultTextEncoding() const;
	virtual QString getTitle() const;
	virtual QUrl getUrl() const;
	virtual QIcon getIcon() const;
	virtual HistoryInformation getHistory() const;
	virtual int getZoom() const;
	virtual bool isLoading() const;
	virtual bool isPrivate() const;

public slots:
	virtual void triggerAction(WebAction action, bool checked = false);
	virtual void setDefaultTextEncoding(const QString &encoding);
	virtual void setHistory(const HistoryInformation &history);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url);
	virtual void setPrivate(bool enabled);
	void showMenu(const QPoint &position, MenuFlags flags);

signals:
	void statusMessageChanged(const QString &message, int timeout = 5);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
	void isPrivateChanged(bool pinned);
};

}

#endif
