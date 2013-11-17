#ifndef OTTER_WEBVIEWWEBKIT_H
#define OTTER_WEBVIEWWEBKIT_H

#include <QtWebKitWidgets/QWebView>

namespace Otter
{

class WebViewWebKit : public QWebView
{
	Q_OBJECT

public:
	explicit WebViewWebKit(QWidget *parent = NULL);

protected:
	void wheelEvent(QWheelEvent *event);

signals:
	void requestedZoomChange(int zoom);
};

}

#endif
