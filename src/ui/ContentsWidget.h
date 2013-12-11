#ifndef OTTER_CONTENTSWIDGET_H
#define OTTER_CONTENTSWIDGET_H

#include "Window.h"

namespace Otter
{

class ContentsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsWidget(Window *window);

	virtual void print(QPrinter *printer) = 0;
	virtual ContentsWidget* clone(Window *window = NULL) = 0;
	virtual QAction* getAction(WindowAction action) = 0;
	virtual QUndoStack* getUndoStack() = 0;
	virtual QString getTitle() const = 0;
	virtual QString getType() const = 0;
	virtual QUrl getUrl() const = 0;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() const = 0;
	virtual HistoryInformation getHistory() const = 0;
	virtual int getZoom() const = 0;
	virtual bool canZoom() const = 0;
	virtual bool isClonable() const = 0;
	virtual bool isLoading() const = 0;
	virtual bool isPrivate() const = 0;

public slots:
	virtual void triggerAction(WindowAction action, bool checked = false) = 0;
	virtual void setHistory(const HistoryInformation &history) = 0;
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url) = 0;

signals:
	void requestedOpenUrl(QUrl url, bool privateWindow = false, bool background = false, bool newWindow = false);
	void requestedAddBookmark(QUrl url);
	void requestedNewWindow(ContentsWidget *widget);
	void actionsChanged();
	void canZoomChanged(bool can);
	void statusMessageChanged(const QString &message, int timeout);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
};

}

#endif
