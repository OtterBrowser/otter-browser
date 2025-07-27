/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/Console.h"
#include "../core/ThemesManager.h"

#include <QtCore/QMetaMethod>

namespace Otter
{

Action::Action(const QString &text, bool isTranslateable, QObject *parent) : QAction(parent),
	m_textOverride(text),
	m_flags(HasTextOverrideFlag),
	m_identifier(-1)
{
	if (isTranslateable)
	{
		m_flags |= IsTextOverrideTranslateableFlag;
	}

	initialize();
}

Action::Action(int identifier, const QVariantMap &parameters, QObject *parent) : QAction(parent),
	m_parameters(parameters),
	m_flags(NoFlags),
	m_identifier(identifier)
{
	initialize();
}

Action::Action(int identifier, const QVariantMap &parameters, const ActionExecutor::Object &executor, QObject *parent) : QAction(parent),
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

	if (definition.isValid())
	{
		if (definition.flags.testFlag(ActionsManager::ActionDefinition::IsCheckableFlag))
		{
			setCheckable(true);
		}

		if (definition.flags.testFlag(ActionsManager::ActionDefinition::IsDeprecatedFlag))
		{
			Console::addMessage(tr("Creating instance of deprecated action: %1").arg(ActionsManager::getActionName(m_identifier)), Console::OtherCategory, Console::WarningLevel);
		}

		connect(this, &Action::triggered, this, &Action::triggerAction);
		connect(ActionsManager::getInstance(), &ActionsManager::shortcutsChanged, this, &Action::updateShortcut);
	}

	setShortcutContext(Qt::WidgetShortcut);
	updateIcon();
	updateState();
}

void Action::triggerAction(bool isChecked)
{
	if (!m_executor.isValid())
	{
		return;
	}

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

void Action::handleArbitraryActionsStateChanged(const QVector<int> &identifiers)
{
	if (identifiers.contains(m_identifier))
	{
		updateState();
	}
}

void Action::handleCategorizedActionsStateChanged(const QVector<int> &categories)
{
	if (categories.contains(getDefinition().category))
	{
		updateState();
	}
}

void Action::updateIcon()
{
	if (hasIconOverride())
	{
		return;
	}

	const bool isLeftToRight(QGuiApplication::isLeftToRight());

	switch (m_identifier)
	{
		case ActionsManager::GoBackAction:
			setIcon(ThemesManager::createIcon(isLeftToRight ? QLatin1String("go-previous") : QLatin1String("go-next")));

			break;
		case ActionsManager::GoForwardAction:
			setIcon(ThemesManager::createIcon(isLeftToRight ? QLatin1String("go-next") : QLatin1String("go-previous")));

			break;
		case ActionsManager::RewindAction:
			setIcon(ThemesManager::createIcon(isLeftToRight ? QLatin1String("go-first") : QLatin1String("go-last")));

			break;
		case ActionsManager::FastForwardAction:
			setIcon(ThemesManager::createIcon(isLeftToRight ? QLatin1String("go-last") : QLatin1String("go-first")));

			break;
		default:
			break;
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

	if (hasTextOverride())
	{
		if (isTextOverrideTranslateable())
		{
			state.text = QCoreApplication::translate("actions", m_textOverride.toUtf8().constData());
		}
		else
		{
			state.text = m_textOverride;
		}
	}

	if (definition.flags.testFlag(ActionsManager::ActionDefinition::RequiresParameters) && m_parameters.isEmpty())
	{
		state.isEnabled = false;
	}

	updateShortcut();
	setState(state);
}

void Action::setExecutor(ActionExecutor::Object executor)
{
	const ActionsManager::ActionDefinition definition(getDefinition());
	const QMetaMethod updateStateMethod(getMethod("updateState()"));
	const QMetaMethod handleArbitraryActionsStateChangedMethod(getMethod("handleArbitraryActionsStateChanged(QVector<int>)"));
	const QMetaMethod handleCategorizedActionsStateChangeddMethod(getMethod("handleCategorizedActionsStateChanged(QVector<int>)"));
	const bool isExecutorValid(executor.isValid());

	if (m_executor.isValid())
	{
		m_executor.disconnectSignals(this, &updateStateMethod, &handleArbitraryActionsStateChangedMethod, &handleCategorizedActionsStateChangeddMethod);
	}

	if (isExecutorValid)
	{
		QObject *object(executor.getObject());

		switch (definition.scope)
		{
			case ActionsManager::ActionDefinition::MainWindowScope:
				if (!object->inherits("Otter::Application") && !object->inherits("Otter::MainWindow"))
				{
					MainWindow *mainWindow(MainWindow::findMainWindow(object));

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
				if (!object->inherits("Otter::Application"))
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

	if (isExecutorValid)
	{
		m_executor.connectSignals(this, &updateStateMethod, &handleArbitraryActionsStateChangedMethod, &handleCategorizedActionsStateChangeddMethod);
	}
}

void Action::setIconOverride(const QString &icon)
{
	setIconOverride(ThemesManager::createIcon(icon));
}

void Action::setIconOverride(const QIcon &icon)
{
	m_flags |= HasIconOverrideFlag;

	setIcon(icon);
}

void Action::setTextOverride(const QString &text, bool isTranslateable)
{
	m_textOverride = text;
	m_flags |= HasTextOverrideFlag;
	m_flags.setFlag(IsTextOverrideTranslateableFlag, isTranslateable);

	updateState();
}

void Action::setState(const ActionsManager::ActionDefinition::State &state)
{
	setStatusTip(state.statusTip);
	setToolTip(state.toolTip);
	setText(state.text);
	setEnabled(state.isEnabled);
	setChecked(state.isChecked);

	if (!hasIconOverride())
	{
		setIcon(state.icon);
		updateIcon();
	}
}

QString Action::getTextOverride() const
{
	return m_textOverride;
}

QMetaMethod Action::getMethod(const char *method) const
{
	return metaObject()->method(metaObject()->indexOfMethod(method));
}

ActionsManager::ActionDefinition Action::getDefinition() const
{
	return ActionsManager::getActionDefinition(m_identifier);
}

QVariantMap Action::getParameters() const
{
	return m_parameters;
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::hasIconOverride() const
{
	return m_flags.testFlag(HasIconOverrideFlag);
}

bool Action::hasTextOverride() const
{
	return m_flags.testFlag(HasTextOverrideFlag);
}

bool Action::isTextOverrideTranslateable() const
{
	return m_flags.testFlag(IsTextOverrideTranslateableFlag);
}

bool Action::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::LanguageChange:
			updateState();

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
