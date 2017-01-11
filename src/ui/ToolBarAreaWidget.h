/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TOOLBARAREAWIDGET_H
#define OTTER_TOOLBARAREAWIDGET_H

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QWidget>

namespace Otter
{

class MainWindow;
class ToolBarWidget;

class ToolBarAreaWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ToolBarAreaWidget(Qt::ToolBarArea area, MainWindow *parent);

	Qt::ToolBarArea getArea() const;

protected:
	void paintEvent(QPaintEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void startToolBarDragging();
	void endToolBarDragging();
	void insertToolBar(ToolBarWidget *toolBar);
	void updateDropRow(const QPoint &position);
	void updateToolBarsOrder();

protected slots:
	void activateToolBars(Qt::ToolBarAreas areas);
	void toolBarAdded(int identifier);
	void toolBarModified(int identifier);
	void setControlsHidden(bool areHidden);

private:
	MainWindow *m_mainWindow;
	QBoxLayout *m_layout;
	Qt::ToolBarArea m_area;
	int m_dropRow;

friend class MainWindow;
friend class ToolBarWidget;
};

}

#endif
