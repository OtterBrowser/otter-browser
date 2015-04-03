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

	virtual void search(const QString &query, const QString &engine);
	virtual void print(QPrinter *printer) = 0;
	void showDialog(ContentsDialog *dialog);
	virtual WebWidget* clone(bool cloneHistory = true) = 0;
	virtual Action* getAction(int identifier) = 0;
	WebBackend* getBackend();
	virtual QString getTitle() const = 0;
	virtual QString getSelectedText() const;
	QString getStatusMessage() const;
	QVariant getOption(const QString &key, const QUrl &url = QUrl()) const;
	virtual QUrl getUrl() const = 0;
	QUrl getRequestedUrl() const;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() = 0;
	virtual QPoint getScrollPosition() const = 0;
	virtual QRect getProgressBarGeometry() const = 0;
	virtual WindowHistoryInformation getHistory() const = 0;
	QStringList getAlternateStyleSheets() const;
	QVariantHash getOptions() const;
	virtual QHash<QByteArray, QByteArray> getHeaders() const = 0;
	virtual QVariantHash getStatistics() const = 0;
	ScrollMode getScrollMode() const;
	virtual int getZoom() const = 0;
	bool hasOption(const QString &key) const;
	virtual bool isLoading() const = 0;
	virtual bool isPrivate() const = 0;
	virtual bool findInPage(const QString &text, FindFlags flags = NoFlagsFind) = 0;

public slots:
	virtual void triggerAction(int identifier, bool checked = false) = 0;
	virtual void clearOptions();
	virtual void clearSelection() = 0;
	virtual void goToHistoryIndex(int index) = 0;
	void showContextMenu(const QPoint &position, MenuFlags flags);
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
	virtual void pasteText(const QString &text) = 0;
	void startReloadTimer();
	void setAlternateStyleSheets(const QStringList &styleSheets);
	virtual void setOptions(const QVariantHash &options);
	QMenu* getPasteNoteMenu();
	QMenu* getReloadTimeMenu();
	QMenu* getQuickSearchMenu();

protected slots:
	void triggerAction();
	void pasteNote(QAction *action);
	void reloadTimeMenuAboutToShow();
	void quickSearch(QAction *action);
	void quickSearchMenuAboutToShow();
	void updateQuickSearch();
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
	QStringList m_alternateStyleSheets;
	QVariantHash m_options;
	ScrollMode m_scrollMode;
	int m_reloadTimer;
	int m_scrollTimer;

	static QMap<int, QPixmap> m_scrollCursors;

signals:
	void aboutToReload();
	void requestedCloseWindow();
	void requestedOpenUrl(QUrl url, OpenHints hints);
	void requestedAddBookmark(QUrl url, QString title, QString description);
	void requestedNewWindow(WebWidget *widget, OpenHints hints);
	void requestedSearch(QString query, QString search, OpenHints hints);
	void requestedPermission(QString option, QUrl url, bool cancel);
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
