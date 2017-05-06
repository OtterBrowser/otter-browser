/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ZoomWidget.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOptionSlider>

namespace Otter
{

ZoomWidget::ZoomWidget(Window *window, QWidget *parent) : QSlider(parent),
	m_window(nullptr)
{
	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	setRange(0, 300);
	setTracking(true);
	setOrientation(Qt::Horizontal);
	setFocusPolicy(Qt::TabFocus);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setTickPosition(QSlider::TicksBelow);
	setTickInterval(100);
	setWindow(window);

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::AddressBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
	if (!isEnabled() || event->button() != Qt::LeftButton)
	{
		QSlider::mousePressEvent(event);

		return;
	}

	QStyleOptionSlider option;
	option.initFrom(this);
	option.minimum = minimum();
	option.maximum = maximum();
	option.sliderPosition = value();
	option.sliderValue = value();

	const QRect handle(style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this));
	const QRect groove(style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this));
	int value(0);

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

void ZoomWidget::setWindow(Window *window)
{
	if (m_window)
	{
		disconnect(m_window, SIGNAL(canZoomChanged(bool)), this, SLOT(setEnabled(bool)));
		disconnect(m_window, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)));
		disconnect(this, SIGNAL(valueChanged(int)), m_window, SLOT(setZoom(int)));
	}

	m_window = window;

	setEnabled(m_window && m_window->canZoom());

	if (window)
	{
		setZoom(window->getZoom());

		connect(window, SIGNAL(canZoomChanged(bool)), this, SLOT(setEnabled(bool)));
		connect(window, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)));
		connect(this, SIGNAL(valueChanged(int)), window, SLOT(setZoom(int)));
	}
	else
	{
		setZoom(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	}
}

void ZoomWidget::setZoom(int zoom)
{
	zoom = qMax(zoom, 10);

	setValue(zoom);
	setToolTip(tr("Zoom %1%").arg(zoom));
	setStatusTip(tr("Zoom %1%").arg(zoom));

	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (mainWindow && (underMouse() || isSliderDown()))
	{
		QStatusTipEvent event(statusTip());

		QApplication::sendEvent(mainWindow, &event);
	}
}

QSize ZoomWidget::sizeHint() const
{
	return QSize(150, QSlider::sizeHint().height());
}

}
