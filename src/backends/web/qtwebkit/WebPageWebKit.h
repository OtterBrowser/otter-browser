#ifndef OTTER_WEBPAGEWEBKIT_H
#define OTTER_WEBPAGEWEBKIT_H

#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class WebPageWebKit : public QWebPage
{
	Q_OBJECT

public:
	explicit WebPageWebKit(QObject *parent = 0);

	void triggerAction(WebAction action, bool checked = false);
	bool extension(Extension extension, const ExtensionOption *option = NULL, ExtensionReturn *output = NULL);
	bool supportsExtension(Extension extension) const;
};

}

#endif
