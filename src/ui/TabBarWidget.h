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

#include <QtGui/QDrag>
#include <QtWidgets/QProxyStyle>
#include <QtWidgets/QTabBar>

namespace Otter
{

class PreviewWidget;
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

class TabBarStyle : public QProxyStyle
{
public:
	explicit TabBarStyle(QStyle *style = nullptr);

	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
	QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;
	QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override;
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
	Window* getWindow(int index) const;
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	int getPinnedTabsAmount() const;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void timerEvent(QTimerEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void enterEvent(QEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void tabLayoutChange() override;
	void tabInserted(int index) override;
	void tabRemoved(int index) override;
	void tabHovered(int index);
	void showPreview(int index);
	void hidePreview();
	QSize tabSizeHint(int index) const override;
	int getDropIndex() const;
	bool event(QEvent *event) override;

protected slots:
	void optionChanged(int identifier, const QVariant &value);
	void currentTabChanged(int index);
	void updatePinnedTabsAmount(Window *modifiedWindow = nullptr);
	void updateButtons();
	void updateTabs(int index = -1);
	void setCycle(bool enable);
	void setArea(Qt::ToolBarArea area);
	void setShape(QTabBar::Shape shape);

private:
	PreviewWidget *m_previewWidget;
	QPoint m_dragMovePosition;
	QPoint m_dragStartPosition;
	quint64 m_draggedWindow;
	QTabBar::ButtonPosition m_closeButtonPosition;
	QTabBar::ButtonPosition m_iconButtonPosition;
	int m_tabSize;
	int m_maximumTabSize;
	int m_minimumTabSize;
	int m_pinnedTabsAmount;
	int m_clickedTab;
	int m_hoveredTab;
	int m_previewTimer;
	bool m_showCloseButton;
	bool m_showUrlIcon;
	bool m_enablePreviews;
	bool m_isDraggingTab;
	bool m_isDetachingTab;
	bool m_isIgnoringTabDrag;

signals:
	void layoutChanged();
	void tabsAmountChanged(int amount);
};

}

#endif
