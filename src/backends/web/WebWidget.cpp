#include "WebWidget.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

WebWidget::WebWidget(QWidget *parent) : QWidget(parent)
{
}

void WebWidget::print(QPrinter *printer)
{
	Q_UNUSED(printer)
}

void WebWidget::triggerAction(WebAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void WebWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void WebWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

void WebWidget::setPrivate(bool enabled)
{
	Q_UNUSED(enabled)
}

WebWidget* WebWidget::clone(QWidget *parent)
{
	Q_UNUSED(parent)

	return NULL;
}

QAction* WebWidget::getAction(WebAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* WebWidget::getUndoStack()
{
	return NULL;
}

QString WebWidget::getTitle() const
{
	return QString();
}

QUrl WebWidget::getUrl() const
{
	return QUrl();
}

QIcon WebWidget::getIcon() const
{
	return QIcon();
}

int WebWidget::getZoom() const
{
	return 100;
}

bool WebWidget::isLoading() const
{
	return false;
}

bool WebWidget::isPrivate() const
{
	return false;
}


}
