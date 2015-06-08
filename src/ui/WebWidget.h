/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_WEBWIDGET_H
#define OTTER_WEBWIDGET_H

#include "../core/SessionsManager.h"
#include "Window.h"

#include <QtGui/QHelpEvent>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QUndoStack>

namespace Otter
{

class ContentsDialog;
class Menu;
class WebBackend;

class WebWidget : public QWidget
{
	Q_OBJECT

public:
	enum ScrollMode
	{
		NoScroll = 0,
		MoveScroll = 1,
		DragScroll = 2
	};

	enum ScrollDirection
	{
		NoDirection = 0,
		TopDirection = 1,
		BottomDirection = 2,
		RightDirection = 4,
		LeftDirection = 8
	};

	Q_DECLARE_FLAGS(ScrollDirections, ScrollDirection)

	enum MenuFlag
	{
		NoMenu = 0,
		StandardMenu = 1,
		LinkMenu = 2,
		MailMenu = 4,
		ImageMenu = 8,
		MediaMenu = 16,
		SelectionMenu = 32,
		EditMenu = 64,
		FrameMenu = 128,
		FormMenu = 256
	};

	Q_DECLARE_FLAGS(MenuFlags, MenuFlag)

	enum PermissionPolicy
	{
		DeniedPermission = 0,
		GrantedPermission = 1,
		RememberPermission = 2
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
		MediaHasControlsTest = 16,
		MediaIsLoopedTest = 32,
		MediaIsMutedTest = 64,
		MediaIsPausedTest = 128
	};

	Q_DECLARE_FLAGS(HitTestFlags, HitTestFlag)

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
		HitTestFlags flags;

		HitTestResult() : flags(NoFlagsTest) {}

		explicit HitTestResult(const QVariant &result)
		{
			const QVariantMap map = result.toMap();
			const QVariantMap geometryMap = map.value(QLatin1String("geometry")).toMap();

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
			flags = static_cast<HitTestFlags>(map.value(QLatin1String("flags")).toInt());
		}
	};

	virtual void search(const QString &query, const QString &engine);
	virtual void print(QPrinter *printer) = 0;
	void showDialog(ContentsDialog *dialog);
	virtual WebWidget* clone(bool cloneHistory = true) = 0;
	virtual Action* getAction(int identifier);
	WebBackend* getBackend();
	virtual QString getTitle() const = 0;
	virtual QString getSelectedText() const;
	QString getStatusMessage() const;
	QVariant getOption(const QString &key, const QUrl &url = QUrl()) const;
	virtual QUrl getUrl() const = 0;
	QUrl getRequestedUrl() const;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() = 0;
	QPoint getClickPosition() const;
	virtual QPoint getScrollPosition() const = 0;
	virtual QRect getProgressBarGeometry() const = 0;
	virtual WindowHistoryInformation getHistory() const = 0;
	virtual HitTestResult getHitTestResult(const QPoint &position);
	QStringList getAlternateStyleSheets() const;
	virtual QList<LinkUrl> getFeeds() const;
	virtual QList<LinkUrl> getSearchEngines() const;
	QVariantHash getOptions() const;
	virtual QVariantHash getStatistics() const;
	virtual QHash<QByteArray, QByteArray> getHeaders() const;
	ScrollMode getScrollMode() const;
	virtual int getZoom() const = 0;
	virtual bool handleContextMenuEvent(QContextMenuEvent *event, bool canPropagate = true, QObject *sender = NULL);
	virtual bool handleMousePressEvent(QMouseEvent *event, bool canPropagate = true, QObject *sender = NULL);
	virtual bool handleMouseReleaseEvent(QMouseEvent *event, bool canPropagate = true, QObject *sender = NULL);
	virtual bool handleMouseDoubleClickEvent(QMouseEvent *event, bool canPropagate = true, QObject *sender = NULL);
	virtual bool handleWheelEvent(QWheelEvent *event, bool canPropagate = true, QObject *sender = NULL);
	bool hasOption(const QString &key) const;
	virtual bool hasSelection() const;
	virtual bool isLoading() const = 0;
	virtual bool isPrivate() const = 0;
	virtual bool findInPage(const QString &text, FindFlags flags = NoFlagsFind) = 0;

public slots:
	virtual void triggerAction(int identifier, bool checked = false) = 0;
	virtual void clearOptions();
	virtual void clearSelection() = 0;
	virtual void goToHistoryIndex(int index) = 0;
	virtual void showContextMenu(const QPoint &position = QPoint(), MenuFlags flags = NoMenu);
	virtual void setPermission(const QString &key, const QUrl &url, PermissionPolicies policies);
	virtual void setOption(const QString &key, const QVariant &value);
	virtual void setScrollPosition(const QPoint &position) = 0;
	virtual void setHistory(const WindowHistoryInformation &history) = 0;
	void setScrollMode(ScrollMode mode);
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url, bool typed = true) = 0;
	void setRequestedUrl(const QUrl &url, bool typed = true, bool onlyUpdate = false);

protected:
	explicit WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void openUrl(const QUrl &url, OpenHints hints);
	virtual void pasteText(const QString &text) = 0;
	void startReloadTimer();
	void handleToolTipEvent(QHelpEvent *event, QWidget *widget);
	void updateHitTestResult(const QPoint &position);
	void setAlternateStyleSheets(const QStringList &styleSheets);
	void setClickPosition(const QPoint &position);
	virtual void setOptions(const QVariantHash &options);
	Action* getExistingAction(int identifier);
	QMenu* getPasteNoteMenu();
	QMenu* getReloadTimeMenu();
	QMenu* getQuickSearchMenu();
	QString suggestSaveFileName() const;
	HitTestResult getCurrentHitTestResult() const;
	virtual int getAmountOfNotLoadedPlugins() const;
	virtual bool canGoBack() const;
	virtual bool canGoForward() const;
	virtual bool canShowContextMenu(const QPoint &position) const;
	virtual bool isScrollBar(const QPoint &position) const;

protected slots:
	void triggerAction();
	void pasteNote(QAction *action);
	void reloadTimeMenuAboutToShow();
	void quickSearch(QAction *action);
	void quickSearchMenuAboutToShow();
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
	QMenu *m_reloadTimeMenu;
	QMenu *m_quickSearchMenu;
	QUrl m_requestedUrl;
	QString m_javaScriptStatusMessage;
	QString m_overridingStatusMessage;
	QPoint m_beginCursorPosition;
	QPoint m_beginScrollPosition;
	QPoint m_clickPosition;
	QStringList m_alternateStyleSheets;
	QHash<int, Action*> m_actions;
	QVariantHash m_options;
	HitTestResult m_hitResult;
	ScrollMode m_scrollMode;
	int m_reloadTimer;
	int m_scrollTimer;
	bool m_ignoreContextMenu;
	bool m_ignoreContextMenuNextTime;
	bool m_isUsingRockerNavigation;

	static QMap<int, QPixmap> m_scrollCursors;

signals:
	void aboutToReload();
	void requestedCloseWindow();
	void requestedOpenUrl(QUrl url, OpenHints hints);
	void requestedAddBookmark(QUrl url, QString title, QString description);
	void requestedEditBookmark(QUrl url);
	void requestedNewWindow(WebWidget *widget, OpenHints hints);
	void requestedSearch(QString query, QString search, OpenHints hints);
	void requestedPermission(QString option, QUrl url, bool cancel);
	void requestedGeometryChange(const QRect &geometry);
	void progressBarGeometryChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
	void loadProgress(int progress);
	void loadMessageChanged(QString message);
	void loadStatusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
