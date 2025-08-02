/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACTIONEXECUTOR_H
#define OTTER_ACTIONEXECUTOR_H

#include "ActionsManager.h"

#include <QtCore/QMetaMethod>
#include <QtCore/QPointer>

namespace Otter
{

class ActionExecutor
{
public:
	class Object final
	{
	public:
		explicit Object();
		explicit Object(QObject *object, ActionExecutor *executor);
		Object(const Object &other);

		void connectSignals(const QObject *receiver, const QMetaMethod *actionsStateChangedMethod, const QMetaMethod *arbitraryActionsStateChangedMethod, const QMetaMethod *categorizedActionsStateChangedMethod);
		void disconnectSignals(const QObject *receiver, const QMetaMethod *actionsStateChangedMethod, const QMetaMethod *arbitraryActionsStateChangedMethod, const QMetaMethod *categorizedActionsStateChangedMethod);
		void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger);
		QObject* getObject() const;
		Object& operator=(const Object &other);
		ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const;
		bool isValid() const;

	protected:
		QMetaMethod getMethod(const QMetaObject *metaObject, const char *method) const;

	private:
		QPointer<QObject> m_object;
		ActionExecutor *m_executor;
	};

	explicit ActionExecutor();
	virtual ~ActionExecutor();

	virtual ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const = 0;
	virtual void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) = 0;
	virtual bool isAboutToClose() const;
};

}

#endif
