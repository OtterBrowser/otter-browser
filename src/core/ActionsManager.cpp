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

#include "ActionsManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

namespace Otter
{

ActionsManager* ActionsManager::m_instance = NULL;
QHash<QString, ActionTemplate*> ActionsManager::m_templateActions;
QHash<QWidget*, QHash<QString, QAction*> > ActionsManager::m_windowActions;

ActionsManager::ActionsManager(QObject *parent) : QObject(parent)
{
}

void ActionsManager::createInstance(QObject *parent)
{
	m_instance = new ActionsManager(parent);
}

void ActionsManager::removeWindow(QObject *window)
{
	m_windowActions.remove(qobject_cast<QWidget*>(window));
}

void ActionsManager::registerWindow(QWidget *window)
{
	if (!window)
	{
		return;
	}

	if (!m_windowActions.contains(window))
	{
		m_windowActions[window] = QHash<QString, QAction*>();

		connect(window, SIGNAL(destroyed(QObject*)), m_instance, SLOT(removeWindow(QObject*)));
	}
}

void ActionsManager::registerAction(QWidget *window, const QLatin1String &name, const QString &text, const QIcon &icon)
{
	QAction *action = new QAction(icon, text, window);
	action->setObjectName(name);

	registerAction(window, action);
}

void ActionsManager::registerAction(QWidget *window, QAction *action)
{
	if (!action || action->isSeparator() || action->objectName().isEmpty() || !m_windowActions.contains(window))
	{
		return;
	}

	const QString name = (action->objectName().startsWith(QLatin1String("action")) ? action->objectName().mid(6) : action->objectName());

	action->setShortcut(QKeySequence(SettingsManager::getDefaultValue(QLatin1String("Actions/") + name).toString()));

	m_windowActions[window][name] = action;
}

void ActionsManager::registerActions(QWidget *window, QList<QAction*> actions)
{
	for (int i = 0; i < actions.count(); ++i)
	{
		registerAction(window, actions.at(i));
	}
}

void ActionsManager::triggerAction(const QLatin1String &action)
{
	QAction *object = getAction(action);

	if (object)
	{
		object->trigger();
	}
}

void ActionsManager::setupLocalAction(QAction *localAction, const QLatin1String &globalAction, bool connectTrigger)
{
	if (!localAction)
	{
		return;
	}

	QAction *action = getAction(globalAction);

	if (action)
	{
		localAction->setCheckable(action->isCheckable());
		localAction->setChecked(action->isChecked());
		localAction->setEnabled(action->isEnabled());
		localAction->setIcon(action->icon());
		localAction->setText(action->text());
		localAction->setShortcut(action->shortcut());
		localAction->setObjectName(action->objectName());

		if (connectTrigger)
		{
			connect(localAction, SIGNAL(triggered()), action, SLOT(trigger()));
		}
	}
}

void ActionsManager::restoreDefaultShortcuts(const QLatin1String &action)
{
	const QList<QWidget*> windows = m_windowActions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (m_windowActions.contains(windows.at(i)) && m_windowActions[windows.at(i)].contains(action))
		{
//TODO
		}
	}
}

void ActionsManager::setShortcuts(const QLatin1String &action, const QList<QKeySequence> &shortcuts)
{
	Q_UNUSED(shortcuts)

	const QList<QWidget*> windows = m_windowActions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (m_windowActions.contains(windows.at(i)) && m_windowActions[windows.at(i)].contains(action))
		{
//TODO
		}
	}

//	SettingsManager::setValue(QLatin1String("Actions/") + action, shortcuts.toString());
}

QAction* ActionsManager::getAction(const QLatin1String &action)
{
	if (!m_windowActions.contains(SessionsManager::getActiveWindow()) || !m_windowActions[SessionsManager::getActiveWindow()].contains(action))
	{
		return NULL;
	}

	return m_windowActions[SessionsManager::getActiveWindow()][action];
}

QList<QKeySequence> ActionsManager::getShortcuts(const QLatin1String &action)
{
	Q_UNUSED(action)

	QList<QKeySequence> shortcuts;

//TODO

	return shortcuts;
}

QList<QKeySequence> ActionsManager::getDefaultShortcuts(const QLatin1String &action)
{
	Q_UNUSED(action)

	QList<QKeySequence> shortcuts;

//TODO

	return shortcuts;
}

QStringList ActionsManager::getIdentifiers()
{
	if (!m_windowActions.contains(SessionsManager::getActiveWindow()))
	{
		return QStringList();
	}

	return m_windowActions[SessionsManager::getActiveWindow()].keys();
}

bool ActionsManager::hasShortcut(const QKeySequence &shortcut, const QLatin1String &excludeAction)
{
	if (!m_windowActions.contains(SessionsManager::getActiveWindow()) || shortcut.isEmpty())
	{
		return false;
	}

	const QList<QAction*> actions = m_windowActions[SessionsManager::getActiveWindow()].values();

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) && actions.at(i)->shortcut() == shortcut && (excludeAction.size() == 0 || actions.at(i)->objectName() != excludeAction))
		{
			return true;
		}
	}

	return false;
}

}
