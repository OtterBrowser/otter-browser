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

Action::Action(int identifier, const QIcon &icon, const QString &text, QObject *parent) : QAction(icon, text, parent),
	m_identifier(identifier)
{
	update();
}

void Action::setup(Action *action)
{
	if (!action)
	{
		update();
		setEnabled(false);

		return;
	}

	setEnabled(action->isEnabled());
	setText(action->text());
	setIcon(action->icon());
	setCheckable(action->isCheckable());
	setChecked(action->isChecked());
}

void Action::update()
{
	const ActionDefinition action = ActionsManager::getActionDefinition(m_identifier);
	QString text = QCoreApplication::translate("actions", action.text.toUtf8().constData());

	if (!action.shortcuts.isEmpty())
	{
		text += QLatin1Char('\t') + action.shortcuts.first().toString(QKeySequence::NativeText);
	}

	setText(text);
}

QList<QKeySequence> Action::getShortcuts() const
{
	return ActionsManager::getActionDefinition(m_identifier).shortcuts.toList();
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		update();
	}

	return QAction::event(event);
}

}
