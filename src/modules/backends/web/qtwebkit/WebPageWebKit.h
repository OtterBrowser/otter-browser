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
	explicit WebPageWebKit(WebWidget *parent);

	void triggerAction(WebAction action, bool checked = false);
	void setParent(WebWidget *parent);
	bool extension(Extension extension, const ExtensionOption *option = NULL, ExtensionReturn *output = NULL);
	bool supportsExtension(Extension extension) const;

protected:
	QWebPage* createWindow(WebWindowType type);
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

private:
	WebWidget *m_webWidget;

signals:
	void requestedNewWindow(WebWidget *widget);
};

}

#endif
