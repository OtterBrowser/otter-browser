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

#ifndef OTTER_WINDOW_H
#define OTTER_WINDOW_H

#include "../core/SessionsManager.h"
#include "../core/WindowsManager.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QWidget>

namespace Otter
{

enum WindowLoadingState
{
	DelayedState = 0,
	LoadingState = 1,
	LoadedState = 2
};

class AddressWidget;
class ContentsWidget;
class SearchWidget;

class Window : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
	Q_PROPERTY(QString type READ getType)
	Q_PROPERTY(QUrl url READ getUrl WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QIcon icon READ getIcon NOTIFY iconChanged)
	Q_PROPERTY(QPixmap thumbnail READ getThumbnail)
	Q_PROPERTY(WindowLoadingState loadingState READ getLoadingState NOTIFY loadingStateChanged)
	Q_PROPERTY(bool canClone READ canClone)
	Q_PROPERTY(bool isPinned READ isPinned WRITE setPinned NOTIFY isPinnedChanged)
	Q_PROPERTY(bool isPrivate READ isPrivate)

public:
	explicit Window(bool isPrivate, ContentsWidget *widget, QWidget *parent = NULL);

	void clear();
	void setSession(const SessionWindow &session);
	Window* clone(bool cloneHistory = true, QWidget *parent = NULL);
	ContentsWidget* getContentsWidget();
	QVariant getOption(const QString &key) const;
	QString getSearchEngine() const;
	QString getTitle() const;
	QLatin1String getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;
	QPixmap getThumbnail() const;
	WindowHistoryInformation getHistory() const;
	SessionWindow getSession() const;
	WindowLoadingState getLoadingState() const;
	qint64 getIdentifier() const;
	bool canClone() const;
	bool isPinned() const;
	bool isPrivate() const;
	bool isUrlEmpty() const;

public slots:
	void triggerAction(int identifier, bool checked = false);
	void close();
	void search(const QString &query, const QString &engine);
	void setOption(const QString &key, const QVariant &value);
	void setSearchEngine(const QString &engine);
	void setUrl(const QUrl &url, bool typed = true);
	void setControlsHidden(bool hidden);
	void setPinned(bool pinned);

protected:
	void showEvent(QShowEvent *event);
	void focusInEvent(QFocusEvent *event);
	void setContentsWidget(ContentsWidget *widget);

protected slots:
	void handleOpenUrlRequest(const QUrl &url, OpenHints hints);
	void handleSearchRequest(const QString &query, const QString &engine, OpenHints hints = DefaultOpen);
	void notifyLoadingStateChanged(bool loading);
	void notifyRequestedCloseWindow();

private:
	QWidget *m_navigationBar;
	AddressWidget *m_addressWidget;
	SearchWidget *m_searchWidget;
	ContentsWidget *m_contentsWidget;
	SessionWindow m_session;
	qint64 m_identifier;
	bool m_areControlsHidden;
	bool m_isPinned;
	bool m_isPrivate;

	static qint64 m_identifierCounter;

signals:
	void aboutToClose();
	void requestedOpenUrl(QUrl url, OpenHints hints);
	void requestedOpenBookmark(BookmarksItem *bookmark, OpenHints hints);
	void requestedSearch(QString query, QString engine, OpenHints hints = DefaultOpen);
	void requestedAddBookmark(QUrl url, QString title);
	void requestedNewWindow(ContentsWidget *widget, OpenHints hints);
	void requestedCloseWindow(Window *window);
	void canZoomChanged(bool can);
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void loadingStateChanged(WindowLoadingState loading);
	void zoomChanged(int zoom);
	void isPinnedChanged(bool pinned);
};

}

#endif
