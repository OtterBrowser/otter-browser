/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

enum FindFlag
{
	NoFlagsFind = 0,
	BackwardFind = 1,
	CaseSensitiveFind = 2,
	HighlightAllFind = 4
};

Q_DECLARE_FLAGS(FindFlags, FindFlag)

class WebBackend;

class WebWidget : public QWidget
{
	Q_OBJECT

public:
	virtual void search(const QString &query, const QString &engine);
	virtual void print(QPrinter *printer) = 0;
	void setQuickSearchEngine(const QString &engine);
	virtual WebWidget* clone(bool cloneHistory = true) = 0;
	virtual QAction* getAction(ActionIdentifier action) = 0;
	virtual QUndoStack* getUndoStack() = 0;
	WebBackend* getBackend();
	QMenu* getReloadTimeMenu();
	QMenu* getQuickSearchMenu();
	QString getQuickSearchEngine() const;
	virtual QString getDefaultCharacterEncoding() const = 0;
	virtual QString getTitle() const = 0;
	virtual QString getSelectedText() const;
	QString getStatusMessage() const;
	QVariant getOption(const QString &key, const QUrl &url = QUrl()) const;
	virtual QVariant evaluateJavaScript(const QString &script) = 0;
	virtual QUrl getUrl() const = 0;
	QUrl getRequestedUrl() const;
	virtual QIcon getIcon() const = 0;
	virtual QPixmap getThumbnail() = 0;
	virtual QRect getProgressBarGeometry() const = 0;
	virtual WindowHistoryInformation getHistory() const = 0;
	QVariantHash getOptions() const;
	virtual QHash<QByteArray, QByteArray> getHeaders() const = 0;
	virtual QVariantHash getStatistics() const = 0;
	virtual QPair<QString, QString> getUserAgent() const = 0;
	virtual int getReloadTime() const;
	virtual int getZoom() const = 0;
	virtual bool isLoading() const = 0;
	virtual bool isPrivate() const = 0;
	virtual bool find(const QString &text, FindFlags flags = HighlightAllFind) = 0;

public slots:
	virtual void clearOptions();
	virtual void goToHistoryIndex(int index) = 0;
	virtual void triggerAction(ActionIdentifier action, bool checked = false) = 0;
	void showContextMenu(const QPoint &position, MenuFlags flags);
	virtual void setOption(const QString &key, const QVariant &value);
	virtual void setDefaultCharacterEncoding(const QString &encoding) = 0;
	virtual void setUserAgent(const QString &identifier, const QString &value) = 0;
	virtual void setHistory(const WindowHistoryInformation &history) = 0;
	virtual void setReloadTime(int time);
	virtual void setZoom(int zoom) = 0;
	virtual void setUrl(const QUrl &url, bool typed = true) = 0;
	void setRequestedUrl(const QUrl &url, bool typed = true, bool onlyUpdate = false);

protected:
	explicit WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent = NULL);

	void timerEvent(QTimerEvent *event);
	void startReloadTimer();
	virtual void setOptions(const QVariantHash &options);

protected slots:
	void reloadTimeMenuAboutToShow();
	void quickSearch(QAction *action);
	void quickSearchMenuAboutToShow();
	void updateQuickSearch();
	void setReloadTime(QAction *action);
	void setStatusMessage(const QString &message, bool override = false);

private:
	WebBackend *m_backend;
	QMenu *m_reloadTimeMenu;
	QMenu *m_quickSearchMenu;
	QUrl m_requestedUrl;
	QString m_quickSearchEngine;
	QString m_javaScriptStatusMessage;
	QString m_overridingStatusMessage;
	QVariantHash m_options;
	int m_reloadTime;
	int m_reloadTimer;

signals:
	void requestedOpenUrl(QUrl url, OpenHints hints);
	void requestedAddBookmark(QUrl url, QString title);
	void requestedNewWindow(WebWidget *widget, OpenHints hints);
	void requestedSearch(QString query, QString search);
	void actionsChanged();
	void progressBarGeometryChanged();
	void quickSearchEngineChanged();
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingChanged(bool loading);
	void zoomChanged(int zoom);
	void loadProgress(int progress);
	void loadStatusChanged(int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed);
};

}

#endif
