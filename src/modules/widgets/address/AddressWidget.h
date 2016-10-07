/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_ADDRESSWIDGET_H
#define OTTER_ADDRESSWIDGET_H

#include "../../../core/WindowsManager.h"
#include "../../../ui/ComboBoxWidget.h"

#include <QtCore/QTime>
#include <QtCore/QUrl>
#include <QtWidgets/QLabel>

namespace Otter
{

class AddressCompletionModel;
class ItemViewWidget;
class LineEditWidget;
class Window;

class AddressWidget : public ComboBoxWidget
{
	Q_OBJECT

public:
	enum CompletionMode
	{
		NoCompletionMode = 0,
		InlineCompletionMode = 1,
		PopupCompletionMode = 2
	};

	Q_DECLARE_FLAGS(CompletionModes, CompletionMode)

	explicit AddressWidget(Window *window, QWidget *parent = NULL);

	void showPopup();
	void hidePopup();
	QString getText() const;
	QUrl getUrl() const;
	bool event(QEvent *event);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void activate(Qt::FocusReason reason);
	void handleUserInput(const QString &text, WindowsManager::OpenHints hints = WindowsManager::DefaultOpen);
	void setText(const QString &text);
	void setUrl(const QUrl &url, bool force = false);
	void setWindow(Window *window = NULL);

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
	bool startDrag(QMouseEvent *event);

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void openFeed(QAction *action);
	void openUrl(const QString &url);
	void openUrl(const QModelIndex &index);
	void removeIcon();
	void updateBookmark(const QUrl &url = QUrl());
	void updateFeeds();
	void updateLoadPlugins();
	void updateLineEdit();
	void updateIcons();
	void setCompletion(const QString &filter);
	void setIcon(const QIcon &icon);
	void setText(const QModelIndex &index);

private:
	QPointer<Window> m_window;
	LineEditWidget *m_lineEdit;
	AddressCompletionModel *m_completionModel;
	ItemViewWidget *m_completionView;
	QLabel *m_bookmarkLabel;
	QLabel *m_feedsLabel;
	QLabel *m_loadPluginsLabel;
	QLabel *m_urlIconLabel;
	QTime m_popupHideTime;
	QPoint m_dragStartPosition;
	QRect m_lineEditRectangle;
	QRect m_historyDropdownArrowRectangle;
	QRect m_securityBadgeRectangle;
	AddressWidget::CompletionModes m_completionModes;
	WindowsManager::OpenHints m_hints;
	int m_removeModelTimer;
	bool m_isHistoryDropdownEnabled;
	bool m_isNavigatingCompletion;
	bool m_isUsingSimpleMode;
	bool m_wasPopupVisible;

signals:
	void requestedOpenBookmark(BookmarksItem *bookmark, WindowsManager::OpenHints hints);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::AddressWidget::CompletionModes)

#endif
