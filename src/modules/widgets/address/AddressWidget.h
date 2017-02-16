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

#include "../../../core/WindowsManager.h"
#include "../../../ui/LineEditWidget.h"

#include <QtCore/QPointer>
#include <QtCore/QTime>
#include <QtCore/QUrl>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QLabel>

namespace Otter
{

class AddressCompletionModel;
class ItemViewWidget;
class LineEditWidget;
class Window;

class AddressDelegate : public QItemDelegate
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

	explicit AddressDelegate(ViewMode mode, QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected slots:
	void optionChanged(int identifier, const QVariant &value);

private:
	DisplayMode m_displayMode;
	ViewMode m_viewMode;
};

class AddressWidget : public LineEditWidget
{
	Q_OBJECT
	Q_ENUMS(EntryIdentifier)

public:
	enum CompletionMode
	{
		NoCompletionMode = 0,
		InlineCompletionMode,
		PopupCompletionMode
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
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void handleUserInput(const QString &text, WindowsManager::OpenHints hints = WindowsManager::DefaultOpen);
	void setWindow(Window *window = nullptr);
	void setUrl(const QUrl &url, bool force = false);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void hideCompletion();
	void showCompletion(bool isTypedHistory);
	EntryIdentifier getEntry(const QPoint &position) const;
	bool startDrag(QMouseEvent *event);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void openFeed(QAction *action);
	void openUrl(const QString &url);
	void openUrl(const QModelIndex &index);
	void removeEntry();
	void updateGeometries();
	void setCompletion(const QString &filter);
	void setIcon(const QIcon &icon);
	void setTextFromIndex(const QModelIndex &index);

private:
	QPointer<Window> m_window;
	AddressCompletionModel *m_completionModel;
	ItemViewWidget *m_completionView;
	QPoint m_dragStartPosition;
	QVector<EntryIdentifier> m_layout;
	QHash<EntryIdentifier, EntryDefinition> m_entries;
	EntryIdentifier m_clickedEntry;
	EntryIdentifier m_hoveredEntry;
	CompletionModes m_completionModes;
	WindowsManager::OpenHints m_hints;
	bool m_isNavigatingCompletion;
	bool m_isUsingSimpleMode;
	bool m_isTypedHistoryCompletion;

	static int m_entryIdentifierEnumerator;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::AddressWidget::CompletionModes)

#endif
