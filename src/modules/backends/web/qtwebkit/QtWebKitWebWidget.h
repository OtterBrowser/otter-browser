/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_QTWEBKITWEBWIDGET_H
#define OTTER_QTWEBKITWEBWIDGET_H

#include "../../../../ui/WebWidget.h"

#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebInspector>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolButton>

namespace Otter
{

class ContentsDialog;
class NetworkManager;
class QtWebKitWebBackend;
class QtWebKitWebPage;

class QtWebKitWebWidget : public WebWidget
{
	Q_OBJECT

public:
	void search(const QString &query, const QString &engine);
	void print(QPrinter *printer);
	WebWidget* clone(bool cloneHistory = true);
	QAction* getAction(ActionIdentifier action);
	QUndoStack* getUndoStack();
	QString getDefaultCharacterEncoding() const;
	QString getTitle() const;
	QString getSelectedText() const;
	QVariant evaluateJavaScript(const QString &script);
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail();
	QRect getProgressBarGeometry() const;
	WindowHistoryInformation getHistory() const;
	QPair<QString, QString> getUserAgent() const;
	int getRefreshTime() const;
	int getZoom() const;
	bool isLoading() const;
	bool isPrivate() const;
	bool find(const QString &text, FindFlags flags = HighlightAllFind);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void showDialog(ContentsDialog *dialog);
	void hideDialog(ContentsDialog *dialog);
	void goToHistoryIndex(int index);
	void triggerAction(ActionIdentifier action, bool checked = false);
	void setDefaultCharacterEncoding(const QString &encoding);
	void setUserAgent(const QString &identifier, const QString &value);
	void setHistory(const WindowHistoryInformation &history);
	void setRefreshTime(int time);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool typed = true);

protected:
	explicit QtWebKitWebWidget(bool isPrivate = false, WebBackend *backend = NULL, ContentsWidget *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void focusInEvent(QFocusEvent *event);
	void markPageRealoded();
	void openUrl(QUrl url, OpenHints hints = DefaultOpen);
	void setNetworkManager(NetworkManager *manager);
	QWebPage* getPage();
	QWebPage::WebAction mapAction(ActionIdentifier action) const;

protected slots:
	void triggerAction();
	void optionChanged(const QString &option, const QVariant &value);
	void pageLoadStarted();
	void pageLoadFinished(bool ok);
	void downloadFile(const QNetworkRequest &request);
	void downloadFile(QNetworkReply *reply);
	void saveState(QWebFrame *frame, QWebHistoryItem *item);
	void restoreState(QWebFrame *frame);
	void hideInspector();
	void linkHovered(const QString &link);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void updateQuickSearchAction();
	void showContextMenu(const QPoint &position = QPoint());

private:
	QWebView *m_webView;
	QtWebKitWebPage *m_page;
	QWebInspector *m_inspector;
	QToolButton *m_inspectorCloseButton;
	NetworkManager *m_networkManager;
	QSplitter *m_splitter;
	QPixmap m_thumbnail;
	QPoint m_hotclickPosition;
	QWebHitTestResult m_hitResult;
	QHash<ActionIdentifier, QAction*> m_actions;
	qint64 m_historyEntry;
	int m_refreshTime;
	int m_refreshTimer;
	bool m_ignoreContextMenu;
	bool m_isUsingRockerNavigation;
	bool m_isLoading;
	bool m_isReloading;
	bool m_isTyped;

friend class QtWebKitWebBackend;
friend class QtWebKitWebPage;
};

}

#endif
