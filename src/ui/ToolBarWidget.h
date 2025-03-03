/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "../core/BookmarksModel.h"
#include "../core/GesturesController.h"
#include "../core/SessionsManager.h"
#include "../core/ToolBarsManager.h"

#include <QtCore/QMimeData>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>

namespace Otter
{

class MainWindow;
class SidebarWidget;
class TabBarWidget;
class Window;

class ToolBarWidget : public QToolBar, public GesturesController
{
	Q_OBJECT

public:
	explicit ToolBarWidget(int identifier, Window *window, QWidget *parent);

	static QMenu* createCustomizationMenu(int identifier, const QVector<QAction*> &actions = {}, QWidget *parent = nullptr);
	void setDefinition(const ToolBarsManager::ToolBarDefinition &definition);
	void setState(const Session::MainWindow::ToolBarState &state);
	MainWindow* getMainWindow() const;
	Window* getWindow() const;
	QString getTitle() const;
	ToolBarsManager::ToolBarDefinition getDefinition() const;
	Session::MainWindow::ToolBarState getState() const;
	GestureContext getGestureContext(const QPoint &position = {}) const override;
	QVector<QPointer<QWidget> > getAddressFields() const;
	QVector<QPointer<QWidget> > getSearchFields() const;
	Qt::ToolBarArea getArea() const;
	Qt::ToolButtonStyle getButtonStyle() const;
	int getIdentifier() const;
	int getIconSize() const;
	int getMaximumButtonSize() const;
	bool event(QEvent *event) override;
	static bool calculateShouldBeVisible(const ToolBarsManager::ToolBarDefinition &definition, const Session::MainWindow::ToolBarState &state, ToolBarsManager::ToolBarsMode mode);
	bool canDrop(QDropEvent *event) const;
	bool isCollapsed() const;
	bool isHorizontal() const;
	virtual bool shouldBeVisible(ToolBarsManager::ToolBarsMode mode) const;

public slots:
	void reload();
	void resetGeometry();
	void setArea(Qt::ToolBarArea area);

protected:
	void timerEvent(QTimerEvent *event) override;
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
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
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	virtual void clearEntries();
	virtual void populateEntries();
	void updateDropIndex(const QPoint &position);
	void updateToggleGeometry();
	void setAddressFields(const QVector<QPointer<QWidget> > &addressFields);
	void setSearchFields(const QVector<QPointer<QWidget> > &searchFields);
	bool isDragHandle(const QPoint &position) const;

protected slots:
	void scheduleBookmarksReload();
	void loadBookmarks();
	void toggleVisibility();
	void handleToolBarModified(int identifier);
	void handleToolBarRemoved(int identifier);
	void handleBookmarkModified(BookmarksModel::Bookmark *bookmark);
	void handleBookmarkMoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent);
	void handleBookmarkRemoved(BookmarksModel::Bookmark *bookmark, BookmarksModel::Bookmark *previousParent);
	void handleFullScreenStateChanged(bool isFullScreen);
	void setToolBarLocked(bool locked);

private:
	MainWindow *m_mainWindow;
	Window *m_window;
	SidebarWidget *m_sidebarWidget;
	BookmarksModel::Bookmark *m_bookmark;
	BookmarksModel::Bookmark *m_dropBookmark;
	QPushButton *m_toggleButton;
	QPoint m_dragStartPosition;
	QVector<QPointer<QWidget> > m_addressFields;
	QVector<QPointer<QWidget> > m_searchFields;
	Session::MainWindow::ToolBarState m_state;
	Qt::ToolBarArea m_area;
	int m_reloadTimer;
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

class TabBarToolBarWidget final : public ToolBarWidget
{
	Q_OBJECT

public:
	explicit TabBarToolBarWidget(int identifier, Window *window, QWidget *parent);

	bool shouldBeVisible(ToolBarsManager::ToolBarsMode mode) const override;
	bool event(QEvent *event) override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void findTabBar();
	void clearEntries() override;
	void populateEntries() override;

protected slots:
	void updateVisibility();

private:
	TabBarWidget *m_tabBar;
};

}

#endif
