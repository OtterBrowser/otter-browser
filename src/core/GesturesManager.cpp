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

#include "GesturesManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "../ui/MainWindow.h"
#include "../../3rdparty/mousegestures/QjtMouseGestureFilter.h"
#include "../../3rdparty/mousegestures/QjtMouseGesture.h"

#include <QtCore/QSettings>

namespace Otter
{

GesturesManager* GesturesManager::m_instance = NULL;
QjtMouseGestureFilter* GesturesManager::m_filter = NULL;
QHash<QjtMouseGesture*, QString> GesturesManager::m_gestures;

GesturesManager::GesturesManager(QObject *parent) : QObject(parent)
{
	m_instance = this;

	loadProfiles();

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), m_instance, SLOT(optionChanged(QString)));
}

void GesturesManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new GesturesManager(parent);
	}
}

void GesturesManager::optionChanged(const QString &option)
{
	if (option == QLatin1String("Browser/GesturesProfilesOrder"))
	{
		loadProfiles();
	}
}

void GesturesManager::loadProfiles()
{
	if (m_filter)
	{
		m_filter->clearGestures(true);
	}
	else
	{
		m_filter = new QjtMouseGestureFilter(m_instance);
	}

	m_gestures.clear();

	const QStringList gestureProfiles = SettingsManager::getValue(QLatin1String("Browser/GesturesProfilesOrder")).toStringList();

	for (int i = 0; i < gestureProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/gestures/") + gestureProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/gestures/") + gestureProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList gestures = profile.childGroups();

		for (int j = 0; j < gestures.count(); ++j)
		{
			const QString action = profile.value(gestures.at(j) + QLatin1String("/action"), QString()).toString();
			const QStringList rawDirections = gestures.at(j).split(QLatin1Char(','));

			if (action.isEmpty() || rawDirections.isEmpty())
			{
				continue;
			}

			DirectionList directions;

			for (int k = 0; k < rawDirections.count(); ++k)
			{
				if (rawDirections.at(k) == QLatin1String("up"))
				{
					directions.append(Up);
				}
				else if (rawDirections.at(k) == QLatin1String("down"))
				{
					directions.append(Down);
				}
				else if (rawDirections.at(k) == QLatin1String("left"))
				{
					directions.append(Left);
				}
				else if (rawDirections.at(k) == QLatin1String("right"))
				{
					directions.append(Right);
				}
				else if (rawDirections.at(k) == QLatin1String("anyHorizontal"))
				{
					directions.append(AnyHorizontal);
				}
				else if (rawDirections.at(k) == QLatin1String("anyVertical"))
				{
					directions.append(AnyVertical);
				}
			}

			if (!directions.isEmpty())
			{
				QjtMouseGesture *gesture = new QjtMouseGesture(directions, m_instance);

				m_gestures[gesture] = action;

				m_filter->addGesture(gesture);

				connect(gesture, SIGNAL(gestured()), m_instance, SLOT(triggerGesture()));
			}
		}
	}
}

void GesturesManager::startGesture(QObject *object, QMouseEvent *event)
{
	if (m_filter)
	{
		m_filter->startGesture(object, event);
	}
}

bool GesturesManager::endGesture(QObject *object, QMouseEvent *event)
{
	if (m_filter)
	{
		return m_filter->endGesture(object, event);
	}

	return false;
}

void GesturesManager::triggerGesture()
{
	QjtMouseGesture *gesture = qobject_cast<QjtMouseGesture*>(sender());
	MainWindow *window = qobject_cast<MainWindow*>(SessionsManager::getActiveWindow());

	if (gesture && window)
	{
		window->getActionsManager()->triggerAction(m_gestures.value(gesture, QString()));
	}
}

GesturesManager* GesturesManager::getInstance()
{
	return m_instance;
}

}

