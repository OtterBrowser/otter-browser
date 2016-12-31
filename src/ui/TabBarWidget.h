/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_TABBARWIDGET_H
#define OTTER_TABBARWIDGET_H

#include "../core/WindowsManager.h"

#include <QtGui/QDrag>
#include <QtGui/QMovie>
#include <QtWidgets/QProxyStyle>
#include <QtWidgets/QTabBar>

namespace Otter
{

class PreviewWidget;
class TabBarWidget;
class Window;

class TabDrag : public QDrag
{
public:
	explicit TabDrag(quint64 window, QObject *parent);
	~TabDrag();

	void timerEvent(QTimerEvent *event) override;

private:
	quint64 m_window;
	int m_releaseTimer;
};

class TabHandleWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TabHandleWidget(Window *window, TabBarWidget *parent);

	Window* getWindow() const;

protected:
	void paintEvent(QPaintEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
	void markAsActive();
	void markAsNeedingAttention();
	void handleLoadingStateChanged(WindowsManager::LoadingState state);
	void updateGeometries();

private:
	Window *m_window;
	TabBarWidget *m_tabBarWidget;
	QMovie *m_loadingMovie;
	QRect m_closeButtonRectangle;
	QRect m_urlIconRectangle;
	QRect m_thumbnailRectangle;
	QRect m_titleRectangle;
	bool m_isCloseButtonUnderMouse;
	bool m_wasCloseButtonPressed;

	static QMovie *m_delayedLoadingMovie;
	static QMovie *m_ongoingLoadingMovie;
	static QIcon m_lockedIcon;
};

class TabBarWidget : public QTabBar
{
	Q_OBJECT

public:
	explicit TabBarWidget(QWidget *parent = nullptr);

	void addTab(int index, Window *window);
	void removeTab(int index);
	void activateTabOnLeft();
	void activateTabOnRight();
	void showPreview(int index, int delay = 0);
	void hidePreview();
	Window* getWindow(int index) const;
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	int getPinnedTabsAmount() const;
	static bool areThumbnailsEnabled();
	static bool isLayoutReversed();
	static bool isCloseButtonEnabled();
	static bool isUrlIconEnabled();

public slots:
	void updateSize();

protected:
	void changeEvent(QEvent *event) override;
	void childEvent(QChildEvent *event) override;
	void timerEvent(QTimerEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void enterEvent(QEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void tabLayoutChange() override;
	void tabInserted(int index) override;
	void tabRemoved(int index) override;
	void tabHovered(int index);
	void updateStyle();
	QStyleOptionTab createStyleOptionTab(int index) const;
	QSize tabSizeHint(int index) const override;
	int getDropIndex() const;
	bool event(QEvent *event) override;

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void updatePreviewPosition();
	void updatePinnedTabsAmount(Window *modifiedWindow = nullptr);
	void setArea(Qt::ToolBarArea area);

private:
	PreviewWidget *m_previewWidget;
	QWidget *m_movableTabWidget;
	QPoint m_dragMovePosition;
	QPoint m_dragStartPosition;
	QSize m_maximumTabSize;
	QSize m_minimumTabSize;
	quint64 m_draggedWindow;
	int m_tabWidth;
	int m_clickedTab;
	int m_hoveredTab;
	int m_pinnedTabsAmount;
	int m_previewTimer;
	bool m_arePreviewsEnabled;
	bool m_isDraggingTab;
	bool m_isDetachingTab;
	bool m_isIgnoringTabDrag;
	bool m_needsUpdateOnLeave;

	static bool m_areThumbnailsEnabled;
	static bool m_isLayoutReversed;
	static bool m_isCloseButtonEnabled;
	static bool m_isUrlIconEnabled;

signals:
	void needsGeometriesUpdate();
	void tabsAmountChanged(int amount);
};

}

#endif
