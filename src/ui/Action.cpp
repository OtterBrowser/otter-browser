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
	m_flags(CanTriggerActionFlag | FollowsActionStateFlag),
	m_identifier(identifier)
{
	initialize();
}

Action::Action(int identifier, const QVariantMap &parameters, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(CanTriggerActionFlag | FollowsActionStateFlag),
	m_identifier(identifier)
{
	initialize();
}

Action::Action(int identifier, const QVariantMap &parameters, ActionFlags flags, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(flags),
	m_identifier(identifier)
{
	initialize();
}

void Action::initialize()
{
	switch (m_identifier)
	{
		case ActionsManager::PreferencesAction:
			setMenuRole(QAction::PreferencesRole);

			break;
		case ActionsManager::AboutQtAction:
			setMenuRole(QAction::AboutQtRole);

			break;
		case ActionsManager::ExitAction:
			setMenuRole(QAction::QuitRole);

			break;
		case ActionsManager::AboutApplicationAction:
			setMenuRole(QAction::AboutRole);

			break;
		default:
			break;
	}

	if (m_identifier > 0)
	{
		setCheckable(getDefinition().flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag));
	}

	if (m_flags.testFlag(CanTriggerActionFlag))
	{
		connect(this, SIGNAL(triggered(bool)), this, SLOT(triggerAction(bool)));
	}

	update(true);
}

void Action::triggerAction(bool isChecked)
{
	if (m_executor.isValid())
	{
		if (m_identifier > 0 && getDefinition().flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag))
		{
			QVariantMap parameters(m_parameters);
			parameters[QLatin1String("isChecked")] = isChecked;

			m_executor.triggerAction(m_identifier, parameters);
		}
		else
		{
			m_executor.triggerAction(m_identifier, m_parameters);
		}
	}
}

void Action::handleActionsStateChanged(const QVector<int> &identifiers)
{
	if (identifiers.contains(m_identifier))
	{
		updateState();
	}
}

void Action::handleActionsStateChanged(ActionsManager::ActionDefinition::ActionCategories categories)
{
	const ActionsManager::ActionDefinition::ActionCategory category(getDefinition().category);

	if (category != ActionsManager::ActionDefinition::OtherCategory && categories.testFlag(category))
	{
		updateState();
	}
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
	const QVector<QKeySequence> shortcuts(m_parameters.isEmpty() ? definition.shortcuts : ActionsManager::getActionShortcuts(m_identifier, m_parameters));
	ActionsManager::ActionDefinition::State state;

	if (reset)
	{
		state = definition.defaultState;
		state.text = QCoreApplication::translate("actions", state.text.toUtf8().constData());

		switch (m_identifier)
		{
			case ActionsManager::GoBackAction:
				state.icon = ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next"));

				break;
			case ActionsManager::GoForwardAction:
				state.icon = ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous"));

				break;
			case ActionsManager::RewindAction:
				state.icon = ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-first") : QLatin1String("go-last"));

				break;
			case ActionsManager::FastForwardAction:
				state.icon = ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-last") : QLatin1String("go-first"));

				break;
			default:
				break;
		}
	}
	else
	{
		state = getState();
	}

	if (m_flags.testFlag(IsOverridingTextFlag))
	{
		state.text = QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	if (!shortcuts.isEmpty())
	{
		state.text += QLatin1Char('\t') + shortcuts.first().toString(QKeySequence::NativeText);
	}

	setState(state);
}

void Action::updateState()
{
	const ActionsManager::ActionDefinition definition(getDefinition());
	const QVector<QKeySequence> shortcuts(m_parameters.isEmpty() ? definition.shortcuts : ActionsManager::getActionShortcuts(m_identifier, m_parameters));
	ActionsManager::ActionDefinition::State state;

	if (m_executor.isValid())
	{
		state = m_executor.getActionState(m_identifier, m_parameters);
	}
	else
	{
		state = definition.defaultState;
		state.isEnabled = false;
	}

	if (m_flags.testFlag(IsOverridingTextFlag))
	{
		state.text = QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	if (!shortcuts.isEmpty())
	{
		state.text += QLatin1Char('\t') + shortcuts.first().toString(QKeySequence::NativeText);
	}

	setState(state);
}

void Action::setExecutor(ActionExecutor::Object executor)
{
	if (m_executor.isValid())
	{
		if (executor.getObject()->inherits("Otter::MainWindow"))
		{
			disconnect(m_executor.getObject(), SIGNAL(currentWindowChanged(quint64)), this, SLOT(updateState()));
		}

		disconnect(m_executor.getObject(), SIGNAL(actionsStateChanged(QVector<int>)), this, SLOT(handleActionsStateChanged(QVector<int>)));
		disconnect(m_executor.getObject(), SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SLOT(handleActionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
	}

	m_executor = executor;

	if (executor.isValid())
	{
		updateState();

		connect(executor.getObject(), SIGNAL(actionsStateChanged(QVector<int>)), this, SLOT(handleActionsStateChanged(QVector<int>)));

		if (getDefinition().category != ActionsManager::ActionDefinition::OtherCategory)
		{
			connect(executor.getObject(), SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SLOT(handleActionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
		}

		if (executor.getObject()->inherits("Otter::MainWindow"))
		{
			connect(executor.getObject(), SIGNAL(currentWindowChanged(quint64)), this, SLOT(updateState()));
		}
	}
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;

	m_flags |= IsOverridingTextFlag;

	update();
}

void Action::setOverrideIcon(const QIcon &icon)
{
	m_flags |= IsOverridingIconFlag;

	setIcon(icon);
}

void Action::setState(const ActionsManager::ActionDefinition::State &state)
{
	setText(state.text);
	setEnabled(state.isEnabled);
	setChecked(state.isChecked);

	if (!m_flags.testFlag(IsOverridingIconFlag))
	{
		setIcon(state.icon);
	}
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
