/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_QTWEBKITWEBWIDGET_H
#define OTTER_QTWEBKITWEBWIDGET_H

#include "../../../../ui/WebWidget.h"

#include <QtCore/QQueue>
#include <QtNetwork/QNetworkReply>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>

namespace Otter
{

class ContentsDialog;
class QtWebKitInspector;
class QtWebKitNetworkManager;
class QtWebKitWebBackend;
class QtWebKitPage;
class SourceViewerWebWidget;

class QtWebKitWebWidget final : public WebWidget
{
	Q_OBJECT

public:
	~QtWebKitWebWidget();

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
	QStringList getBlockedElements() const;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPixmap createThumbnail(const QSize &size = {}) override;
	QPoint getScrollPosition() const override;
	QRect getGeometry(bool excludeScrollBars = false) const override;
	LinkUrl getActiveFrame() const override;
	LinkUrl getActiveImage() const override;
	LinkUrl getActiveLink() const override;
	LinkUrl getActiveMedia() const override;
	SslInformation getSslInformation() const override;
	WindowHistoryInformation getHistory() const override;
	HitTestResult getHitTestResult(const QPoint &position) override;
	QStringList getStyleSheets() const override;
	QVector<LinkUrl> getFeeds() const override;
	QVector<LinkUrl> getLinks() const override;
	QVector<LinkUrl> getSearchEngines() const override;
	QVector<NetworkManager::ResourceInformation> getBlockedRequests() const override;
	QMap<QByteArray, QByteArray> getHeaders() const override;
	QMultiMap<QString, QString> getMetaData() const override;
	ContentStates getContentState() const override;
	LoadingState getLoadingState() const override;
	int getZoom() const override;
	int findInPage(const QString &text, FindFlags flags = NoFlagsFind) override;
	bool hasSelection() const override;
	bool hasWatchedChanges(ChangeWatcher watcher) const override;
	bool isAudible() const override;
	bool isAudioMuted() const override;
	bool isFullScreen() const override;
	bool isPrivate() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void clearOptions() override;
	void fillPassword(const PasswordsManager::PasswordInformation &password) override;
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void setActiveStyleSheet(const QString &styleSheet) override;
	void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies) override;
	void setOption(int identifier, const QVariant &value) override;
	void setScrollPosition(const QPoint &position) override;
	void setHistory(const WindowHistoryInformation &history) override;
	void setZoom(int zoom) override;
	void setUrl(const QUrl &url, bool isTyped = true) override;

protected:
	enum HistoryEntryData
	{
		IdentifierEntryData = 0,
		ZoomEntryData,
		PositionEntryData,
		VisitTimeEntryData
	};

	explicit QtWebKitWebWidget(const QVariantMap &parameters, WebBackend *backend, QtWebKitNetworkManager *networkManager = nullptr, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void clearPluginToken();
	void resetSpellCheck(QWebElement element);
	void muteAudio(QWebFrame *frame, bool isMuted);
	void openRequest(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void openFormRequest(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void startDelayedTransfer(Transfer *transfer);
	void handleHistory();
	void handleNavigationRequest(const QUrl &url, QWebPage::NavigationType type);
	void setHistory(const QVariantMap &history);
	void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = {}) override;
	QtWebKitPage* getPage() const;
	QString getMessageToken() const;
	QString getPluginToken() const;
	QUrl resolveUrl(QWebFrame *frame, const QUrl &url) const;
	QVector<LinkUrl> getLinks(const QString &query) const;
	int getAmountOfDeferredPlugins() const override;
	bool canLoadPlugins() const;
	bool canGoBack() const override;
	bool canGoForward() const override;
	bool canFastForward() const override;
	bool canInspect() const override;
	bool canTakeScreenshot() const override;
	bool canRedo() const override;
	bool canUndo() const override;
	bool canShowContextMenu(const QPoint &position) const override;
	bool canViewSource() const override;
	bool isInspecting() const override;
	bool isNavigating() const;
	bool isPopup() const override;
	bool isScrollBar(const QPoint &position) const override;

protected slots:
	void saveState(QWebFrame *frame, QWebHistoryItem *item);
	void restoreState(QWebFrame *frame);
	void handleDownloadRequested(const QNetworkRequest &request);
	void handleUnsupportedContent(QNetworkReply *reply);
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleLoadStarted();
	void handleLoadProgress(int progress);
	void handleLoadFinished(bool result);
	void handleViewSourceReplyFinished();
	void handlePrintRequest(QWebFrame *frame);
	void handleFullScreenRequest(QWebFullScreenRequest request);
	void handlePermissionRequest(QWebFrame *frame, QWebPage::Feature feature);
	void handlePermissionCancel(QWebFrame *frame, QWebPage::Feature feature);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void notifyPermissionRequested(QWebFrame *frame, QWebPage::Feature nativeFeature, bool cancel);
	void notifySavePasswordRequested(const PasswordsManager::PasswordInformation &password, bool isUpdate);
	void notifyContentStateChanged();
	void updateAmountOfDeferredPlugins();
	void updateOptions(const QUrl &url);

private:
	QWebView *m_webView;
	QtWebKitPage *m_page;
	QtWebKitInspector *m_inspector;
	QtWebKitNetworkManager *m_networkManager;
	QString m_messageToken;
	QString m_pluginToken;
	QPixmap m_thumbnail;
	QNetworkRequest m_formRequest;
	QByteArray m_formRequestBody;
	QQueue<Transfer*> m_transfers;
	QHash<QNetworkReply*, QPointer<SourceViewerWebWidget> > m_viewSourceReplies;
	QNetworkAccessManager::Operation m_formRequestOperation;
	LoadingState m_loadingState;
	int m_amountOfDeferredPlugins;
	int m_transfersTimer;
	bool m_canLoadPlugins;
	bool m_isAudioMuted;
	bool m_isFullScreen;
	bool m_isTyped;
	bool m_isNavigating;

signals:
	void widgetActivated(WebWidget *widget);

friend class QtWebKitFrame;
friend class QtWebKitNetworkManager;
friend class QtWebKitPluginFactory;
friend class QtWebKitWebBackend;
friend class QtWebKitPage;
};

}

#endif
