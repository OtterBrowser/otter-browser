/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_LONGTERMTIMER_H
#define OTTER_LONGTERMTIMER_H

#include <QtCore/QObject>

namespace Otter
{

class LongTermTimer : public QObject
{
	Q_OBJECT

public:
	explicit LongTermTimer(quint64 seconds, QObject *receiver, const char *member);

	static void runTimer(quint64 seconds, QObject *receiver, const char *member);

protected:
	void timerEvent(QTimerEvent *event);
	void updateTimer(const quint64 secondsLeft, const bool updateCounter = false);

private:
	quint64 m_secondsLeft;
	int m_timer;

signals:
	void timeout();
};

}

#endif
