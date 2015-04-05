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

#include <QtCore/QSettings>

namespace Otter
{

GesturesManager* GesturesManager::m_instance = NULL;
MouseGestures::Recognizer* GesturesManager::m_recognizer = NULL;
QHash<GesturesManager::GesturesContext, QVector<MouseGesture> > GesturesManager::m_gestures;
GesturesManager::GesturesContext GesturesManager::m_context = GesturesManager::GenericGesturesContext;

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
	m_gestures.clear();

	const QStringList gestureProfiles = SettingsManager::getValue(QLatin1String("Browser/GesturesProfilesOrder")).toStringList();

	for (int i = 0; i < gestureProfiles.count(); ++i)
	{
		QSettings profile(SessionsManager::getReadableDataPath(QLatin1String("gestures/") + gestureProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList contexts = profile.childGroups();

		for (int j = 0; j < contexts.count(); ++j)
		{
			GesturesContext context = GenericGesturesContext;

			if (contexts.at(j) == QLatin1String("Link"))
			{
				context = LinkGesturesContext;
			}

			profile.beginGroup(contexts.at(j));

			const QStringList gestures = profile.allKeys();

			for (int k = 0; k < gestures.count(); ++k)
			{
				const QStringList rawMouseActions = gestures.at(k).split(QLatin1Char(','));
				const int action = ActionsManager::getActionIdentifier(profile.value(gestures.at(k), QString()).toString());

				if (action < 0 || rawMouseActions.isEmpty())
				{
					continue;
				}

				MouseGestures::ActionList mouseActions;

				for (int l = 0; l < rawMouseActions.count(); ++l)
				{
					if (rawMouseActions.at(l) == QLatin1String("up"))
					{
						mouseActions.push_back(MouseGestures::MoveUpMouseAction);
					}
					else if (rawMouseActions.at(l) == QLatin1String("down"))
					{
						mouseActions.push_back(MouseGestures::MoveDownMouseAction);
					}
					else if (rawMouseActions.at(l) == QLatin1String("left"))
					{
						mouseActions.push_back(MouseGestures::MoveLeftMouseAction);
					}
					else if (rawMouseActions.at(l) == QLatin1String("right"))
					{
						mouseActions.push_back(MouseGestures::MoveRightMouseAction);
					}
					else if (rawMouseActions.at(l) == QLatin1String("anyHorizontal"))
					{
						mouseActions.push_back(MouseGestures::MoveHorizontallyMouseAction);
					}
					else if (rawMouseActions.at(l) == QLatin1String("anyVertical"))
					{
						mouseActions.push_back(MouseGestures::MoveVerticallyMouseAction);
					}
				}

				if (!mouseActions.empty())
				{
					MouseGesture definition;
					definition.action = action;
					definition.mouseActions = mouseActions;
					definition.identifier = 0;

					if (!m_gestures.contains(context))
					{
						m_gestures[context] = QVector<MouseGesture>();
					}

					m_gestures[context].append(definition);
				}
			}

			profile.endGroup();
		}
	}
}

void GesturesManager::startGesture(GesturesContext context, QObject *object, QMouseEvent *event)
{
	if (!m_gestures.contains(context))
	{
		return;
	}

	m_context = context;

	m_recognizer = new MouseGestures::Recognizer();

	for (int i = 0; i < m_gestures[context].count(); ++i)
	{
		m_gestures[context][i].identifier = m_recognizer->registerGesture(m_gestures[context][i].mouseActions);
	}

	m_recognizer->startGesture(event->pos().x(), event->pos().y());

	object->installEventFilter(m_instance);
}

bool GesturesManager::endGesture(QObject *object, QMouseEvent *event)
{
	object->removeEventFilter(m_instance);

	if (m_recognizer && m_gestures.contains(m_context))
	{
		const int gesture = m_recognizer->endGesture(event->pos().x(), event->pos().y());

		if (gesture < 0)
		{
			return false;
		}

		for (int i = 0; i < m_gestures[m_context].count(); ++i)
		{
			if (m_gestures[m_context][i].identifier == gesture)
			{
				MainWindow *window = qobject_cast<MainWindow*>(SessionsManager::getActiveWindow());

				if (window)
				{
					window->triggerAction(m_gestures[m_context][i].action);
				}

				return true;
			}
		}
	}

	return false;
}

GesturesManager* GesturesManager::getInstance()
{
	return m_instance;
}

bool GesturesManager::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseMove && m_recognizer)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent)
		{
			m_recognizer->addPosition(mouseEvent->pos().x(), mouseEvent->pos().y());
		}

		return true;
	}

	return QObject::eventFilter(object, event);
}

}

