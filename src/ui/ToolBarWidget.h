/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TOOLBARWIDGET_H
#define OTTER_TOOLBARWIDGET_H

#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>

#include "../core/ToolBarsManager.h"

namespace Otter
{

class MainWindow;
class ToolBarWidget;
class Window;

class ToolBarDragAreaWidget : public QWidget
{
public:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

protected:
	explicit ToolBarDragAreaWidget(ToolBarWidget *parent);

private:
	ToolBarWidget *m_toolBar;
	QPoint m_dragStartPosition;

friend class ToolBarWidget;
};

class ToolBarWidget : public QToolBar
{
	Q_OBJECT

public:
	explicit ToolBarWidget(int identifier, Window *window, QWidget *parent);

	static QMenu* createCustomizationMenu(int identifier, QList<QAction*> actions = QList<QAction*>(), QWidget *parent = NULL);
	void setDefinition(const ToolBarDefinition &definition);
	Qt::ToolBarArea getArea() const;
	int getIdentifier() const;
	int getMaximumButtonSize() const;

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void startToolBarDragging();
	void endToolBarDragging();
	void setup();
	QWidget* createWidget(const ToolBarActionDefinition &definition);

protected slots:
	void toolBarModified(int identifier);
	void toolBarRemoved(int identifier);
	void notifyWindowChanged(qint64 identifier);
	void updateBookmarks();
	void updateVisibility();
	void setToolBarLocked(bool locked);

private:
	MainWindow *m_mainWindow;
	Window *m_window;
	ToolBarDragAreaWidget *m_dragArea;
	int m_identifier;

signals:
	void areaChanged(Qt::ToolBarArea area);
	void windowChanged(Window *window);
	void maximumButtonSizeChanged(int size);

friend class ToolBarDragAreaWidget;
};

}

#endif
