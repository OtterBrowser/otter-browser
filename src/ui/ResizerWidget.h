/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_RESIZERWIDGET_H
#define OTTER_RESIZERWIDGET_H

#include <QtWidgets/QWidget>

namespace Otter
{

class ResizerWidget final : public QWidget
{
	Q_OBJECT

public:
	enum ResizeDirection
	{
		LeftToRightDirection = 0,
		RightToLeftDirection,
		TopToBottomDirection,
		BottomToTopDirection
	};

	explicit ResizerWidget(QWidget *widget, QWidget *parent = nullptr);

	void setDirection(ResizeDirection direction);

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

private:
	QWidget *m_widget;
	QPoint m_dragStartPosition;
	ResizeDirection m_direction;
	bool m_isMoving;

signals:
	void finished();
	void resized(int size);
};

}

#endif
