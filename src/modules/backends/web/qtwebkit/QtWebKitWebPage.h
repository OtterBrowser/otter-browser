#ifndef OTTER_QTWEBKITWEBPAGE_H
#define OTTER_QTWEBKITWEBPAGE_H

#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class QtWebKitWebWidget;
class WebWidget;

class QtWebKitWebPage : public QWebPage
{
	Q_OBJECT

public:
	explicit QtWebKitWebPage(QtWebKitWebWidget *parent);

	void triggerAction(WebAction action, bool checked = false);
	void setParent(QtWebKitWebWidget *parent);
	bool extension(Extension extension, const ExtensionOption *option = NULL, ExtensionReturn *output = NULL);
	bool shouldInterruptJavaScript();
	bool supportsExtension(Extension extension) const;

protected:
	void javaScriptAlert(QWebFrame *frame, const QString &message);
	QWebPage* createWindow(WebWindowType type);
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
	bool javaScriptConfirm(QWebFrame *frame, const QString & message);

protected slots:
	void clearIgnoreJavaScriptPopups();

private:
	QtWebKitWebWidget *m_webWidget;
	bool m_ignoreJavaScriptPopups;

signals:
	void requestedNewWindow(WebWidget *widget);
};

}

#endif
