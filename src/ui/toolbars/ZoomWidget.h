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

#ifndef OTTER_ZOOMWIDGET_H
#define OTTER_ZOOMWIDGET_H

#include <QtWidgets/QSlider>

namespace Otter
{

class MainWindow;

class ZoomWidget : public QSlider
{
	Q_OBJECT

public:
	explicit ZoomWidget(QWidget *parent = NULL);

public slots:
	void setZoom(int zoom);

protected:
	void mousePressEvent(QMouseEvent *event);

private:
	MainWindow *m_mainWindow;
};

}

#endif
