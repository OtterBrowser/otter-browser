#ifndef OTTER_WEBWIDGETWEBKIT_H
#define OTTER_WEBWIDGETWEBKIT_H

#include "WebViewWebKit.h"
#include "../WebWidget.h"

#include <QtWebKitWidgets/QWebPage>

namespace Otter
{

class WebWidgetWebKit : public WebWidget
{
	Q_OBJECT

public:
	explicit WebWidgetWebKit(bool privateWindow = false, QWidget *parent = NULL);

	void print(QPrinter *printer);
	WebWidget* clone(QWidget *parent = NULL);
	QAction* getAction(WebAction action);
	QUndoStack* getUndoStack();
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	HistoryInformation getHistory() const;
	int getZoom() const;
	bool isLoading() const;
	bool isPrivate() const;
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void triggerAction(WebAction action, bool checked = false);
	void setDefaultTextEncoding(const QString &encoding);
	void setHistory(const HistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url);

protected:
	QWebPage::WebAction mapAction(WebAction action) const;

protected slots:
	void triggerAction();
	void loadStarted();
	void loadFinished(bool ok);
	void linkHovered(const QString &link, const QString &title);
	void restoreState(QWebFrame *frame);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void showMenu(const QPoint &position);

private:
	WebViewWebKit *m_webWidget;
	QHash<WebAction, QAction*> m_customActions;
	bool m_isLinkHovered;
	bool m_isLoading;

signals:
	void actionsChanged();
	void statusMessageChanged(const QString &message, int timeout = 5);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
	void isPrivateChanged(bool pinned);
};

}

#endif
