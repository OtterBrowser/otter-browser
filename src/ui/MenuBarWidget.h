/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_MENUBARWIDGET_H
#define OTTER_MENUBARWIDGET_H

#include <QtWidgets/QMenuBar>

namespace Otter
{

class MainWindow;
class ToolBarWidget;

class MenuBarWidget final : public QMenuBar
{
	Q_OBJECT

public:
	explicit MenuBarWidget(MainWindow *parent);

protected:
	void changeEvent(QEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void reload();

protected slots:
	void updateGeometries();

private:
	MainWindow *m_mainWindow;
	ToolBarWidget *m_leftToolBar;
	ToolBarWidget *m_rightToolBar;
};

}

#endif
