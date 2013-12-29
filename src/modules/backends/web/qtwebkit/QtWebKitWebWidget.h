#ifndef OTTER_QTWEBKITWEBWIDGET_H
#define OTTER_QTWEBKITWEBWIDGET_H

#include "../../../../ui/WebWidget.h"

#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebInspector>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>
#include <QtWidgets/QSplitter>

namespace Otter
{

class NetworkAccessManager;
class QtWebKitWebPage;

class QtWebKitWebWidget : public WebWidget
{
	Q_OBJECT

public:
	explicit QtWebKitWebWidget(bool privateWindow = false, ContentsWidget *parent = NULL, QtWebKitWebPage *page = NULL);

	void search(const QString &query, const QString &engine);
	void print(QPrinter *printer);
	void setQuickSearchEngine(const QString &searchEngine);
	WebWidget* clone(ContentsWidget *parent = NULL);
	QAction* getAction(WindowAction action);
	QUndoStack* getUndoStack();
	QString getDefaultTextEncoding() const;
	QString getTitle() const;
	QString getSearchEngine() const;
	QVariant evaluateJavaScript(const QString &script);
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail();
	QRect getProgressBarGeometry() const;
	WindowHistoryInformation getHistory() const;
	int getZoom() const;
	bool isLoading() const;
	bool isPrivate() const;
	bool find(const QString &text, FindFlags flags = HighlightAllFind);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void showDialog(QWidget *dialog);
	void hideDialog(QWidget *dialog);
	void triggerAction(WindowAction action, bool checked = false);
	void setDefaultTextEncoding(const QString &encoding);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	QWebPage::WebAction mapAction(WindowAction action) const;

protected slots:
	void search(QAction *action);
	void triggerAction();
	void optionChanged(const QString &option, const QVariant &value);
	void pageLoadStarted();
	void pageLoadFinished(bool ok);
	void downloadFile(const QNetworkRequest &request);
	void downloadFile(QNetworkReply *reply);
	void linkHovered(const QString &link, const QString &title);
	void saveState(QWebFrame *frame, QWebHistoryItem *item);
	void restoreState(QWebFrame *frame);
	void searchMenuAboutToShow();
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void updateSearchActions(const QString &engine = QString());
	void showContextMenu(const QPoint &position);

private:
	ContentsWidget *m_parent;
	QWebView *m_webView;
	QWebInspector *m_inspector;
	NetworkAccessManager *m_networkAccessManager;
	QSplitter *m_splitter;
	QString m_searchEngine;
	QPixmap m_thumbnail;
	QWebHitTestResult m_hitResult;
	QHash<WindowAction, QAction*> m_actions;
	qint64 m_historyEntry;
	bool m_isLinkHovered;
	bool m_isLoading;
	bool m_isTyped;
};

}

#endif
