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
QHash<QWidget*, QHash<QString, QAction*> > ActionsManager::m_windowActions;
QHash<QString, QAction*> ActionsManager::m_applicationActions;
QHash<QString, QKeySequence> ActionsManager::m_nativeShortcuts;

ActionsManager::ActionsManager(QObject *parent) : QObject(parent)
{
	m_nativeShortcuts[QLatin1String("NewWindow")] = QKeySequence(QKeySequence::New);
	m_nativeShortcuts[QLatin1String("Open")] = QKeySequence(QKeySequence::Open);
	m_nativeShortcuts[QLatin1String("Save")] = QKeySequence(QKeySequence::Save);
	m_nativeShortcuts[QLatin1String("Exit")] = QKeySequence(QKeySequence::Quit);
	m_nativeShortcuts[QLatin1String("Undo")] = QKeySequence(QKeySequence::Undo);
	m_nativeShortcuts[QLatin1String("Redo")] = QKeySequence(QKeySequence::Redo);
	m_nativeShortcuts[QLatin1String("Redo")] = QKeySequence(QKeySequence::Redo);
	m_nativeShortcuts[QLatin1String("Cut")] = QKeySequence(QKeySequence::Cut);
	m_nativeShortcuts[QLatin1String("Copy")] = QKeySequence(QKeySequence::Copy);
	m_nativeShortcuts[QLatin1String("Paste")] = QKeySequence(QKeySequence::Paste);
	m_nativeShortcuts[QLatin1String("Delete")] = QKeySequence(QKeySequence::Delete);
	m_nativeShortcuts[QLatin1String("SelectAll")] = QKeySequence(QKeySequence::SelectAll);
	m_nativeShortcuts[QLatin1String("Find")] = QKeySequence(QKeySequence::Find);
	m_nativeShortcuts[QLatin1String("FindNext")] = QKeySequence(QKeySequence::FindNext);
	m_nativeShortcuts[QLatin1String("FindPrevious")] = QKeySequence(QKeySequence::FindPrevious);
	m_nativeShortcuts[QLatin1String("Reload")] = QKeySequence(QKeySequence::Refresh);
	m_nativeShortcuts[QLatin1String("ZoomIn")] = QKeySequence(QKeySequence::ZoomIn);
	m_nativeShortcuts[QLatin1String("ZoomOut")] = QKeySequence(QKeySequence::ZoomOut);
	m_nativeShortcuts[QLatin1String("Back")] = QKeySequence(QKeySequence::Back);
	m_nativeShortcuts[QLatin1String("Forward")] = QKeySequence(QKeySequence::Forward);
	m_nativeShortcuts[QLatin1String("Help")] = QKeySequence(QKeySequence::HelpContents);
	m_nativeShortcuts[QLatin1String("ApplicationConfiguration")] = QKeySequence(QKeySequence::Preferences);

	optionChanged(QLatin1String("Browser/ActionMacrosProfiles"), SettingsManager::getValue(QLatin1String("Browser/ActionMacrosProfiles")));
	optionChanged(QLatin1String("Browser/KeyboardShortcutsProfiles"), SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfiles")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

ActionsManager::~ActionsManager()
{
	qDeleteAll(m_applicationActions);

	m_applicationActions.clear();
}

void ActionsManager::createInstance(QObject *parent)
{
	m_instance = new ActionsManager(parent);
}

void ActionsManager::optionChanged(const QString &option, const QVariant &value)
{
	Q_UNUSED(value)

	if (option == QLatin1String("Browser/ActionMacrosProfiles"))
	{
//TODO
	}
	else if (option == QLatin1String("Browser/KeyboardShortcutsProfiles"))
	{
//TODO
	}
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

//TODO setup shortcuts

		connect(window, SIGNAL(destroyed(QObject*)), m_instance, SLOT(removeWindow(QObject*)));
	}
}

void ActionsManager::registerAction(QWidget *window, const QLatin1String &name, const QString &text, const QIcon &icon)
{
	if (m_applicationActions.contains(name))
	{
		return;
	}

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

	if (m_nativeShortcuts.contains(name))
	{
		action->setShortcut(QKeySequence(m_nativeShortcuts[name]));
	}

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
