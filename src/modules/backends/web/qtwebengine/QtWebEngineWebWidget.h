/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBENGINEWEBWIDGET_H
#define OTTER_QTWEBENGINEWEBWIDGET_H

#include "../../../../ui/WebWidget.h"

#include <QtNetwork/QNetworkReply>
#include <QtWebEngineWidgets/QWebEngineFullScreenRequest>
#include <QtWebEngineWidgets/QWebEngineView>

namespace Otter
{

class ContentsDialog;
class QtWebEnginePage;
class SourceViewerWebWidget;

class QtWebEngineWebWidget : public WebWidget
{
	Q_OBJECT

public:
	void search(const QString &query, const QString &searchEngine);
	void print(QPrinter *printer);
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false);
	Action* getAction(int identifier);
	QString getTitle() const;
	QString getSelectedText() const;
	QVariant getPageInformation(PageInformation key) const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail();
	QPoint getScrollPosition() const;
	QRect getProgressBarGeometry() const;
	WindowHistoryInformation getHistory() const;
	HitTestResult getHitTestResult(const QPoint &position);
	QHash<QByteArray, QByteArray> getHeaders() const;
	WindowsManager::LoadingState getLoadingState() const;
	int getZoom() const;
	bool hasSelection() const;
#if QT_VERSION >= 0x050700
	bool isAudible() const;
	bool isAudioMuted() const;
#endif
	bool isPrivate() const;
	bool findInPage(const QString &text, FindFlags flags = NoFlagsFind);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void clearOptions();
	void goToHistoryIndex(int index);
	void removeHistoryIndex(int index, bool purge = false);
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies);
	void setOption(int identifier, const QVariant &value);
	void setScrollPosition(const QPoint &position);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool isTyped = true);

protected:
	explicit QtWebEngineWebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event);
	void focusInEvent(QFocusEvent *event);
	void pasteText(const QString &text);
	void updateOptions(const QUrl &url);
	void setHistory(QDataStream &stream);
	void setOptions(const QHash<int, QVariant> &options);
	QWebEnginePage* getPage();
	QList<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	QDateTime getLastUrlClickTime() const;
	bool canGoBack() const;
	bool canGoForward() const;
	bool canShowContextMenu(const QPoint &position) const;
	bool canViewSource() const;
	bool isScrollBar(const QPoint &position) const;

protected slots:
	void pageLoadStarted();
	void pageLoadFinished();
	void linkHovered(const QString &link);
	void iconReplyFinished();
	void viewSourceReplyFinished(QNetworkReply::NetworkError error = QNetworkReply::NoError);
	void handleIconChange(const QUrl &url);
	void handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy);
	void handleFullScreenRequest(QWebEngineFullScreenRequest request);
	void handlePermissionRequest(const QUrl &url, QWebEnginePage::Feature feature);
	void handlePermissionCancel(const QUrl &url, QWebEnginePage::Feature feature);
	void handleWindowCloseRequest();
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void notifyPermissionRequested(const QUrl &url, QWebEnginePage::Feature nativeFeature, bool cancel);
	void notifyRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status);
	void notifyDocumentLoadingProgress(int progress);
	void updateUndo();
	void updateRedo();

private:
	QWebEngineView *m_webView;
	QtWebEnginePage *m_page;
	QNetworkReply *m_iconReply;
	QTime *m_loadingTime;
	QIcon m_icon;
	QDateTime m_lastUrlClickTime;
	HitTestResult m_hitResult;
	QPoint m_scrollPosition;
	QHash<QNetworkReply*, QPointer<SourceViewerWebWidget> > m_viewSourceReplies;
	WindowsManager::LoadingState m_loadingState;
	int m_documentLoadingProgress;
	int m_scrollTimer;
	bool m_isEditing;
	bool m_isTyped;

friend class QtWebEnginePage;
friend class QtWebEngineWebBackend;
};

}

#endif
