/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_TOOLBARWIDGET_H
#define OTTER_TOOLBARWIDGET_H

#include <QtCore/QMimeData>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>

#include "../core/ToolBarsManager.h"

namespace Otter
{

class BookmarksItem;
class MainWindow;
class SidebarWidget;
class Window;

class ToolBarDropZoneWidget final : public QToolBar
{
	Q_OBJECT

public:
	explicit ToolBarDropZoneWidget(MainWindow *parent);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	bool canDrop(const QMimeData *mimeData);

private:
	MainWindow *m_mainWindow;
	bool m_isDropTarget;

signals:
	void toolBarDraggedChanged(bool isDragging);
};

class ToolBarWidget : public QToolBar
{
	Q_OBJECT

public:
	explicit ToolBarWidget(int identifier, Window *window, QWidget *parent);

	static QMenu* createCustomizationMenu(int identifier, QVector<QAction*> actions = {}, QWidget *parent = nullptr);
	void reload();
	void setDefinition(const ToolBarsManager::ToolBarDefinition &definition);
	QString getTitle() const;
	ToolBarsManager::ToolBarDefinition getDefinition() const;
	Qt::ToolBarArea getArea();
	Qt::ToolButtonStyle getButtonStyle() const;
	int getIdentifier() const;
	int getIconSize() const;
	int getMaximumButtonSize() const;
	bool canDrop(QDropEvent *event) const;
	bool shouldBeVisible(bool isFullScreen) const;
	bool event(QEvent *event) override;

public slots:
	void resetGeometry();

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void enterEvent(QEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void updateDropIndex(const QPoint &position);
	void updateToggleGeometry();

protected slots:
	void loadBookmarks();
	void toggleVisibility();
	void notifyWindowChanged(quint64 identifier);
	void handleToolBarModified(int identifier);
	void handleToolBarRemoved(int identifier);
	void handleBookmarkModified(BookmarksItem *bookmark);
	void handleBookmarkMoved(BookmarksItem *bookmark, BookmarksItem *previousParent);
	void handleBookmarkRemoved(BookmarksItem *bookmark, BookmarksItem *previousParent);
	void updateVisibility();
	void setToolBarLocked(bool locked);

private:
	MainWindow *m_mainWindow;
	Window *m_window;
	SidebarWidget *m_sidebarWidget;
	BookmarksItem *m_bookmark;
	BookmarksItem *m_dropBookmark;
	QPushButton *m_toggleButton;
	QPoint m_dragStartPosition;
	int m_identifier;
	int m_dropIndex;
	bool m_isCollapsed;
	bool m_isInitialized;

signals:
	void windowChanged(Window *window);
	void areaChanged(Qt::ToolBarArea area);
	void buttonStyleChanged(Qt::ToolButtonStyle buttonStyle);
	void iconSizeChanged(int size);
	void maximumButtonSizeChanged(int size);
	void toolBarModified();
};

}

#endif
