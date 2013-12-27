#include "ActionsManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

namespace Otter
{

ActionsManager* ActionsManager::m_instance = NULL;
QHash<QWidget*, QHash<QString, QAction*> > ActionsManager::m_actions;

ActionsManager::ActionsManager(QObject *parent) : QObject(parent)
{
}

void ActionsManager::createInstance(QObject *parent)
{
	m_instance = new ActionsManager(parent);
}

void ActionsManager::removeWindow(QObject *window)
{
	m_actions.remove(qobject_cast<QWidget*>(window));
}

void ActionsManager::registerWindow(QWidget *window)
{
	if (!window)
	{
		return;
	}

	if (!m_actions.contains(window))
	{
		m_actions[window] = QHash<QString, QAction*>();

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
	if (!action || action->isSeparator() || action->objectName().isEmpty() || !m_actions.contains(window))
	{
		return;
	}

	const QString name = (action->objectName().startsWith(QLatin1String("action")) ? action->objectName().mid(6) : action->objectName());

	action->setShortcut(QKeySequence(SettingsManager::getDefaultValue(QLatin1String("Actions/") + name).toString()));

	m_actions[window][name] = action;
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

void ActionsManager::restoreDefaultShortcut(const QLatin1String &action)
{
	const QList<QWidget*> windows = m_actions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (m_actions.contains(windows.at(i)) && m_actions[windows.at(i)].contains(action))
		{
			m_actions[windows.at(i)].value(action)->setShortcut(getDefaultShortcut(action));
		}
	}
}

void ActionsManager::setShortcut(const QLatin1String &action, const QKeySequence &shortcut)
{
	const QList<QWidget*> windows = m_actions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (m_actions.contains(windows.at(i)) && m_actions[windows.at(i)].contains(action))
		{
			m_actions[windows.at(i)].value(action)->setShortcut(shortcut);
		}
	}

	SettingsManager::setValue(QLatin1String("Actions/") + action, shortcut.toString());
}

QAction* ActionsManager::getAction(const QLatin1String &action)
{
	if (!m_actions.contains(SessionsManager::getActiveWindow()) || !m_actions[SessionsManager::getActiveWindow()].contains(action))
	{
		return NULL;
	}

	return m_actions[SessionsManager::getActiveWindow()][action];
}

QKeySequence ActionsManager::getShortcut(const QLatin1String &action)
{
	return QKeySequence(SettingsManager::getValue(QLatin1String("Actions/") + action).toString());
}

QKeySequence ActionsManager::getDefaultShortcut(const QLatin1String &action)
{
	return QKeySequence(SettingsManager::getDefaultValue(QLatin1String("Actions/") + action).toString());
}

QStringList ActionsManager::getIdentifiers()
{
	if (!m_actions.contains(SessionsManager::getActiveWindow()))
	{
		return QStringList();
	}

	return m_actions[SessionsManager::getActiveWindow()].keys();
}

bool ActionsManager::hasShortcut(const QKeySequence &shortcut, const QLatin1String &excludeAction)
{
	if (!m_actions.contains(SessionsManager::getActiveWindow()) || shortcut.isEmpty())
	{
		return false;
	}

	const QList<QAction*> actions = m_actions[SessionsManager::getActiveWindow()].values();

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
