/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WORKSPACEWIDGET_H
#define OTTER_WORKSPACEWIDGET_H

#include <QtCore/QPointer>
#include <QtWidgets/QMdiArea>

namespace Otter
{

class Window;

class WorkspaceWidget : public QWidget
{
	Q_OBJECT

public:
	explicit WorkspaceWidget(QWidget *parent = NULL);

	void addWindow(Window *window);
	void setActiveWindow(Window *window);
	Window* getActiveWindow();

protected:
	void resizeEvent(QResizeEvent *event);

private:
	QMdiArea *m_mdiArea;
	QPointer<Window> m_activeWindow;
};

}

#endif
