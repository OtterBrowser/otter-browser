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
#include "MainWindow.h"
#include "../core/Application.h"
#include "../core/ThemesManager.h"

namespace Otter
{

Action::Action(int identifier, const QVariantMap &parameters, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(NoFlags),
	m_identifier(identifier)
{
	initialize();
}

Action::Action(int identifier, const QVariantMap &parameters, ActionExecutor::Object executor, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(NoFlags),
	m_identifier(identifier)
{
	initialize();
	setExecutor(executor);
}

void Action::initialize()
{
	const ActionsManager::ActionDefinition definition(getDefinition());

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

	setShortcutContext(Qt::WidgetShortcut);

	if (definition.isValid())
	{
		if (definition.flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag))
		{
			setCheckable(true);
		}

		connect(this, SIGNAL(triggered(bool)), this, SLOT(triggerAction(bool)));
	}

	updateIcon();
	updateState();

	connect(ActionsManager::getInstance(), SIGNAL(shortcutsChanged()), this, SLOT(updateShortcut()));
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

void Action::updateIcon()
{
	if (!m_flags.testFlag(IsOverridingIconFlag))
	{
		switch (m_identifier)
		{
			case ActionsManager::GoBackAction:
				setIcon(ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-previous") : QLatin1String("go-next")));

				break;
			case ActionsManager::GoForwardAction:
				setIcon(ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-next") : QLatin1String("go-previous")));

				break;
			case ActionsManager::RewindAction:
				setIcon(ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-first") : QLatin1String("go-last")));

				break;
			case ActionsManager::FastForwardAction:
				setIcon(ThemesManager::createIcon(QGuiApplication::isLeftToRight() ? QLatin1String("go-last") : QLatin1String("go-first")));

				break;
			default:
				break;
		}
	}
}

void Action::updateShortcut()
{
	setShortcut(ActionsManager::getActionShortcut(m_identifier, m_parameters));
}

void Action::updateState()
{
	const ActionsManager::ActionDefinition definition(getDefinition());
	ActionsManager::ActionDefinition::State state;

	if (m_executor.isValid())
	{
		state = m_executor.getActionState(m_identifier, m_parameters);
	}
	else if (definition.isValid())
	{
		state = definition.getDefaultState();
		state.isEnabled = false;
	}

	if (m_flags.testFlag(IsOverridingTextFlag))
	{
		state.text = QCoreApplication::translate("actions", m_overrideText.toUtf8().constData());
	}

	if (definition.isValid() && definition.flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters) && m_parameters.isEmpty())
	{
		state.isEnabled = false;
	}

	updateShortcut();
	setState(state);
}

void Action::setExecutor(ActionExecutor::Object executor)
{
	const ActionsManager::ActionDefinition definition(getDefinition());

	if (m_executor.isValid())
	{
		if (m_executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged()") >= 0)
		{
			disconnect(m_executor.getObject(), SIGNAL(actionsStateChanged()), this, SLOT(updateState()));
		}

		if (m_executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged(QVector<int>)") >= 0)
		{
			disconnect(m_executor.getObject(), SIGNAL(actionsStateChanged(QVector<int>)), this, SLOT(handleActionsStateChanged(QVector<int>)));
		}

		if (definition.category != ActionsManager::ActionDefinition::OtherCategory && m_executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)") >= 0)
		{
			disconnect(m_executor.getObject(), SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SLOT(handleActionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
		}
	}

	if (executor.isValid())
	{
		switch (definition.scope)
		{
			case ActionsManager::ActionDefinition::MainWindowScope:
				if (!executor.getObject()->inherits("Otter::Application") && !executor.getObject()->inherits("Otter::MainWindow"))
				{
					MainWindow *mainWindow(MainWindow::findMainWindow(executor.getObject()));

					if (mainWindow)
					{
						executor = ActionExecutor::Object(mainWindow, mainWindow);
					}
					else
					{
						executor = ActionExecutor::Object();
					}
				}

				break;
			case ActionsManager::ActionDefinition::ApplicationScope:
				if (!executor.getObject()->inherits("Otter::Application"))
				{
					executor = ActionExecutor::Object(Application::getInstance(), Application::getInstance());
				}

				break;
			default:
				break;
		}
	}

	m_executor = executor;

	updateState();

	if (executor.isValid())
	{
		if (executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged()") >= 0)
		{
			connect(executor.getObject(), SIGNAL(actionsStateChanged()), this, SLOT(updateState()));
		}

		if (executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged(QVector<int>)") >= 0)
		{
			connect(executor.getObject(), SIGNAL(actionsStateChanged(QVector<int>)), this, SLOT(handleActionsStateChanged(QVector<int>)));
		}

		if (definition.category != ActionsManager::ActionDefinition::OtherCategory && executor.getObject()->metaObject()->indexOfSignal("actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)") >= 0)
		{
			connect(executor.getObject(), SIGNAL(actionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)), this, SLOT(handleActionsStateChanged(ActionsManager::ActionDefinition::ActionCategories)));
		}
	}
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;

	m_flags |= IsOverridingTextFlag;

	setState(getState());
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

Otter::ActionsManager::ActionDefinition Action::getDefinition() const
{
	return ActionsManager::getActionDefinition(m_identifier);
}

ActionsManager::ActionDefinition::State Action::getState() const
{
	ActionsManager::ActionDefinition::State state;
	state.text = (m_flags.testFlag(IsOverridingTextFlag) ? QCoreApplication::translate("actions", m_overrideText.toUtf8().constData()) : getDefinition().getText());
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
			setState(getState());

			break;
		case QEvent::LayoutDirectionChange:
			updateIcon();

			break;
		default:
			break;
	}

	return QAction::event(event);
}

}
