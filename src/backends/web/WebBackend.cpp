#include "WebBackend.h"
#include "WebWidget.h"

namespace Otter
{

WebBackend::WebBackend(QObject *parent) : QObject(parent)
{
}

WebWidget *WebBackend::createWidget(QWidget *parent)
{
	return new WebWidget(parent);
}

QString WebBackend::getTitle() const
{
	return QString();
}

QString WebBackend::getDescription() const
{
	return QString();
}

}
