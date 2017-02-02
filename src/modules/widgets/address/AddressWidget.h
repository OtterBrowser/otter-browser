/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../../../ui/ComboBoxWidget.h"

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

class AddressWidget : public ComboBoxWidget
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

	void showPopup();
	void hidePopup();
	QString getText() const;
	QUrl getUrl() const;
	bool event(QEvent *event);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void activate(Qt::FocusReason reason);
	void handleUserInput(const QString &text, WindowsManager::OpenHints hints = WindowsManager::DefaultOpen);
	void setWindow(Window *window = nullptr);
	void setText(const QString &text);
	void setUrl(const QUrl &url, bool force = false);

protected:
	void changeEvent(QEvent *event);
	void timerEvent(QTimerEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void focusInEvent(QFocusEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void hideCompletion();
	EntryIdentifier getEntry(const QPoint &position) const;
	bool startDrag(QMouseEvent *event);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void openFeed(QAction *action);
	void openUrl(const QString &url);
	void openUrl(const QModelIndex &index);
	void removeEntry();
	void updateGeometries();
	void updateLineEdit();
	void setCompletion(const QString &filter);
	void setIcon(const QIcon &icon);
	void setText(const QModelIndex &index);

private:
	QPointer<Window> m_window;
	LineEditWidget *m_lineEdit;
	AddressCompletionModel *m_completionModel;
	ItemViewWidget *m_completionView;
	QAbstractItemView *m_visibleView;
	QTime m_popupHideTime;
	QPoint m_dragStartPosition;
	QRect m_lineEditRectangle;
	QVector<EntryIdentifier> m_layout;
	QHash<EntryIdentifier, EntryDefinition> m_entries;
	EntryIdentifier m_clickedEntry;
	EntryIdentifier m_hoveredEntry;
	CompletionModes m_completionModes;
	WindowsManager::OpenHints m_hints;
	int m_removeModelTimer;
	bool m_isNavigatingCompletion;
	bool m_isUsingSimpleMode;
	bool m_wasPopupVisible;

	static int m_entryIdentifierEnumerator;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::AddressWidget::CompletionModes)

#endif
