/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TABBARTOOLBARWIDGET_H
#define OTTER_TABBARTOOLBARWIDGET_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>

namespace Otter
{

class ActionWidget;
class TabBarWidget;

class TabBarToolBarWidget : public QToolBar
{
	Q_OBJECT

public:
	explicit TabBarToolBarWidget(QMainWindow *parent = NULL);

	TabBarWidget* getTabBar();

public slots:
	void updateNewTabPosition();
	void updateOrientation();

private:
	QMainWindow *m_window;
	QWidget *m_widget;
	TabBarWidget *m_tabBar;
	ActionWidget *m_newTabButton;
};

}

#endif
