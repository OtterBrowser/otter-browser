/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2017 Piktas Zuikis <piktas.zuikis@inbox.lt>
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

#ifndef OTTER_WEBWIDGET_H
#define OTTER_WEBWIDGET_H

#include "../core/ActionExecutor.h"
#include "../core/NetworkManager.h"
#include "../core/PasswordsManager.h"
#include "../core/SessionsManager.h"
#include "../core/SpellCheckManager.h"

#include <QtGui/QHelpEvent>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslCipher>
#include <QtNetwork/QSslError>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QWidget>

namespace Otter
{

class ContentsDialog;
class ContentsWidget;
class Transfer;
class WebBackend;

class WebWidget : public QWidget, public ActionExecutor
{
	Q_OBJECT

public:
	enum ChangeWatcher
	{
		UnknownWatcher = 0,
		FeedsWatcher,
		LinksWatcher,
		MetaDataWatcher,
		SearchEnginesWatcher,
		StylesheetsWatcher
	};

	enum ContentState
	{
		UnknownContentState = 0,
		ApplicationContentState = 1,
		LocalContentState = 2,
		RemoteContentState = 4,
		SecureContentState = 8,
		TrustedContentState = 16,
		MixedContentState = 32,
		FraudContentState = 64
	};

	Q_DECLARE_FLAGS(ContentStates, ContentState)

	enum LoadingState
	{
		DeferredLoadingState = 0,
		OngoingLoadingState,
		FinishedLoadingState,
		CrashedLoadingState
	};

	enum FeaturePermission
	{
		UnknownFeature = 0,
		FullScreenFeature,
		GeolocationFeature,
		NotificationsFeature,
		PointerLockFeature,
		CaptureAudioFeature,
		CaptureVideoFeature,
		CaptureAudioVideoFeature,
		PlaybackAudioFeature
	};

	enum PermissionPolicy
	{
		DeniedPermission = 0,
		GrantedPermission,
		KeepAskingPermission
	};

	Q_DECLARE_FLAGS(PermissionPolicies, PermissionPolicy)

	enum FindFlag
	{
		NoFlagsFind = 0,
		BackwardFind = 1,
		CaseSensitiveFind = 2,
		ExactMatchFind = 4,
		HighlightAllFind = 8
	};

	Q_DECLARE_FLAGS(FindFlags, FindFlag)

	enum SaveFormat
	{
		UnknownSaveFormat = 0,
		CompletePageSaveFormat,
		MhtmlSaveFormat,
		PdfSaveFormat,
		SingleFileSaveFormat
	};

	enum PageInformation
	{
		UnknownInformation = 0,
		DocumentBytesReceivedInformation,
		DocumentBytesTotalInformation,
		DocumentLoadingProgressInformation,
		DocumentMimeTypeInformation,
		TotalBytesReceivedInformation,
		TotalBytesTotalInformation,
		TotalLoadingProgressInformation,
		RequestsBlockedInformation,
		RequestsFinishedInformation,
		RequestsStartedInformation,
		LoadingSpeedInformation,
		LoadingFinishedInformation,
		LoadingTimeInformation,
		LoadingMessageInformation
	};

	struct HitTestResult
	{
		enum HitTestFlag
		{
			NoTest = 0,
			IsContentEditableTest = 1,
			IsEmptyTest = 2,
			IsFormTest = 4,
			IsLinkFromSelectionTest = 8,
			IsSelectedTest = 16,
			IsSpellCheckEnabled = 32,
			MediaHasControlsTest = 64,
			MediaIsLoopedTest = 128,
			MediaIsMutedTest = 256,
			MediaIsPausedTest = 512
		};

		Q_DECLARE_FLAGS(HitTestFlags, HitTestFlag)

		QString title;
		QString tagName;
		QString alternateText;
		QString longDescription;
		QUrl formUrl;
		QUrl frameUrl;
		QUrl imageUrl;
		QUrl linkUrl;
		QUrl mediaUrl;
		QPoint hitPosition;
		QRect elementGeometry;
		qreal playbackRate = 1;
		HitTestFlags flags = NoTest;
	};

	struct LinkUrl final
	{
		QString title;
		QString mimeType;
		QUrl url;
	};

	struct SslInformation final
	{
		struct SslError final
		{
			QSslError error;
			QUrl url;
		};

		QSslCipher cipher;
		QVector<QSslCertificate> certificates;
		QVector<SslError> errors;
	};

	virtual void search(const QString &query, const QString &searchEngine);
	virtual void print(QPrinter *printer) = 0;
	void startWatchingChanges(QObject *object, ChangeWatcher watcher);
	void stopWatchingChanges(QObject *object, ChangeWatcher watcher);
	void showDialog(ContentsDialog *dialog, bool lockEventLoop = true);
	void setParent(QWidget *parent);
	virtual void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = {});
	void setWindowIdentifier(quint64 identifier);
	virtual WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = {}) const = 0;
	virtual QWidget* getInspector();
	virtual QWidget* getViewport();
	WebBackend* getBackend() const;
	virtual QString getTitle() const = 0;
	virtual QString getDescription() const;
	virtual QString getActiveStyleSheet() const;
	virtual QString getCharacterEncoding() const;
	virtual QString getSelectedText() const;
	QString getStatusMessage() const;
	QVariant getOption(int identifier, const QUrl &url = {}) const;
	virtual QVariant getPageInformation(PageInformation key) const;
	virtual QUrl getUrl() const = 0;
	QUrl getRequestedUrl() const;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap createThumbnail(const QSize &size = {});
	QPoint getClickPosition() const;
	virtual QPoint getScrollPosition() const = 0;
	virtual QRect getGeometry(bool excludeScrollBars = false) const;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	virtual LinkUrl getActiveFrame() const;
	virtual LinkUrl getActiveImage() const;
	virtual LinkUrl getActiveLink() const;
	virtual LinkUrl getActiveMedia() const;
	virtual SslInformation getSslInformation() const;
	virtual WindowHistoryInformation getHistory() const = 0;
	virtual HitTestResult getHitTestResult(const QPoint &position);
	virtual QStringList getStyleSheets() const;
	virtual QVector<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	virtual QVector<LinkUrl> getFeeds() const;
	virtual QVector<LinkUrl> getLinks() const;
	virtual QVector<LinkUrl> getSearchEngines() const;
	virtual QVector<NetworkManager::ResourceInformation> getBlockedRequests() const;
	QHash<int, QVariant> getOptions() const;
	virtual QMap<QByteArray, QByteArray> getHeaders() const;
	virtual QMultiMap<QString, QString> getMetaData() const;
	virtual WebWidget::ContentStates getContentState() const;
	virtual WebWidget::LoadingState getLoadingState() const = 0;
	quint64 getWindowIdentifier() const;
	virtual int getZoom() const = 0;
	virtual int findInPage(const QString &text, FindFlags flags = NoFlagsFind) = 0;
	bool hasOption(int identifier) const;
	virtual bool hasSelection() const;
	virtual bool hasWatchedChanges(ChangeWatcher watcher) const;
	virtual bool isAudible() const;
	virtual bool isAudioMuted() const;
	virtual bool isFullScreen() const;
	virtual bool isPrivate() const = 0;
	bool isWatchingChanges(ChangeWatcher watcher) const;

public slots:
	virtual void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	virtual void clearOptions();
	virtual void fillPassword(const PasswordsManager::PasswordInformation &password);
	virtual void showContextMenu(const QPoint &position = {});
	virtual void setActiveStyleSheet(const QString &styleSheet);
	virtual void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies);
	virtual void setOption(int identifier, const QVariant &value);
	virtual void setScrollPosition(const QPoint &position) = 0;
	virtual void setHistory(const WindowHistoryInformation &history) = 0;
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url, bool isTyped = true) = 0;
	void setRequestedUrl(const QUrl &url, bool isTyped = true, bool onlyUpdate = false);

protected:
	explicit WebWidget(const QVariantMap &parameters, WebBackend *backend, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	void openUrl(const QUrl &url, SessionsManager::OpenHints hints);
	void startReloadTimer();
	void startTransfer(Transfer *transfer);
	void handleToolTipEvent(QHelpEvent *event, QWidget *widget);
	void updateHitTestResult(const QPoint &position);
	virtual void updateWatchedData(ChangeWatcher watcher);
	void setClickPosition(const QPoint &position);
	QString suggestSaveFileName(const QString &extension) const;
	QString suggestSaveFileName(SaveFormat format) const;
	QString getSavePath(const QVector<SaveFormat> &allowedFormats, SaveFormat *selectedFormat) const;
	QString getOpenActionText(SessionsManager::OpenHints hints) const;
	static QString getFastForwardScript(bool isSelectingTheBestLink);
	HitTestResult getCurrentHitTestResult() const;
	PermissionPolicy getPermission(FeaturePermission feature, const QUrl &url) const;
	virtual int getAmountOfDeferredPlugins() const;
	virtual bool canGoBack() const;
	virtual bool canGoForward() const;
	virtual bool canFastForward() const;
	virtual bool canInspect() const;
	virtual bool canTakeScreenshot() const;
	virtual bool canRedo() const;
	virtual bool canUndo() const;
	virtual bool canShowContextMenu(const QPoint &position) const;
	virtual bool canViewSource() const;
	virtual bool isInspecting() const;
	virtual bool isPopup() const;
	virtual bool isScrollBar(const QPoint &position) const;

protected slots:
	void handleLoadingStateChange(WebWidget::LoadingState state);
	void handleWindowCloseRequest();
	void notifyRedoActionStateChanged();
	void notifyUndoActionStateChanged();
	void setStatusMessage(const QString &message);
	void setStatusMessageOverride(const QString &message);

private:
	ContentsWidget *m_parent;
	WebBackend *m_backend;
	QUrl m_requestedUrl;
	QString m_statusMessage;
	QString m_statusMessageOverride;
	QPoint m_clickPosition;
	QHash<int, QVariant> m_options;
	QHash<ChangeWatcher, QVector<QObject*> > m_changeWatchers;
	HitTestResult m_hitResult;
	quint64 m_windowIdentifier;
	int m_loadingTime;
	int m_loadingTimer;
	int m_reloadTimer;

	static QString m_fastForwardScript;

signals:
	void aboutToNavigate();
	void aboutToReload();
	void needsAttention();
	void requestedCloseWindow();
	void requestedNewWindow(WebWidget *widget, SessionsManager::OpenHints hints, const QVariantMap &parameters);
	void requestedPopupWindow(const QUrl &parentUrl, const QUrl &popupUrl);
	void requestedPermission(WebWidget::FeaturePermission feature, const QUrl &url, bool isCancellation);
	void requestedSavePassword(const PasswordsManager::PasswordInformation &password, bool isUpdate);
	void requestedGeometryChange(const QRect &geometry);
	void requestedInspectorVisibilityChange(bool isVisible);
	void geometryChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
	void categorizedActionsStateChanged(const QVector<int> &categories);
	void contentStateChanged(WebWidget::ContentStates state);
	void loadingStateChanged(WebWidget::LoadingState state);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void optionChanged(int identifier, const QVariant &value);
	void watchedDataChanged(ChangeWatcher watcher);
	void zoomChanged(int zoom);
	void isAudibleChanged(bool isAudible);
	void isFullScreenChanged(bool isFullScreen);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::WebWidget::ContentStates)
Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::WebWidget::PermissionPolicies)

#endif
