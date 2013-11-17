#include "WebViewWebKit.h"

#include <QtGui/QWheelEvent>

namespace Otter
{

WebViewWebKit::WebViewWebKit(QWidget *parent) : QWebView(parent)
{
}

void WebViewWebKit::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() & Qt::CTRL || event->buttons() & Qt::LeftButton)
	{
		emit requestedZoomChange((zoomFactor() * 100) + (event->delta() / 16));

		event->accept();
	}
	else
	{
		QWebView::wheelEvent(event);
	}
}

}
