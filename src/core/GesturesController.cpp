/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "GesturesController.h"

namespace Otter
{

GesturesController::GesturesController() = default;

GesturesController::~GesturesController() = default;

void GesturesController::installGesturesFilter(QObject *object, GesturesController *target, const QVector<GesturesManager::GesturesContext> &gesturesContexts, bool canPropagateEvents)
{
	GesturesFilter *filter(new GesturesFilter(object, target));
	filter->setPropagateEvents(canPropagateEvents);

	m_gesturesContexts = gesturesContexts;
}

GesturesController::GestureContext GesturesController::getGestureContext(const QPoint &position)
{
	Q_UNUSED(position)

	return {{}, m_gesturesContexts};
}

GesturesFilter::GesturesFilter(QObject *object, GesturesController *controller) : QObject(object),
	m_controller(controller),
	m_canPropagateEvents(true)
{
	object->installEventFilter(this);
}

void GesturesFilter::setPropagateEvents(bool canPropagateEvents)
{
	m_canPropagateEvents = canPropagateEvents;
}

bool GesturesFilter::eventFilter(QObject *object, QEvent *event)
{
	if (GesturesManager::isTracking())
	{
		return QObject::eventFilter(object, event);
	}

	QPoint position;

	switch (event->type())
	{
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonPress:
			position = static_cast<QMouseEvent*>(event)->pos();

			break;
		case QEvent::Wheel:
			position = static_cast<QWheelEvent*>(event)->pos();

			break;
		default:
			break;
	}

	if (!position.isNull())
	{
		const GesturesController::GestureContext context(m_controller->getGestureContext(position));

		if (!context.contexts.isEmpty())
		{
			GesturesManager::startGesture(parent(), event, context.contexts, context.parameters);

			if (!m_canPropagateEvents)
			{
				return true;
			}
		}
	}

	return QObject::eventFilter(object, event);
}

}
