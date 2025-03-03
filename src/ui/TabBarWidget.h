/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABBARWIDGET_H
#define OTTER_TABBARWIDGET_H

#include "WebWidget.h"
#include "../core/GesturesController.h"

#include <QtWidgets/QTabBar>

namespace Otter
{

class Animation;
class PreviewWidget;
class TabBarWidget;
class Window;

class TabHandleWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit TabHandleWidget(Window *window, TabBarWidget *parent);

	void setIsActiveWindow(bool isActive);
	Window* getWindow() const;

protected:
	void timerEvent(QTimerEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;

protected slots:
	void handleLoadingStateChanged(WebWidget::LoadingState state);
	void updateGeometries();
	void updateTitle();

private:
	Window *m_window;
	TabBarWidget *m_tabBarWidget;
	QString m_title;
	QRect m_closeButtonRectangle;
	QRect m_urlIconRectangle;
	QRect m_thumbnailRectangle;
	QRect m_labelRectangle;
	QRect m_titleRectangle;
	int m_dragTimer;
	bool m_isActiveWindow;
	bool m_isCloseButtonUnderMouse;
	bool m_wasCloseButtonPressed;

	static Animation *m_spinnerAnimation;
	static QIcon m_lockedIcon;
};

class TabBarWidget final : public QTabBar, public GesturesController
{
	Q_OBJECT

public:
	explicit TabBarWidget(QWidget *parent = nullptr);

	void addTab(int index, Window *window);
	void removeTab(int index);
	void showPreview(int index, int delay = 0);
	void hidePreview();
	Window* getWindow(int index) const;
	GestureContext getGestureContext(const QPoint &position = {}) const override;
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	static bool areThumbnailsEnabled();
	static bool isLayoutReversed();
	static bool isCloseButtonEnabled();
	static bool isUrlIconEnabled();
	bool isHorizontal() const;

public slots:
	void updateSize();

protected:
	void changeEvent(QEvent *event) override;
	void childEvent(QChildEvent *event) override;
	void timerEvent(QTimerEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
#if QT_VERSION >= 0x060000
	void enterEvent(QEnterEvent *event) override;
#else
	void enterEvent(QEvent *event) override;
#endif
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
	QStyleOptionTab createStyleOptionTab(int index) const;
	QSize tabSizeHint(int index) const override;
	int getDropIndex() const;
	bool event(QEvent *event) override;

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleCurrentChanged(int index);
	void updatePinnedTabsAmount();
	void updateStyle();
	void setArea(Qt::ToolBarArea area);

private:
	PreviewWidget *m_previewWidget;
	QPointer<TabHandleWidget> m_activeTabHandleWidget;
	QPointer<QWidget> m_movableTabWidget;
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
