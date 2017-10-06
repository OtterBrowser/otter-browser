/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
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

void ActionExecutor::Object::triggerAction(int identifier, const QVariantMap &parameters)
{
	if (!m_object.isNull())
	{
		m_executor->triggerAction(identifier, parameters);
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
	return !m_object.isNull();
}

ActionExecutor::ActionExecutor()
{
}

ActionExecutor::~ActionExecutor()
{
}

}
