/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
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

class QtWebEnginePage;
class QtWebEngineUrlRequestInterceptor;
class QtWebEngineWebWidget;
class SourceViewerWebWidget;

class QtWebEngineInspectorWidget final : public QWebEngineView
{
public:
	explicit QtWebEngineInspectorWidget(QWebEnginePage *inspectedPage, QtWebEngineWebWidget *parent);

protected:
	void showEvent(QShowEvent *event) override;

private:
	QWebEnginePage *m_page;
	QWebEnginePage *m_inspectedPage;
};

class QtWebEngineWebWidget final : public WebWidget
{
	Q_OBJECT

public:
	~QtWebEngineWebWidget();

	void search(const QString &query, const QString &searchEngine) override;
	void print(QPrinter *printer) override;
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = {}) const override;
	QWidget* getInspector() override;
	QWidget* getViewport() override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getActiveStyleSheet() const override;
	QString getSelectedText() const override;
	QVariant getPageInformation(PageInformation key) const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPixmap createThumbnail(const QSize &size = {}) override;
	QPoint getScrollPosition() const override;
	LinkUrl getActiveFrame() const override;
	LinkUrl getActiveImage() const override;
	LinkUrl getActiveLink() const override;
	LinkUrl getActiveMedia() const override;
	SslInformation getSslInformation() const override;
	Session::Window::History getHistory() const override;
	HitTestResult getHitTestResult(const QPoint &position) override;
	QStringList getStyleSheets() const override;
	QVector<LinkUrl> getFeeds() const override;
	QVector<LinkUrl> getLinks() const override;
	QVector<LinkUrl> getSearchEngines() const override;
	QVector<NetworkManager::ResourceInformation> getBlockedRequests() const override;
	QMultiMap<QString, QString> getMetaData() const override;
	LoadingState getLoadingState() const override;
	int getZoom() const override;
	bool hasSelection() const override;
	bool hasWatchedChanges(ChangeWatcher watcher) const override;
	bool isAudible() const override;
	bool isAudioMuted() const override;
	bool isFullScreen() const override;
	bool isPrivate() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void clearOptions() override;
	void findInPage(const QString &text, FindFlags flags = NoFlagsFind) override;
	void setActiveStyleSheet(const QString &styleSheet) override;
	void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies) override;
	void setOption(int identifier, const QVariant &value) override;
	void setScrollPosition(const QPoint &position) override;
	void setHistory(const Session::Window::History &history) override;
	void setZoom(int zoom) override;
	void setUrl(const QUrl &url, bool isTypedIn = true) override;

protected:
	explicit QtWebEngineWebWidget(const QVariantMap &parameters, WebBackend *backend, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void ensureInitialized();
	void viewSource(const QUrl &url);
	void notifyWatchedDataChanged(ChangeWatcher watcher);
	void updateOptions(const QUrl &url);
	void updateWatchedData(ChangeWatcher watcher) override;
	void setHistory(QDataStream &stream);
	void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = {}) override;
	QWebEnginePage* getPage() const;
	QString parsePosition(const QString &script, const QPoint &position) const;
	QDateTime getLastUrlClickTime() const;
	QStringList getBlockedElements() const;
	QVector<LinkUrl> processLinks(const QVariantList &rawLinks) const;
	bool canGoBack() const override;
	bool canGoForward() const override;
	bool canFastForward() const override;
	bool canInspect() const override;
	bool canRedo() const override;
	bool canUndo() const override;
	bool canShowContextMenu(const QPoint &position) const override;
	bool canViewSource() const override;
	bool isInspecting() const override;
	bool isPopup() const override;
	bool isScrollBar(const QPoint &position) const override;
	bool isTypedIn() const;

protected slots:
	void handleLoadStarted();
	void handleLoadFinished();
	void handleViewSourceReplyFinished();
	void handlePrintRequest();
	void handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy);
	void handleFullScreenRequest(QWebEngineFullScreenRequest request);
	void notifyTitleChanged();
	void notifyUrlChanged();
	void notifyIconChanged();
	void notifyPermissionRequested(const QUrl &url, QWebEnginePage::Feature nativeFeature, bool cancel);
	void notifyRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status);
	void notifyNavigationActionsChanged();

private:
	QWebEngineView *m_webView;
	QtWebEngineInspectorWidget *m_inspectorWidget;
	QtWebEnginePage *m_page;
	QtWebEngineUrlRequestInterceptor *m_requestInterceptor;
	QString m_findInPageText;
	QDateTime m_lastUrlClickTime;
	QPixmap m_thumbnail;
	HitTestResult m_hitResult;
	QHash<QNetworkReply*, QPointer<SourceViewerWebWidget> > m_viewSourceReplies;
	QMultiMap<QString, QString> m_metaData;
	QStringList m_styleSheets;
	QVector<LinkUrl> m_feeds;
	QVector<LinkUrl> m_links;
	QVector<LinkUrl> m_searchEngines;
	QVector<bool> m_watchedChanges;
	LoadingState m_loadingState;
	TrileanValue m_canGoForwardValue;
	int m_documentLoadingProgress;
	int m_focusProxyTimer;
	int m_updateNavigationActionsTimer;
	bool m_isClosing;
	bool m_isEditing;
	bool m_isFullScreen;
	bool m_isTypedIn;

friend class QtWebEnginePage;
friend class QtWebEngineWebBackend;
};

}

#endif
