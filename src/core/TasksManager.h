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

#ifndef OTTER_TASKSMANAGER_H
#define OTTER_TASKSMANAGER_H

#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QQueue>

#include <functional>

namespace Otter
{

class TasksManager final : public QObject
{
	Q_OBJECT

public:
	struct Task final
	{
		QPointer<QObject> object = nullptr;
		std::function<void()> function = nullptr;
		QDateTime nextRun;
		quint64 identifier = 0;
		uint interval = 0;
		bool isRepeating = true;
	};

	static void createInstance();
	static void updateTask(quint64 identifier, int interval, bool isRepeating);
	static void removeTask(quint64 identifier);
	static TasksManager* getInstance();
	static quint64 registerTask(uint interval, bool isRepeating, const std::function<void()> &function, QObject *object = nullptr);

protected:
	explicit TasksManager(QObject *parent = nullptr);

	void timerEvent(QTimerEvent *event) override;
	static void updateQueue();

private:
	int m_tasksTimer;

	static TasksManager *m_instance;
	static QMap<quint64, Task> m_tasks;
	static QQueue<quint64> m_queue;

signals:
	void timeout(quint64 identifier);
};

}

#endif
