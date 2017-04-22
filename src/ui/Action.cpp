/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Action.h"
#include "../core/ThemesManager.h"

#include <QtGui/QGuiApplication>

namespace Otter
{

Action::Action(int identifier, QObject *parent) : QAction(parent),
	m_flags(CanTriggerAction | FollowsActionStateFlag),
	m_identifier(identifier)
{
	update(true);
}

Action::Action(int identifier, const QVariantMap &parameters, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(CanTriggerAction | FollowsActionStateFlag),
	m_identifier(identifier)
{
	update(true);
}

Action::Action(int identifier, const QVariantMap &parameters, ActionFlags flags, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(flags),
	m_identifier(identifier)
{
	update(true);
}

void Action::setup(Action *action)
{
	if (!action)
	{
		action = qobject_cast<Action*>(sender());
	}

	if (!action)
	{
		update(true);
		setEnabled(false);

		return;
	}

	m_identifier = action->getIdentifier();

	setCheckable(action->isCheckable());
	setState(action->getState());
}

void Action::update(bool reset)
{
	if (m_identifier < 0)
	{
		if (m_flags.testFlag(IsOverridingTextFlag))
		{
			setText(QCoreApplication::translate("actions", m_overrideText.toUtf8().constData()));
		}

		return;
	}

	const ActionsManager::ActionDefinition definition(getDefinition());
	ActionsManager::ActionDefinition::State state;

	if (reset)
	{
		state = definition.defaultState;
		state.text = QCoreApplication::translate("actions", state.text.toUtf8().constData());

		switch (m_identifier)
		{
			case ActionsManager::GoBackAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next"));

				break;
			case ActionsManager::GoForwardAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous"));

				break;
			case ActionsManager::RewindAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-first") : QLatin1String("go-last"));

				break;
			case ActionsManager::FastForwardAction:
				state.icon = ThemesManager::getIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-last") : QLatin1String("go-first"));

				break;
			default:
				break;
		}

		setCheckable(definition.flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag));
	}
	else
	{
		state = getState();
	}

	if (m_flags.testFlag(IsOverridingTextFlag))
	{
		state.text = QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	if (m_parameters.isEmpty() && !definition.shortcuts.isEmpty())
	{
		state.text += QLatin1Char('\t') + definition.shortcuts.first().toString(QKeySequence::NativeText);
	}

	setState(state);
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;

	m_flags |= IsOverridingTextFlag;

	update();
}

void Action::setState(const ActionsManager::ActionDefinition::State &state)
{
	setText(state.text);
	setIcon(state.icon);
	setEnabled(state.isEnabled);
	setChecked(state.isChecked);
}

void Action::setParameters(const QVariantMap &parameters)
{
	m_parameters = parameters;

	update();
}

QString Action::getText() const
{
	if (m_flags.testFlag(IsOverridingTextFlag))
	{
		return QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	return getDefinition().getText();
}

Otter::ActionsManager::ActionDefinition Action::getDefinition() const
{
	return ActionsManager::getActionDefinition(m_identifier);
}

ActionsManager::ActionDefinition::State Action::getState() const
{
	ActionsManager::ActionDefinition::State state;
	state.text = getText();
	state.icon = icon();
	state.isEnabled = isEnabled();
	state.isChecked = isChecked();

	return state;
}

QVariantMap Action::getParameters() const
{
	return m_parameters;
}

QVector<QKeySequence> Action::getShortcuts() const
{
	return getDefinition().shortcuts;
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LanguageChange:
			update();

			break;
		case QEvent::LayoutDirectionChange:
			switch (m_identifier)
			{
				case ActionsManager::GoBackAction:
				case ActionsManager::GoForwardAction:
				case ActionsManager::RewindAction:
				case ActionsManager::FastForwardAction:
					update(true);

					break;
				default:
					break;
			}

			break;
		default:
			break;
	}

	return QAction::event(event);
}

bool Action::calculateCheckedState(const QVariantMap &parameters, Action *action)
{
	if (parameters.contains(QLatin1String("isChecked")))
	{
		return parameters.value(QLatin1String("isChecked")).toBool();
	}

	if (action)
	{
		action->toggle();

		return action->isChecked();
	}

	return true;
}

Shortcut::Shortcut(int identifier, const QKeySequence &sequence, QWidget *parent) : QShortcut(sequence, parent),
	m_identifier(identifier)
{
}

Shortcut::Shortcut(int identifier, const QKeySequence &sequence, const QVariantMap &parameters, QWidget *parent) : QShortcut(sequence, parent),
	m_parameters(parameters),
	m_identifier(identifier)
{
}

void Shortcut::setParameters(const QVariantMap &parameters)
{
	m_parameters = parameters;
}

QVariantMap Shortcut::getParameters() const
{
	return m_parameters;
}

int Shortcut::getIdentifier() const
{
	return m_identifier;
}

}
