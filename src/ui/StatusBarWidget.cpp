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

#include "StatusBarWidget.h"
#include "../core/ActionsManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyleOptionSlider>
#include <QtWidgets/QToolButton>

namespace Otter
{

StatusBarWidget::StatusBarWidget(QWidget *parent) : QStatusBar(parent),
	m_zoomSlider(NULL)
{
}

void StatusBarWidget::setup()
{
	m_zoomSlider = new QSlider(this);
	m_zoomSlider->setRange(10, 250);
	m_zoomSlider->setTracking(true);
	m_zoomSlider->setOrientation(Qt::Horizontal);
	m_zoomSlider->setFocusPolicy(Qt::TabFocus);
	m_zoomSlider->setMaximumWidth(100);
	m_zoomSlider->installEventFilter(this);

	QToolButton *zoomOutButton = new QToolButton(this);
	zoomOutButton->setDefaultAction(ActionsManager::getAction(QLatin1String("ZoomOut"), this));
	zoomOutButton->setAutoRaise(true);

	QToolButton *zoomInButton = new QToolButton(this);
	zoomInButton->setDefaultAction(ActionsManager::getAction(QLatin1String("ZoomIn"), this));
	zoomInButton->setAutoRaise(true);

	addPermanentWidget(zoomOutButton);
	addPermanentWidget(m_zoomSlider);
	addPermanentWidget(zoomInButton);
	setZoom(100);

	connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SIGNAL(requestedZoomChange(int)));
}

void StatusBarWidget::setZoom(int zoom)
{
	m_zoomSlider->setValue(zoom);
	m_zoomSlider->setToolTip(tr("Zoom %1%").arg(zoom));
}

void StatusBarWidget::setZoomEnabled(bool enabled)
{
	m_zoomSlider->setEnabled(enabled);
}

bool StatusBarWidget::eventFilter(QObject *object, QEvent *event)
{
	if (m_zoomSlider && object == m_zoomSlider && m_zoomSlider->isEnabled() && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
		QStyleOptionSlider option;
		option.initFrom(m_zoomSlider);
		option.minimum = m_zoomSlider->minimum();
		option.maximum = m_zoomSlider->maximum();
		option.sliderPosition = m_zoomSlider->value();
		option.sliderValue = m_zoomSlider->value();

		const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, m_zoomSlider);

		if (!handle.contains(mouseEvent->pos()))
		{
			const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, m_zoomSlider);
			int value = 0;

			if (m_zoomSlider->orientation() == Qt::Horizontal)
			{
				value = QStyle::sliderValueFromPosition(m_zoomSlider->minimum(), m_zoomSlider->maximum(), (mouseEvent->x() - (handle.width() / 2) - groove.x()), (groove.right() - handle.width()));
			}
			else
			{
				value = QStyle::sliderValueFromPosition(m_zoomSlider->minimum(), m_zoomSlider->maximum(), (mouseEvent->y() - (handle.height() / 2) - groove.y()), (groove.bottom() - handle.height()), true);
			}

			m_zoomSlider->setValue(value);

			return true;
		}
	}

	return QWidget::eventFilter(object, event);
}

}
