#include "ActionsManager.h"
#include "SettingsManager.h"

namespace Otter
{

ActionsManager* ActionsManager::m_instance = NULL;
QWidget* ActionsManager::m_activeWindow = NULL;
QHash<QWidget*, QHash<QString, QAction*> > ActionsManager::m_actions;

ActionsManager::ActionsManager(QObject *parent) : QObject(parent)
{
}

void ActionsManager::createInstance(QObject *parent)
{
	m_instance = new ActionsManager(parent);
}

void ActionsManager::addWindow(QWidget *window)
{
	if (!window)
	{
		return;
	}

	if (!m_actions.contains(window))
	{
		m_actions[window] = QHash<QString, QAction*>();

		connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(removeWindow(QObject*)));
	}
}

void ActionsManager::removeWindow(QObject *window)
{
	m_actions.remove(qobject_cast<QWidget*>(window));
}

void ActionsManager::addAction(QWidget *window, QAction *action, QString name)
{
	if (!action || action->isSeparator() || !m_actions.contains(window))
	{
		return;
	}

	if (name.isEmpty())
	{
		if (action->objectName().isEmpty())
		{
			QString tmpName = "_%1";
			int number = 0;

			while (m_actions[window].contains(tmpName.arg(number)))
			{
				++number;
			}

			name = tmpName.arg(number);
		}
		else
		{
			name = action->objectName().mid(6);
		}
	}

	action->setObjectName(name);
	action->setShortcut(QKeySequence(SettingsManager::getDefaultValue("Actions/" + name).toString()));

	m_actions[window][name] = action;
}

void ActionsManager::registerWindow(QWidget *window)
{
	m_instance->addWindow(window);
}

void ActionsManager::registerAction(QWidget *window, QAction *action, QString name)
{
	m_instance->addAction(window, action, name);
}

void ActionsManager::registerActions(QWidget *window, QList<QAction*> actions)
{
	for (int i = 0; i < actions.count(); ++i)
	{
		m_instance->addAction(window, actions.at(i));
	}
}

void ActionsManager::triggerAction(const QString &action)
{
	QAction *object = getAction(action);

	if (object)
	{
		object->trigger();
	}
}

void ActionsManager::setupLocalAction(QAction *localAction, const QString &globalAction, bool connectTrigger)
{
	if (!localAction)
	{
		return;
	}

	QAction *action = getAction(globalAction);

	if (action)
	{
		localAction->setEnabled(action->isEnabled());
		localAction->setIcon(action->icon());
		localAction->setText(action->text());
		localAction->setShortcut(action->shortcut());

		if (connectTrigger)
		{
			connect(localAction, SIGNAL(triggered()), action, SLOT(trigger()));
		}
	}
}

void ActionsManager::restoreDefaultShortcut(QAction *action)
{
	if (!m_actions.contains(m_activeWindow))
	{
		return;
	}

	const QString key = m_actions[m_activeWindow].key(action);

	if (!key.isEmpty())
	{
		restoreDefaultShortcut(key);
	}
}

void ActionsManager::restoreDefaultShortcut(const QString &action)
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

void ActionsManager::setShortcut(QAction *action, const QKeySequence &shortcut)
{
	if (!m_actions.contains(m_activeWindow))
	{
		return;
	}

	const QString key = m_actions[m_activeWindow].key(action);

	if (!key.isEmpty())
	{
		setShortcut(key, shortcut);
	}
}

void ActionsManager::setShortcut(const QString &action, const QKeySequence &shortcut)
{
	const QList<QWidget*> windows = m_actions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		if (m_actions.contains(windows.at(i)) && m_actions[windows.at(i)].contains(action))
		{
			m_actions[windows.at(i)].value(action)->setShortcut(shortcut);
		}
	}

	SettingsManager::setValue(("Actions/" + action), shortcut.toString());
}

void ActionsManager::setActiveWindow(QWidget *window)
{
	m_activeWindow = window;
}

QAction* ActionsManager::getAction(const QString &action)
{
	if (!m_actions.contains(m_activeWindow))
	{
		return NULL;
	}

	return m_actions[m_activeWindow].value(action, NULL);
}

QKeySequence ActionsManager::getShortcut(QAction *action)
{
	if (!m_actions.contains(m_activeWindow))
	{
		return QKeySequence();
	}

	const QString key = m_actions[m_activeWindow].key(action);

	if (key.isEmpty())
	{
		return QKeySequence();
	}
	else
	{
		return m_actions[m_activeWindow][key]->shortcut();
	}
}

QKeySequence ActionsManager::getShortcut(const QString &action)
{
	return QKeySequence(SettingsManager::getValue("Actions/" + action).toString());
}

QKeySequence ActionsManager::getDefaultShortcut(QAction *action)
{
	if (!m_actions.contains(m_activeWindow))
	{
		return QKeySequence();
	}

	const QString key = m_actions[m_activeWindow].key(action);

	if (key.isEmpty())
	{
		return QKeySequence();
	}
	else
	{
		return getDefaultShortcut(key);
	}
}

QKeySequence ActionsManager::getDefaultShortcut(const QString &action)
{
	return QKeySequence(SettingsManager::getDefaultValue("Actions/" + action).toString());
}

QStringList ActionsManager::getIdentifiers()
{
	if (!m_actions.contains(m_activeWindow))
	{
		return QStringList();
	}

	return m_actions[m_activeWindow].keys();
}

bool ActionsManager::hasShortcut(const QKeySequence &shortcut, const QString &excludeAction)
{
	if (!m_actions.contains(m_activeWindow) || shortcut.isEmpty())
	{
		return false;
	}

	const QList<QAction*> actions = m_actions[m_activeWindow].values();

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) && actions.at(i)->shortcut() == shortcut && (excludeAction.isEmpty() || actions.at(i)->objectName() != excludeAction))
		{
			return true;
		}
	}

	return false;
}

}
