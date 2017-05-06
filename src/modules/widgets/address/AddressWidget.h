/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ADDRESSWIDGET_H
#define OTTER_ADDRESSWIDGET_H

#include "../../../core/SessionsManager.h"
#include "../../../ui/LineEditWidget.h"

#include <QtCore/QPointer>
#include <QtCore/QUrl>
#include <QtWidgets/QStyledItemDelegate>

namespace Otter
{

class AddressCompletionModel;
class BookmarksItem;
class ItemViewWidget;
class Window;

class AddressDelegate final : public QStyledItemDelegate
{
	Q_OBJECT

public:
	enum DisplayMode
	{
		CompactMode = 0,
		ColumnsMode
	};

	enum ViewMode
	{
		CompletionMode = 0,
		HistoryMode
	};

	explicit AddressDelegate(const QString &highlight, ViewMode mode, QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	QString highlightText(const QString &text, QString html = QString()) const;
	int calculateLength(const QStyleOptionViewItem &option, const QString &text, int length = 0) const;

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	QString m_highlight;
	DisplayMode m_displayMode;
	ViewMode m_viewMode;
};

class AddressWidget final : public LineEditWidget
{
	Q_OBJECT
	Q_ENUMS(EntryIdentifier)

public:
	enum CompletionMode
	{
		NoCompletionMode = 0,
		InlineCompletionMode = 1,
		PopupCompletionMode = 2
	};

	Q_DECLARE_FLAGS(CompletionModes, CompletionMode)

	enum EntryIdentifier
	{
		UnknownEntry = 0,
		AddressEntry,
		WebsiteInformationEntry,
		FaviconEntry,
		ListFeedsEntry,
		BookmarkEntry,
		LoadPluginsEntry,
		FillPasswordEntry,
		HistoryDropdownEntry
	};

	struct EntryDefinition
	{
		QString title;
		QIcon icon;
		QRect rectangle;
		QIcon::Mode mode = QIcon::Normal;
		EntryIdentifier identifier = UnknownEntry;
	};

	explicit AddressWidget(Window *window, QWidget *parent = nullptr);

	QUrl getUrl() const;
	bool event(QEvent *event) override;

public slots:
	void handleUserInput(const QString &text, SessionsManager::OpenHints hints = SessionsManager::DefaultOpen);
	void setWindow(Window *window = nullptr);
	void setUrl(const QUrl &url, bool force = false);

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void showCompletion(bool isTypedHistory);
	EntryIdentifier getEntry(const QPoint &position) const;
	bool startDrag(QMouseEvent *event);

protected slots:
	void addBookmark(QAction *action);
	void openFeed(QAction *action);
	void openUrl(const QString &url);
	void openUrl(const QModelIndex &index);
	void removeEntry();
	void handleOptionChanged(int identifier, const QVariant &value);
	void updateGeometries();
	void setCompletion(const QString &filter);
	void setIcon(const QIcon &icon);
	void setTextFromIndex(const QModelIndex &index);

private:
	QPointer<Window> m_window;
	AddressCompletionModel *m_completionModel;
	QPoint m_dragStartPosition;
	QVector<EntryIdentifier> m_layout;
	QHash<EntryIdentifier, EntryDefinition> m_entries;
	EntryIdentifier m_clickedEntry;
	EntryIdentifier m_hoveredEntry;
	CompletionModes m_completionModes;
	SessionsManager::OpenHints m_hints;
	bool m_isNavigatingCompletion;
	bool m_isUsingSimpleMode;

	static int m_entryIdentifierEnumerator;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, SessionsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, SessionsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::AddressWidget::CompletionModes)

#endif
