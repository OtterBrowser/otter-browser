/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TasksManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

TasksManager* TasksManager::m_instance = nullptr;
QMap<quint64, TasksManager::Task> TasksManager::m_tasks;
QQueue<quint64> TasksManager::m_queue;

TasksManager::TasksManager(QObject *parent) : QObject(parent),
	m_tasksTimer(startTimer(60000))
{
}

void TasksManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new TasksManager(QCoreApplication::instance());
	}
}

void TasksManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_tasksTimer)
	{
		const QDateTime currentDateTime(QDateTime::currentDateTimeUtc());

		if (m_queue.isEmpty() || !m_tasks.contains(m_queue[0]) || m_tasks[m_queue[0]].nextRun > currentDateTime)
		{
			return;
		}

		while (!m_queue.isEmpty())
		{
			if (!m_tasks.contains(m_queue[0]))
			{
				m_queue.dequeue();

				continue;
			}

			if (m_tasks[m_queue[0]].nextRun > currentDateTime)
			{
				break;
			}

			const quint64 identifier(m_queue.dequeue());
			const Task definition(m_tasks[identifier]);

			if (definition.function && definition.object)
			{
				definition.function();
			}
			else
			{
				emit timeout(identifier);
			}
		}

		updateQueue();
	}
}

void TasksManager::updateQueue()
{
///TODO
}

void TasksManager::updateTask(quint64 identifier, int interval, bool isRepeating)
{
	if (!m_tasks.contains(identifier))
	{
		return;
	}

	m_tasks[identifier].interval = interval;
	m_tasks[identifier].isRepeating = isRepeating;

	updateQueue();
}

void TasksManager::removeTask(quint64 identifier)
{
	if (!m_tasks.contains(identifier))
	{
		return;
	}

	m_tasks.remove(identifier);

	updateQueue();
}

TasksManager* TasksManager::getInstance()
{
	return m_instance;
}

quint64 TasksManager::registerTask(uint interval, bool isRepeating, const std::function<void()> &function, QObject *object)
{
	const quint64 identifier(m_tasks.isEmpty() ? 1 : (m_tasks.lastKey() + 1));
	Task definition;
	definition.object = object;
	definition.function = function;
	definition.identifier = identifier;
	definition.interval = interval;
	definition.isRepeating = isRepeating;

	m_tasks[identifier] = definition;

	updateQueue();

	return identifier;
}

}
