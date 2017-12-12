/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "LongTermTimer.h"

#include <QtCore/QTimerEvent>

#include <limits>

namespace Otter
{

LongTermTimer::LongTermTimer(QObject *parent) : QObject(parent),
	m_interval(0),
	m_remainingTime(0),
	m_timer(0)
{
}

void LongTermTimer::start(quint64 interval)
{
	m_interval = interval;

	updateTimer(interval);
}

void LongTermTimer::stop()
{
	if (m_timer > 0)
	{
		killTimer(m_timer);

		m_timer = 0;
		m_interval = 0;
		m_remainingTime = 0;
	}
}

void LongTermTimer::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_timer)
	{
		if (m_remainingTime == 0)
		{
			updateTimer(m_interval);

			emit timeout();
		}
		else
		{
			updateTimer(m_remainingTime, true);
		}
	}
}

void LongTermTimer::updateTimer(const quint64 remainingTime, const bool updateCounter)
{
	quint64 timerValue(std::numeric_limits<int>::max());

	if (remainingTime <= timerValue)
	{
		timerValue = remainingTime;

		if (updateCounter)
		{
			m_remainingTime = 0;
		}
	}
	else if (updateCounter)
	{
		m_remainingTime -= timerValue;
	}
	else
	{
		m_remainingTime = (remainingTime - timerValue);
	}

	m_timer = startTimer(static_cast<int>(timerValue));
}

}
