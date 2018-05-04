/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionExecutor.h"

namespace Otter
{

ActionExecutor::Object::Object() : m_executor(nullptr)
{
}

ActionExecutor::Object::Object(QObject *object, ActionExecutor *executor) : m_object(object), m_executor(executor)
{
}

ActionExecutor::Object::Object(const Object &other) : m_object(other.m_object), m_executor(other.m_executor)
{
}

void ActionExecutor::Object::connectSignals(const QObject *receiver, const QMetaMethod *actionsStateChangedMethod, const QMetaMethod *arbitraryActionsStateChangedMethod, const QMetaMethod *categorizedActionsStateChangedMethod)
{
	if (receiver && m_object)
	{
		const QMetaObject *metaObject(m_object.data()->metaObject());
		const QMetaMethod actionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("actionsStateChanged()")));
		const QMetaMethod arbitraryActionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("arbitraryActionsStateChanged(QVector<int>)")));
		const QMetaMethod categorizedActionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("categorizedActionsStateChanged(QVector<int>)")));

		if (actionsStateChangedSignal.isValid() && actionsStateChangedMethod)
		{
			QObject::connect(m_object.data(), actionsStateChangedSignal, receiver, *(actionsStateChangedMethod));
		}

		if (arbitraryActionsStateChangedSignal.isValid() && arbitraryActionsStateChangedMethod)
		{
			QObject::connect(m_object.data(), arbitraryActionsStateChangedSignal, receiver, *(arbitraryActionsStateChangedMethod));
		}

		if (categorizedActionsStateChangedSignal.isValid() && categorizedActionsStateChangedMethod)
		{
			QObject::connect(m_object.data(), categorizedActionsStateChangedSignal, receiver, *(categorizedActionsStateChangedMethod));
		}
	}
}

void ActionExecutor::Object::disconnectSignals(const QObject *receiver, const QMetaMethod *actionsStateChangedMethod, const QMetaMethod *arbitraryActionsStateChangedMethod, const QMetaMethod *categorizedActionsStateChangedMethod)
{
	if (receiver && m_object)
	{
		const QMetaObject *metaObject(m_object.data()->metaObject());
		const QMetaMethod actionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("actionsStateChanged()")));
		const QMetaMethod arbitraryActionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("arbitraryActionsStateChanged(QVector<int>)")));
		const QMetaMethod categorizedActionsStateChangedSignal(metaObject->method(metaObject->indexOfSignal("categorizedActionsStateChanged(QVector<int>)")));

		if (actionsStateChangedSignal.isValid() && actionsStateChangedMethod)
		{
			QObject::disconnect(m_object.data(), actionsStateChangedSignal, receiver, *(actionsStateChangedMethod));
		}

		if (arbitraryActionsStateChangedSignal.isValid() && arbitraryActionsStateChangedMethod)
		{
			QObject::disconnect(m_object.data(), arbitraryActionsStateChangedSignal, receiver, *(arbitraryActionsStateChangedMethod));
		}

		if (categorizedActionsStateChangedSignal.isValid() && categorizedActionsStateChangedMethod)
		{
			QObject::disconnect(m_object.data(), categorizedActionsStateChangedSignal, receiver, *(categorizedActionsStateChangedMethod));
		}
	}
}

void ActionExecutor::Object::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	if (!m_object.isNull())
	{
		m_executor->triggerAction(identifier, parameters, trigger);
	}
}

QObject* ActionExecutor::Object::getObject() const
{
	return m_object.data();
}

ActionExecutor::Object& ActionExecutor::Object::operator=(const Object &other)
{
	if (this != &other)
	{
		m_object = other.m_object;
		m_executor = other.m_executor;
	}

	return *this;
}

ActionsManager::ActionDefinition::State ActionExecutor::Object::getActionState(int identifier, const QVariantMap &parameters) const
{
	if (!m_object.isNull())
	{
		return m_executor->getActionState(identifier, parameters);
	}

	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
	state.isEnabled = false;

	return state;
}

bool ActionExecutor::Object::isValid() const
{
	return (!m_object.isNull() && !m_executor->isAboutToClose());
}

ActionExecutor::ActionExecutor()
{
}

ActionExecutor::~ActionExecutor()
{
}

bool ActionExecutor::isAboutToClose() const
{
	return false;
}

}
