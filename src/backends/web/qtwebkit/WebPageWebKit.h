#ifndef OTTER_WEBPAGEWEBKIT_H
#define OTTER_WEBPAGEWEBKIT_H

#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class WebWidget;

class WebPageWebKit : public QWebPage
{
	Q_OBJECT

public:
	explicit WebPageWebKit(QObject *parent = 0);

	bool extension(Extension extension, const ExtensionOption *option = NULL, ExtensionReturn *output = NULL);
	bool supportsExtension(Extension extension) const;

protected:
	QWebPage* createWindow(WebWindowType type);
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

signals:
	void requestedNewWindow(WebWidget *widget);
};

}

#endif
