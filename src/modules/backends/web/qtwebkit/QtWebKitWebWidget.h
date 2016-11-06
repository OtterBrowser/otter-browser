/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include <QtCore/QQueue>
#include <QtNetwork/QNetworkReply>
#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebInspector>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebView>
#include <QtWidgets/QSplitter>
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

	void search(const QString &query, const QString &searchEngine);
	void print(QPrinter *printer);
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false);
	Action* getAction(int identifier);
	QString getDefaultCharacterEncoding() const;
	QString getTitle() const;
	QString getActiveStyleSheet() const;
	QString getSelectedText() const;
	QVariant getPageInformation(WebWidget::PageInformation key) const;
	QStringList getBlockedElements() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail();
	QPoint getScrollPosition() const;
	QRect getProgressBarGeometry() const;
	WebWidget::SslInformation getSslInformation() const;
	WindowHistoryInformation getHistory() const;
	HitTestResult getHitTestResult(const QPoint &position);
	QStringList getStyleSheets() const;
	QList<LinkUrl> getFeeds() const;
	QList<LinkUrl> getSearchEngines() const;
	QList<NetworkManager::ResourceInformation> getBlockedRequests() const;
	QHash<QByteArray, QByteArray> getHeaders() const;
	WindowsManager::ContentStates getContentState() const;
	WindowsManager::LoadingState getLoadingState() const;
	int getZoom() const;
	bool hasSelection() const;
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
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
	void fillPassword(const PasswordsManager::PasswordInformation &password);
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void setActiveStyleSheet(const QString &styleSheet);
	void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies);
	void setOption(int identifier, const QVariant &value);
	void setScrollPosition(const QPoint &position);
	void setHistory(const WindowHistoryInformation &history);
	void setZoom(int zoom);
	void setUrl(const QUrl &url, bool isTyped = true);

protected:
	enum HistoryEntryData
	{
		IdentifierEntryData = 0,
		ZoomEntryData = 1,
		PositionEntryData = 2
	};

	explicit QtWebKitWebWidget(bool isPrivate, WebBackend *backend, QtWebKitNetworkManager *networkManager = nullptr, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void focusInEvent(QFocusEvent *event);
	void clearPluginToken();
	void resetSpellCheck(QWebElement element);
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	void muteAudio(QWebFrame *frame, bool isMuted);
#endif
	void openRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void openFormRequest(const QUrl &url, QNetworkAccessManager::Operation operation, QIODevice *outgoingData);
	void pasteText(const QString &text);
	void startDelayedTransfer(Transfer *transfer);
	void handleHistory();
#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
	void setHistory(QDataStream &stream);
#else
	void setHistory(const QVariantMap &history);
#endif
	void setOptions(const QHash<int, QVariant> &options);
	QWebPage* getPage();
	QString getPasswordToken() const;
	QString getPluginToken() const;
	QUrl resolveUrl(QWebFrame *frame, const QUrl &url) const;
	int getAmountOfNotLoadedPlugins() const;
	bool canLoadPlugins() const;
	bool canGoBack() const;
	bool canGoForward() const;
	bool canShowContextMenu(const QPoint &position) const;
	bool canViewSource() const;
	bool isInspecting() const;
	bool isNavigating() const;
	bool isScrollBar(const QPoint &position) const;

protected slots:
	void optionChanged(int identifier, const QVariant &value);
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
	void handlePrintRequest(QWebFrame *frame);
	void handleWindowCloseRequest();
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
	QSplitter *m_splitter;
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
	bool m_isTyped;
	bool m_isNavigating;

signals:
	void widgetActivated(WebWidget *widget);

friend class QtWebKitNetworkManager;
friend class QtWebKitPluginFactory;
friend class QtWebKitWebBackend;
friend class QtWebKitPage;
};

}

#endif
