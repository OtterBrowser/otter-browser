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

#ifndef OTTER_TABBARDOCKWIDGET_H
#define OTTER_TABBARDOCKWIDGET_H

#include <QtGui/QPaintEvent>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QToolButton>

namespace Otter
{

class TabBarWidget;

class TabBarDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	explicit TabBarDockWidget(QWidget *parent = NULL);

	void setup(QMenu *closedWindowsMenu);
	TabBarWidget* getTabBar();

public slots:
	void setClosedWindowsMenuEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event);

protected slots:
	void moveNewTabButton(int position);

private:
	TabBarWidget *m_tabBar;
	QToolButton *m_newTabButton;
	QToolButton *m_trashButton;

};

}

#endif
