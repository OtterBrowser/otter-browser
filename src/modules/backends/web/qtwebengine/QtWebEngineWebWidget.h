/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtWebEngineCore/QtWebEngineCoreVersion>
#include <QtWebEngineWidgets/QWebEngineFullScreenRequest>
#include <QtWebEngineWidgets/QWebEngineView>

namespace Otter
{

class QtWebEnginePage;
class SourceViewerWebWidget;

class QtWebEngineWebWidget final : public WebWidget
{
	Q_OBJECT

public:
	struct QtWebEngineHitTestResult final : public HitTestResult
	{
		explicit QtWebEngineHitTestResult(const QVariant &result)
		{
			const QVariantMap map(result.toMap());
			const QVariantMap geometryMap(map.value(QLatin1String("geometry")).toMap());

			title = map.value(QLatin1String("title")).toString();
			tagName = map.value(QLatin1String("tagName")).toString();
			alternateText = map.value(QLatin1String("alternateText")).toString();
			longDescription = map.value(QLatin1String("longDescription")).toString();
			formUrl = QUrl(map.value(QLatin1String("formUrl")).toString());
			frameUrl = QUrl(map.value(QLatin1String("frameUrl")).toString());
			imageUrl = QUrl(map.value(QLatin1String("imageUrl")).toString());
			linkUrl = QUrl(map.value(QLatin1String("linkUrl")).toString());
			mediaUrl = QUrl(map.value(QLatin1String("mediaUrl")).toString());
			elementGeometry = QRect(geometryMap.value(QLatin1String("x")).toInt(), geometryMap.value(QLatin1String("y")).toInt(), geometryMap.value(QLatin1String("w")).toInt(), geometryMap.value(QLatin1String("h")).toInt());
			hitPosition = map.value(QLatin1String("position")).toPoint();
			playbackRate = map.value(QLatin1String("playbackRate")).toReal();
			flags = static_cast<HitTestFlags>(map.value(QLatin1String("flags")).toInt());
		}
	};

	void search(const QString &query, const QString &searchEngine) override;
	void print(QPrinter *printer) override;
	WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = {}) const override;
#if QTWEBENGINECORE_VERSION >= 0x050B00
	QWidget* getInspector();
#endif
	QWidget* getViewport() override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getActiveStyleSheet() const override;
	QString getSelectedText() const override;
	QVariant getPageInformation(PageInformation key) const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	QPoint getScrollPosition() const override;
	LinkUrl getActiveFrame() const override;
	LinkUrl getActiveImage() const override;
	LinkUrl getActiveLink() const override;
	LinkUrl getActiveMedia() const override;
	WindowHistoryInformation getHistory() const override;
	HitTestResult getHitTestResult(const QPoint &position) override;
	QStringList getStyleSheets() const override;
	QVector<LinkUrl> getFeeds() const override;
	QVector<LinkUrl> getLinks() const override;
	QVector<LinkUrl> getSearchEngines() const override;
	QMultiMap<QString, QString> getMetaData() const override;
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
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void setActiveStyleSheet(const QString &styleSheet) override;
	void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies) override;
	void setOption(int identifier, const QVariant &value) override;
	void setScrollPosition(const QPoint &position) override;
	void setHistory(const WindowHistoryInformation &history) override;
	void setZoom(int zoom) override;
	void setUrl(const QUrl &url, bool isTyped = true) override;

protected:
	explicit QtWebEngineWebWidget(const QVariantMap &parameters, WebBackend *backend, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void ensureInitialized();
	void notifyWatchedDataChanged(ChangeWatcher watcher);
	void updateOptions(const QUrl &url);
	void updateWatchedData(ChangeWatcher watcher) override;
	void setHistory(QDataStream &stream);
	void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = {}) override;
	QWebEnginePage* getPage() const;
	QString parsePosition(const QString &script, const QPoint &position) const;
	QDateTime getLastUrlClickTime() const;
	QVector<LinkUrl> processLinks(const QVariantList &rawLinks) const;
	bool canGoBack() const override;
	bool canGoForward() const override;
	bool canFastForward() const override;
#if QTWEBENGINECORE_VERSION >= 0x050B00
	bool canInspect() const;
#endif
	bool canRedo() const override;
	bool canUndo() const override;
	bool canShowContextMenu(const QPoint &position) const override;
	bool canViewSource() const override;
#if QTWEBENGINECORE_VERSION >= 0x050B00
	bool isInspecting() const;
#endif
	bool isPopup() const override;
	bool isScrollBar(const QPoint &position) const override;

protected slots:
	void handleLoadStarted();
	void handleLoadFinished();
	void handleViewSourceReplyFinished();
	void handleAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator);
	void handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator, const QString &proxy);
	void handleFullScreenRequest(QWebEngineFullScreenRequest request);
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();
	void notifyPermissionRequested(const QUrl &url, QWebEnginePage::Feature nativeFeature, bool cancel);
	void notifyRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus status);
	void notifyNavigationActionsChanged();

private:
	QWebEngineView *m_webView;
#if QTWEBENGINECORE_VERSION >= 0x050B00
	QWebEngineView *m_inspectorView;
#endif
	QtWebEnginePage *m_page;
	QDateTime m_lastUrlClickTime;
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
	bool m_isEditing;
	bool m_isFullScreen;
	bool m_isTyped;

friend class QtWebEnginePage;
friend class QtWebEngineWebBackend;
};

}

#endif
