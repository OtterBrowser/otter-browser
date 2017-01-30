/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "../core/NetworkManager.h"
#include "../core/PasswordsManager.h"
#include "../core/SessionsManager.h"
#include "../core/SpellCheckManager.h"
#include "Window.h"

#include <QtGui/QHelpEvent>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslCipher>
#include <QtNetwork/QSslError>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QUndoStack>

namespace Otter
{

class ContentsDialog;
class Menu;
class Transfer;
class WebBackend;

class WebWidget : public QWidget
{
	Q_OBJECT

public:
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
		GrantedPermission = 1,
		KeepAskingPermission = 2
	};

	Q_DECLARE_FLAGS(PermissionPolicies, PermissionPolicy)

	enum FindFlag
	{
		NoFlagsFind = 0,
		BackwardFind = 1,
		CaseSensitiveFind = 2,
		HighlightAllFind = 4
	};

	Q_DECLARE_FLAGS(FindFlags, FindFlag)

	enum HitTestFlag
	{
		NoFlagsTest = 0,
		IsContentEditableTest = 1,
		IsEmptyTest = 2,
		IsFormTest = 4,
		IsSelectedTest = 8,
		IsSpellCheckEnabled = 16,
		MediaHasControlsTest = 32,
		MediaIsLoopedTest = 64,
		MediaIsMutedTest = 128,
		MediaIsPausedTest = 256
	};

	Q_DECLARE_FLAGS(HitTestFlags, HitTestFlag)

	enum SaveFormat
	{
		UnknownSaveFormat = 0,
		CompletePageSaveFormat = 1,
		MhtmlSaveFormat = 2,
		SingleHtmlFileSaveFormat = 4
	};

	Q_DECLARE_FLAGS(SaveFormats, SaveFormat)

	enum PageInformation
	{
		UnknownInformation = 0,
		DocumentLoadingProgressInformation,
		TotalLoadingProgressInformation,
		BytesReceivedInformation,
		BytesTotalInformation,
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
		QString title;
		QString tagName;
		QString alternateText;
		QString longDescription;
		QUrl formUrl;
		QUrl frameUrl;
		QUrl imageUrl;
		QUrl linkUrl;
		QUrl mediaUrl;
		QPoint position;
		QRect geometry;
		HitTestFlags flags = NoFlagsTest;

		explicit HitTestResult(const QVariant &result)
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
			geometry = QRect(geometryMap.value(QLatin1String("x")).toInt(), geometryMap.value(QLatin1String("y")).toInt(), geometryMap.value(QLatin1String("w")).toInt(), geometryMap.value(QLatin1String("h")).toInt());
			position = map.value(QLatin1String("position")).toPoint();
			flags = static_cast<HitTestFlags>(map.value(QLatin1String("flags")).toInt());
		}

		HitTestResult()
		{
		}
	};

	struct SslInformation
	{
		QSslCipher cipher;
		QList<QSslCertificate> certificates;
		QList<QPair<QUrl, QSslError> > errors;
	};

	virtual void search(const QString &query, const QString &searchEngine);
	virtual void print(QPrinter *printer) = 0;
	void showDialog(ContentsDialog *dialog, bool lockEventLoop = true);
	virtual void setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions = QStringList());
	void setWindowIdentifier(quint64 identifier);
	virtual WebWidget* clone(bool cloneHistory = true, bool isPrivate = false, const QStringList &excludedOptions = QStringList()) = 0;
	virtual QWidget* getViewport();
	virtual Action* getAction(int identifier);
	WebBackend* getBackend();
	virtual QString getTitle() const = 0;
	virtual QString getActiveStyleSheet() const;
	virtual QString getCharacterEncoding() const;
	virtual QString getSelectedText() const;
	QString getStatusMessage() const;
	QVariant getOption(int identifier, const QUrl &url = QUrl()) const;
	virtual QVariant getPageInformation(WebWidget::PageInformation key) const;
	virtual QUrl getUrl() const = 0;
	QUrl getRequestedUrl() const;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() = 0;
	QPoint getClickPosition() const;
	virtual QPoint getScrollPosition() const = 0;
	virtual QRect getProgressBarGeometry() const;
	virtual SslInformation getSslInformation() const;
	virtual WindowHistoryInformation getHistory() const = 0;
	virtual HitTestResult getHitTestResult(const QPoint &position);
	virtual QStringList getStyleSheets() const;
	virtual QList<LinkUrl> getFeeds() const;
	virtual QList<LinkUrl> getSearchEngines() const;
	virtual QList<NetworkManager::ResourceInformation> getBlockedRequests() const;
	QHash<int, QVariant> getOptions() const;
	virtual QHash<QByteArray, QByteArray> getHeaders() const;
	virtual WindowsManager::ContentStates getContentState() const;
	virtual WindowsManager::LoadingState getLoadingState() const = 0;
	quint64 getWindowIdentifier() const;
	virtual int getZoom() const = 0;
	bool hasOption(int identifier) const;
	virtual bool hasSelection() const;
	virtual bool isAudible() const;
	virtual bool isAudioMuted() const;
	virtual bool isFullScreen() const;
	virtual bool isPrivate() const = 0;
	virtual bool findInPage(const QString &text, FindFlags flags = NoFlagsFind) = 0;

public slots:
	virtual void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap()) = 0;
	virtual void clearOptions();
	virtual void fillPassword(const PasswordsManager::PasswordInformation &password);
	virtual void goToHistoryIndex(int index) = 0;
	virtual void removeHistoryIndex(int index, bool purge = false) = 0;
	virtual void showContextMenu(const QPoint &position = QPoint());
	virtual void setActiveStyleSheet(const QString &styleSheet);
	virtual void setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies);
	virtual void setOption(int identifier, const QVariant &value);
	virtual void setScrollPosition(const QPoint &position) = 0;
	virtual void setHistory(const WindowHistoryInformation &history) = 0;
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url, bool isTyped = true) = 0;
	void setRequestedUrl(const QUrl &url, bool isTyped = true, bool onlyUpdate = false);

protected:
	explicit WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent = nullptr);

	void timerEvent(QTimerEvent *event);
	void bounceAction(int identifier, QVariantMap parameters);
	void openUrl(const QUrl &url, WindowsManager::OpenHints hints);
	virtual void pasteText(const QString &text) = 0;
	void startReloadTimer();
	void startTransfer(Transfer *transfer);
	void handleToolTipEvent(QHelpEvent *event, QWidget *widget);
	void updateHitTestResult(const QPoint &position);
	void setClickPosition(const QPoint &position);
	Action* getExistingAction(int identifier);
	QString suggestSaveFileName(SaveFormat format) const;
	HitTestResult getCurrentHitTestResult() const;
	virtual QList<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	PermissionPolicy getPermission(FeaturePermission feature, const QUrl &url) const;
	virtual SaveFormats getSupportedSaveFormats() const;
	virtual int getAmountOfNotLoadedPlugins() const;
	bool calculateCheckedState(int identifier, const QVariantMap &parameters = QVariantMap());
	virtual bool canGoBack() const;
	virtual bool canGoForward() const;
	virtual bool canShowContextMenu(const QPoint &position) const;
	virtual bool canViewSource() const;
	virtual bool isInspecting() const;
	virtual bool isScrollBar(const QPoint &position) const;

protected slots:
	void triggerAction();
	void pasteNote(QAction *action);
	void selectDictionary(QAction *action);
	void selectDictionaryMenuAboutToShow();
	void reloadTimeMenuAboutToShow();
	void openInApplication(QAction *action);
	void openInApplicationMenuAboutToShow();
	void quickSearch(QAction *action);
	void quickSearchMenuAboutToShow();
	void handleLoadingStateChange(WindowsManager::LoadingState state);
	void handleAudibleStateChange(bool isAudible);
	void updatePasswords();
	void updateQuickSearch();
	virtual void updatePageActions(const QUrl &url);
	virtual void updateNavigationActions();
	virtual void updateEditActions();
	virtual void updateLinkActions();
	virtual void updateFrameActions();
	virtual void updateImageActions();
	virtual void updateMediaActions();
	virtual void updateBookmarkActions();
	void setReloadTime(QAction *action);
	void setStatusMessage(const QString &message, bool override = false);

private:
	WebBackend *m_backend;
	Menu *m_pasteNoteMenu;
	QMenu *m_linkApplicationsMenu;
	QMenu *m_frameApplicationsMenu;
	QMenu *m_pageApplicationsMenu;
	QMenu *m_reloadTimeMenu;
	QMenu *m_quickSearchMenu;
	QUrl m_requestedUrl;
	QString m_javaScriptStatusMessage;
	QString m_overridingStatusMessage;
	QPoint m_clickPosition;
	QHash<int, Action*> m_actions;
	QHash<int, QVariant> m_options;
	HitTestResult m_hitResult;
	quint64 m_windowIdentifier;
	int m_loadingTime;
	int m_loadingTimer;
	int m_reloadTimer;

signals:
	void aboutToNavigate();
	void aboutToReload();
	void needsAttention();
	void requestedCloseWindow();
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedAddBookmark(const QUrl &url, const QString &title, const QString &description);
	void requestedEditBookmark(const QUrl &url);
	void requestedNewWindow(WebWidget *widget, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &search, WindowsManager::OpenHints hints);
	void requestedPopupWindow(const QUrl &parentUrl, const QUrl &popupUrl);
	void requestedPermission(WebWidget::FeaturePermission feature, const QUrl &url, bool cancel);
	void requestedSavePassword(const PasswordsManager::PasswordInformation &password, bool isUpdate);
	void requestedGeometryChange(const QRect &geometry);
	void progressBarGeometryChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void contentStateChanged(WindowsManager::ContentStates state);
	void loadingStateChanged(WindowsManager::LoadingState state);
	void pageInformationChanged(WebWidget::PageInformation, const QVariant &value);
	void requestBlocked(const NetworkManager::ResourceInformation &request);
	void zoomChanged(int zoom);
	void isFullScreenChanged(bool isFullScreen);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::WebWidget::PermissionPolicies)

#endif
