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

#include <QtWidgets/QToolBar>

#include "../core/ActionsManager.h"

namespace Otter
{

class MainWindow;

class ToolBarWidget : public QToolBar
{
	Q_OBJECT

public:
	explicit ToolBarWidget(const ToolBarDefinition &definition, Window *window, QWidget *parent);

	int getMaximumButtonSize() const;

public slots:
	void notifyAreaChanged();

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void setup();

protected slots:
	void toolBarModified(const QString &identifier);
	void toolBarRemoved(const QString &identifier);
	void notifyWindowChanged(qint64 identifier);
	void updateBookmarks();

private:
	MainWindow *m_mainWindow;
	Window *m_window;
	QString m_identifier;

signals:
	void areaChanged(Qt::ToolBarArea area);
	void windowChanged(Window *window);
	void maximumButtonSizeChanged(int size);
};

}

#endif
