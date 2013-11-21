#ifndef OTTER_WEBVIEWWEBKIT_H
#define OTTER_WEBVIEWWEBKIT_H

#include "../../../ui/Window.h"

#include <QtWebKitWidgets/QWebView>

namespace Otter
{

class WebViewWebKit : public QWebView
{
	Q_OBJECT

public:
	explicit WebViewWebKit(QWidget *parent = NULL);

protected:
	void mousePressEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

signals:
	void requestedZoomChange(int zoom);
	void requestedTriggerAction(WindowAction action);
};

}

#endif
