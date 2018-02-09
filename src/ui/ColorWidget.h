/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_COLORWIDGET_H
#define OTTER_COLORWIDGET_H

#include <QtWidgets/QWidget>

namespace Otter
{

class LineEditWidget;

class ColorWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit ColorWidget(QWidget *parent = nullptr);

	void setColor(const QString &color);
	void setColor(const QColor &color);
	static void drawThumbnail(QPainter *painter, const QColor &color, const QPalette &palette, const QRect &rectangle);
	QColor getColor() const;

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
	void selectColor();
	void updateColor(const QString &text);

private:
	LineEditWidget *m_lineEditWidget;
	QColor m_color;
	QRect m_buttonRectangle;

signals:
	void colorChanged(const QColor &color);
};

}

#endif
