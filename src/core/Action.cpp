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

#include "Action.h"
#include "ActionsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

Action::Action(const QIcon &icon, const QString &text, QObject *parent) : QAction(icon, text, parent),
	m_identifier(UnknownAction),
	m_scope(MainWindowScope)
{
	setText(QCoreApplication::translate("actions", text.toUtf8().constData()));
}

void Action::setName(const QString &name)
{
	setObjectName(name);
}

void Action::setIdentifier(ActionIdentifier identifier)
{
	m_identifier = identifier;
}

void Action::setScope(ActionScope scope)
{
	m_scope = scope;
}

QString Action::getName() const
{
	return objectName();
}

ActionIdentifier Action::getIdentifier() const
{
	return m_identifier;
}

ActionScope Action::getScope() const
{
	return m_scope;
}

bool Action::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		setText(QCoreApplication::translate("actions", ActionsManager::getActionDefinition(objectName()).text.toUtf8().constData()));
	}

	return QAction::event(event);
}

}
