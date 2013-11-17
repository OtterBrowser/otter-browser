#include "WebBackend.h"
#include "WebWidget.h"

namespace Otter
{

WebBackend::WebBackend(QObject *parent) : QObject(parent)
{
}

WebWidget *WebBackend::createWidget(bool privateWindow, QWidget *parent)
{
	return new WebWidget(privateWindow, parent);
}

QString WebBackend::getTitle() const
{
	return QString();
}

QString WebBackend::getDescription() const
{
	return QString();
}

QIcon WebBackend::getIconForUrl(const QUrl &url)
{
	Q_UNUSED(url)

	return QIcon();
}

}
