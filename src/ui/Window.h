/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QUndoStack>
#include <QtWidgets/QWidget>

namespace Otter
{

struct LinkUrl
{
	QString title;
	QString mimeType;
	QUrl url;
};

class AddressWidget;
class ContentsWidget;
class SearchWidget;
class ToolBarWidget;

class Window : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
	Q_PROPERTY(QString type READ getType)
	Q_PROPERTY(QUrl url READ getUrl WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QIcon icon READ getIcon NOTIFY iconChanged)
	Q_PROPERTY(QPixmap thumbnail READ getThumbnail)
	Q_PROPERTY(WindowsManager::LoadingState loadingState READ getLoadingState NOTIFY loadingStateChanged)
	Q_PROPERTY(bool canClone READ canClone)
	Q_PROPERTY(bool isPinned READ isPinned WRITE setPinned NOTIFY isPinnedChanged)
	Q_PROPERTY(bool isPrivate READ isPrivate)

public:
	explicit Window(bool isPrivate, ContentsWidget *widget = NULL, QWidget *parent = NULL);

	void clear();
	void attachAddressWidget(AddressWidget *widget);
	void detachAddressWidget(AddressWidget *widget);
	void attachSearchWidget(SearchWidget *widget);
	void detachSearchWidget(SearchWidget *widget);
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
	QDateTime getLastActivity() const;
	WindowHistoryInformation getHistory() const;
	SessionWindow getSession() const;
	QSize sizeHint() const;
	WindowsManager::LoadingState getLoadingState() const;
	WindowsManager::ContentStates getContentState() const;
	quint64 getIdentifier() const;
	bool canClone() const;
	bool isAboutToClose() const;
	bool isPinned() const;
	bool isPrivate() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = QVariantMap());
	void close();
	void search(const QString &query, const QString &searchEngine);
	void markActive();
	void setOption(const QString &key, const QVariant &value);
	void setSearchEngine(const QString &searchEngine);
	void setUrl(const QUrl &url, bool typed = true);
	void setControlsHidden(bool hidden);
	void setPinned(bool pinned);

protected:
	void focusInEvent(QFocusEvent *event);
	void setContentsWidget(ContentsWidget *widget);
	AddressWidget* findAddressWidget() const;

protected slots:
	void handleIconChanged(const QIcon &icon);
	void handleOpenUrlRequest(const QUrl &url, WindowsManager::OpenHints hints);
	void handleSearchRequest(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints = WindowsManager::DefaultOpen);
	void handleGeometryChangeRequest(const QRect &geometry);
	void notifyLoadingStateChanged(bool loading);
	void notifyRequestedCloseWindow();
	void updateNavigationBar();

private:
	ToolBarWidget *m_navigationBar;
	ContentsWidget *m_contentsWidget;
	QString m_searchEngine;
	QDateTime m_lastActivity;
	SessionWindow m_session;
	QList<QPointer<AddressWidget> > m_addressWidgets;
	QList<QPointer<SearchWidget> > m_searchWidgets;
	quint64 m_identifier;
	bool m_areControlsHidden;
	bool m_isAboutToClose;
	bool m_isPinned;
	bool m_isPrivate;

	static quint64 m_identifierCounter;

signals:
	void aboutToClose();
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints = WindowsManager::DefaultOpen);
	void requestedAddBookmark(const QUrl &url, const QString &title, const QString &description);
	void requestedEditBookmark(const QUrl &url);
	void requestedNewWindow(ContentsWidget *widget, WindowsManager::OpenHints hints);
	void requestedCloseWindow(Window *window);
	void canZoomChanged(bool can);
	void searchEngineChanged(const QString &searchEngine);
	void statusMessageChanged(const QString &message);
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url, bool force = false);
	void iconChanged(const QIcon &icon);
	void contentStateChanged(WindowsManager::ContentStates state);
	void loadingStateChanged(WindowsManager::LoadingState loading);
	void zoomChanged(int zoom);
	void isPinnedChanged(bool pinned);
	void widgetChanged();
};

}

#endif
