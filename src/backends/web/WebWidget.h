#ifndef OTTER_WEBWIDGET_H
#define OTTER_WEBWIDGET_H

#include "WebBackend.h"

#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QWidget>

namespace Otter
{

class WebWidget : public QWidget
{
	Q_OBJECT

public:
	explicit WebWidget(QWidget *parent = NULL);

	virtual void print(QPrinter *printer);
	virtual WebWidget* clone(QWidget *parent = NULL);
	virtual QAction* getAction(WebAction action);
	virtual QUndoStack* getUndoStack();
	virtual QString getTitle() const;
	virtual QUrl getUrl() const;
	virtual QIcon getIcon() const;
	virtual int getZoom() const;
	virtual bool isLoading() const;
	virtual bool isPrivate() const;


public slots:
	virtual void triggerAction(WebAction action, bool checked = false);
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url);
	virtual void setPrivate(bool enabled);

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
