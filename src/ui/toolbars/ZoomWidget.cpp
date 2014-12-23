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

#include "ZoomWidget.h"
#include "../MainWindow.h"
#include "../../core/WindowsManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyleOptionSlider>

namespace Otter
{

ZoomWidget::ZoomWidget(QWidget *parent) : QSlider(parent),
	m_manager(NULL)
{
	MainWindow *window = MainWindow::findMainWindow(parent);

	if (window)
	{
		m_manager = window->getWindowsManager();

		connect(m_manager, SIGNAL(canZoomChanged(bool)), this, SLOT(setEnabled(bool)));
		connect(m_manager, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)));
		connect(this, SIGNAL(valueChanged(int)), m_manager, SLOT(setZoom(int)));
	}
	else
	{
		setEnabled(false);
	}

	setRange(10, 250);
	setTracking(true);
	setOrientation(Qt::Horizontal);
	setFocusPolicy(Qt::TabFocus);
	setMaximumWidth(100);
	setZoom(100);
}

void ZoomWidget::setZoom(int zoom)
{
	setValue(zoom);
	setToolTip(tr("Zoom %1%").arg(zoom));
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
	if (!isEnabled())
	{
		return;
	}

	QStyleOptionSlider option;
	option.initFrom(this);
	option.minimum = minimum();
	option.maximum = maximum();
	option.sliderPosition = value();
	option.sliderValue = value();

	const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);

	if (handle.contains(event->pos()))
	{
		QSlider::mousePressEvent(event);

		return;
	}

	const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
	int value = 0;

	if (orientation() == Qt::Horizontal)
	{
		value = QStyle::sliderValueFromPosition(minimum(), maximum(), (event->x() - (handle.width() / 2) - groove.x()), (groove.right() - handle.width()));
	}
	else
	{
		value = QStyle::sliderValueFromPosition(minimum(), maximum(), (event->y() - (handle.height() / 2) - groove.y()), (groove.bottom() - handle.height()), true);
	}

	setValue(value);
}

}
