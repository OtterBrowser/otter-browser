/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebInspector>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>
#include <QtWidgets/QToolButton>

namespace Otter
{

class ContentsDialog;
class QtWebKitInspector;
class QtWebKitNetworkManager;
class QtWebKitWebBackend;
class QtWebKitPage;
class QtWebKitPluginFactory;
class SourceViewerWebWidget;

class QtWebKitWebWidget : public WebWidget
{
	Q_OBJECT

public:
	~QtWebKitWebWidget();

	void search(const QString &query, const QString &searchEngine) override;
	void print(QPrinter *printer) override;
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = QStringList()) override;
	QWidget* getInspector() override;
	QWidget* getViewport() override;
	Action* getAction(int identifier) override;
	QString getTitle() const override;
	QString getActiveStyleSheet() const override;
	QString getSelectedText() const override;
	QVariant getPageInformation(WebWidget::PageInformation key) const override;
	QStringList getBlockedElements() const;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPixmap getThumbnail() override;
	QPoint getScrollPosition() const override;
	QRect getProgressBarGeometry() const override;
	WebWidget::SslInformation getSslInformation() const override;
	WindowHistoryInformation getHistory() const override;
	HitTestResult getHitTestResult(const QPoint &position) override;
	QStringList getStyleSheets() const override;
	QList<LinkUrl> getFeeds() const override;
	QList<LinkUrl> getSearchEngines() const override;
	QList<NetworkManager::ResourceInformation> getBlockedRequests() const override;
	QHash<QByteArray, QByteArray> getHeaders() const override;
	WindowsManager::ContentStates getContentState() const override;
	WindowsManager::LoadingState getLoadingState() const override;
	int getZoom() const override;
	bool hasSelection() const override;
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	bool isAudible() const override;
	bool isAudioMuted() const override;
#endif
	bool isFullScreen() const override;
	bool isPrivate() const override;
	bool findInPage(const QString &text, FindFlags flags = NoFlagsFind) override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void clearOptions() override;
	void goToHistoryIndex(int index) override;
	void removeHistoryIndex(int index, bool purge = false) override;
	void fillPassword(const PasswordsManager::PasswordInformation &password) override;
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap()) override;
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
		ZoomEntryData = 1,
		PositionEntryData = 2
	};

	explicit QtWebKitWebWidget(bool isPrivate, WebBackend *backend, QtWebKitNetworkManager *networkManager = nullptr, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void clearPluginToken();
	void resetSpellCheck(QWebElement element);
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	void muteAudio(QWebFrame *frame, bool isMuted);
#endif
	void openRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void openFormRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void pasteText(const QString &text) override;
	void startDelayedTransfer(Transfer *transfer);
	void handleHistory();
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
	void setHistory(QDataStream &stream);
#else
	void setHistory(const QVariantMap &history);
#endif
	void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = QStringList()) override;
	QWebPage* getPage();
	QString getPasswordToken() const;
	QString getPluginToken() const;
	QUrl resolveUrl(QWebFrame *frame, const QUrl &url) const;
	int getAmountOfNotLoadedPlugins() const override;
	bool canLoadPlugins() const;
	bool canGoBack() const override;
	bool canGoForward() const override;
	bool canFastForward() const override;
	bool canShowContextMenu(const QPoint &position) const override;
	bool canViewSource() const override;
	bool isInspecting() const override;
	bool isNavigating() const;
	bool isScrollBar(const QPoint &position) const override;

protected slots:
	void navigating(const QUrl &url, QWebFrame *frame, QWebPage::NavigationType type);
	void pageLoadStarted();
	void pageLoadFinished();
	void downloadFile(const QNetworkRequest &request);
	void downloadFile(QNetworkReply *reply);
	void saveState(QWebFrame *frame, QWebHistoryItem *item);
	void restoreState(QWebFrame *frame);
	void linkHovered(const QString &link);
	void openFormRequest();
	void viewSourceReplyFinished(QNetworkReply::NetworkError error = QNetworkReply::NoError);
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleLoadProgress(int progress);
	void handlePrintRequest(QWebFrame *frame);
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	void handleFullScreenRequest(QWebFullScreenRequest request);
#endif
	void handlePermissionRequest(QWebFrame *frame, QWebPage::Feature feature);
	void handlePermissionCancel(QWebFrame *frame, QWebPage::Feature feature);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void notifyPermissionRequested(QWebFrame *frame, QWebPage::Feature nativeFeature, bool cancel);
	void notifySavePasswordRequested(const PasswordsManager::PasswordInformation &password, bool isUpdate);
	void notifyContentStateChanged();
	void updateUndoText(const QString &text);
	void updateRedoText(const QString &text);
	void updateOptions(const QUrl &url);

private:
	QWebView *m_webView;
	QtWebKitPage *m_page;
	QtWebKitPluginFactory *m_pluginFactory;
	QtWebKitInspector *m_inspector;
	QtWebKitNetworkManager *m_networkManager;
	QString m_passwordToken;
	QString m_pluginToken;
	QPixmap m_thumbnail;
	QUrl m_formRequestUrl;
	QByteArray m_formRequestBody;
	QQueue<Transfer*> m_transfers;
	QHash<QNetworkReply*, QPointer<SourceViewerWebWidget> > m_viewSourceReplies;
	QNetworkAccessManager::Operation m_formRequestOperation;
	WindowsManager::LoadingState m_loadingState;
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
