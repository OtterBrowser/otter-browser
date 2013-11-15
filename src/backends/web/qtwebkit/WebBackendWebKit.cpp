#include "WebBackendWebKit.h"
#include "WebWidgetWebKit.h"

namespace Otter
{

WebBackendWebKit::WebBackendWebKit(QObject *parent) : WebBackend(parent)
{
}

WebWidget *WebBackendWebKit::createWidget(QWidget *parent)
{
	return new WebWidgetWebKit(parent);
}

QString WebBackendWebKit::getTitle() const
{
	return tr("WebKit Backend");
}

QString WebBackendWebKit::getDescription() const
{
	return tr("Backend utilizing QtWebKit module");
}

}
