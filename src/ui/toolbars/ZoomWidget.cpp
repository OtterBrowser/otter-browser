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

#include "ZoomWidget.h"
#include "../MainWindow.h"
#include "../../core/WindowsManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionSlider>

namespace Otter
{

ZoomWidget::ZoomWidget(QWidget *parent) : QSlider(parent),
	m_mainWindow(NULL)
{
	m_mainWindow = MainWindow::findMainWindow(parent);

	if (m_mainWindow)
	{
		connect(m_mainWindow->getWindowsManager(), SIGNAL(canZoomChanged(bool)), this, SLOT(setEnabled(bool)));
		connect(m_mainWindow->getWindowsManager(), SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)));
		connect(this, SIGNAL(valueChanged(int)), m_mainWindow->getWindowsManager(), SLOT(setZoom(int)));
	}
	else
	{
		setEnabled(false);
	}

	setRange(0, 300);
	setTracking(true);
	setOrientation(Qt::Horizontal);
	setFocusPolicy(Qt::TabFocus);
	setTickPosition(QSlider::TicksBelow);
	setTickInterval(100);
	setMaximumWidth(150);
	setZoom(100);
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
	if (!isEnabled() || event->button() != Qt::LeftButton)
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

	setZoom(value);

	if (handle.contains(event->pos()))
	{
		QSlider::mousePressEvent(event);
	}
}

void ZoomWidget::setZoom(int zoom)
{
	zoom = qMax(zoom, 10);

	setValue(zoom);
	setToolTip(tr("Zoom %1%").arg(zoom));
	setStatusTip(tr("Zoom %1%").arg(zoom));

	if (m_mainWindow && (underMouse() || isSliderDown()))
	{
		QStatusTipEvent event(statusTip());

		QApplication::sendEvent(m_mainWindow, &event);
	}
}

}
