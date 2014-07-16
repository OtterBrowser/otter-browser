/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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
#include "ShortcutsManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>

namespace Otter
{

QHash<QString, ActionDefinition> ActionsManager::m_definitions;
QHash<QObject*, MainWindow*> ActionsManager::m_cache;

ActionsManager::ActionsManager(MainWindow *parent) : QObject(parent),
	m_window(parent)
{
	if (m_definitions.isEmpty())
	{
		registerAction(QLatin1String("OpenLinkInThisTab"), QT_TR_NOOP("Open"));
		registerAction(QLatin1String("OpenLinkInNewTab"), QT_TR_NOOP("Open in New Tab"));
		registerAction(QLatin1String("OpenLinkInNewTabBackground"), QT_TR_NOOP("Open in New Background Tab"));
		registerAction(QLatin1String("OpenLinkInNewWindow"), QT_TR_NOOP("Open in New Window"));
		registerAction(QLatin1String("OpenLinkInNewWindowBackground"), QT_TR_NOOP("Open in New Background Window"));
		registerAction(QLatin1String("CopyLinkToClipboard"), QT_TR_NOOP("Copy Link to Clipboard"));
		registerAction(QLatin1String("OpenFrameInThisTab"), QT_TR_NOOP("Open"));
		registerAction(QLatin1String("OpenFrameInNewTab"), QT_TR_NOOP("Open in New Tab"));
		registerAction(QLatin1String("OpenFrameInNewTabBackground"), QT_TR_NOOP("Open in New Background Tab"));
		registerAction(QLatin1String("CopyFrameLinkToClipboard"), QT_TR_NOOP("Copy Frame Link to Clipboard"));
		registerAction(QLatin1String("ReloadFrame"), QT_TR_NOOP("Reload"));
		registerAction(QLatin1String("ReloadImage"), QT_TR_NOOP("Reload Image"));
		registerAction(QLatin1String("ViewSourceFrame"), QT_TR_NOOP("View Source"));
		registerAction(QLatin1String("SaveLinkToDisk"), QT_TR_NOOP("Save Link Target As..."));
		registerAction(QLatin1String("SaveLinkToDownloads"), QT_TR_NOOP("Save to Downloads"));
		registerAction(QLatin1String("BookmarkLink"), QT_TR_NOOP("Bookmark Link..."), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ReloadTime"), QT_TR_NOOP("Reload Each"));
		registerAction(QLatin1String("CopyAddress"), QT_TR_NOOP("Copy Address"));
		registerAction(QLatin1String("Validate"), QT_TR_NOOP("Validate"));
		registerAction(QLatin1String("ContentBlocking"), QT_TR_NOOP("Content Blocking..."));
		registerAction(QLatin1String("WebsitePreferences"), QT_TR_NOOP("Website Preferences..."));
		registerAction(QLatin1String("ImageProperties"), QT_TR_NOOP("Image Properties..."));
		registerAction(QLatin1String("OpenImageInNewTab"), QT_TR_NOOP("Open Image"));
		registerAction(QLatin1String("SaveImageToDisk"), QT_TR_NOOP("Save Image..."));
		registerAction(QLatin1String("CopyImageToClipboard"), QT_TR_NOOP("Copy Image to Clipboard"));
		registerAction(QLatin1String("CopyImageUrlToClipboard"), QT_TR_NOOP("Copy Image Link to Clipboard"));
		registerAction(QLatin1String("Search"), QT_TR_NOOP("Search"));
		registerAction(QLatin1String("SearchMenu"), QT_TR_NOOP("Search Using"));
		registerAction(QLatin1String("OpenSelectionAsLink"), QT_TR_NOOP("Go to This Address"));
		registerAction(QLatin1String("ClearAll"), QT_TR_NOOP("Clear All"));
		registerAction(QLatin1String("SpellCheck"), QT_TR_NOOP("Check Spelling"));
		registerAction(QLatin1String("CreateSearch"), QT_TR_NOOP("Create Search..."));
		registerAction(QLatin1String("InspectElement"), QT_TR_NOOP("Inspect Element..."));
		registerAction(QLatin1String("SaveMediaToDisk"), QT_TR_NOOP("Save Media..."));
		registerAction(QLatin1String("CopyMediaUrlToClipboard"), QT_TR_NOOP("Copy Media Link to Clipboard"));
		registerAction(QLatin1String("ToggleMediaControls"), QT_TR_NOOP("Show Controls"));
		registerAction(QLatin1String("ToggleMediaLoop"), QT_TR_NOOP("Looping"));
		registerAction(QLatin1String("ToggleMediaPlayPause"), QT_TR_NOOP("Play"));
		registerAction(QLatin1String("ToggleMediaMute"), QT_TR_NOOP("Mute"));
		registerAction(QLatin1String("QuickBookmarkAccess"), QT_TR_NOOP("QuickBookmarkAccess"));
		registerAction(QLatin1String("QuickFind"), QT_TR_NOOP("Quick Find"), QIcon(), QuickFindAction);
		registerAction(QLatin1String("ActivateAddressField"), QT_TR_NOOP("Activate Address Field"), QIcon(), ActivateAddressFieldAction);
		registerAction(QLatin1String("CopyAsPlainText"), QT_TR_NOOP("Copy as Plain Text"), QIcon(), CopyAsPlainTextAction);
		registerAction(QLatin1String("PasteAndGo"), QT_TR_NOOP("Paste and Go"), QIcon(), PasteAndGoAction);
		registerAction(QLatin1String("ActivateTabOnLeft"), QT_TR_NOOP("Go to tab on left"), QIcon(), ActivateTabOnLeftAction);
		registerAction(QLatin1String("ActivateTabOnRight"), QT_TR_NOOP("Go to tab on right"), QIcon(), ActivateTabOnRightAction);
	}

	const QList<QAction*> windowActions = m_window->actions();

	for (int i = 0; i < windowActions.count(); ++i)
	{
		if (!windowActions.at(i)->objectName().isEmpty())
		{
			windowActions.at(i)->setShortcutContext(Qt::WidgetWithChildrenShortcut);

			m_actions[windowActions.at(i)->objectName().mid(6)] = windowActions.at(i);
		}
	}

	QHash<QString, ActionDefinition>::const_iterator definitionsIterator;

	for (definitionsIterator = m_definitions.constBegin(); definitionsIterator != m_definitions.constEnd(); ++definitionsIterator)
	{
		Action *action =  new Action(definitionsIterator.value().icon, definitionsIterator.value().text, m_window);
		action->setScope(definitionsIterator.value().scope);
		action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

		if (definitionsIterator.value().identifier != UnknownAction)
		{
			action->setIdentifier(definitionsIterator.value().identifier);
			action->setData(definitionsIterator.value().identifier);

			m_standardActions[definitionsIterator.value().identifier] = action;
		}

		m_actions[definitionsIterator.key()] = action;

		m_window->addAction(action);
	}

	updateActions();

	connect(ShortcutsManager::getInstance(), SIGNAL(shortcutsChanged()), this, SLOT(updateActions()));
}

ActionsManager::~ActionsManager()
{
	QHash<QObject*, MainWindow*>::iterator cacheIterator;

	for (cacheIterator = m_cache.begin(); cacheIterator != m_cache.end(); ++cacheIterator)
	{
		if (cacheIterator.value() == m_window)
		{
			cacheIterator = m_cache.erase(cacheIterator);
		}
	}
}

void ActionsManager::updateActions()
{
	const QHash<QString, QList<QKeySequence> > shortcuts = ShortcutsManager::getShortcuts();
	const QList<QAction*> actions = m_window->actions();

	for (int i = 0; i < actions.count(); ++i)
	{
		const QString name = actions.at(i)->objectName().mid(6);

		if (shortcuts.contains(name))
		{
			actions.at(i)->setShortcuts(shortcuts[name]);
		}
		else
		{
			actions.at(i)->setShortcut(QKeySequence());
		}
	}

	QHash<QString, ActionDefinition>::const_iterator definitionsIterator;

	for (definitionsIterator = m_definitions.constBegin(); definitionsIterator != m_definitions.constEnd(); ++definitionsIterator)
	{
		if (m_actions.contains(definitionsIterator.key()))
		{
			if (shortcuts.contains(definitionsIterator.key()))
			{
				m_actions[definitionsIterator.key()]->setShortcuts(shortcuts[definitionsIterator.key()]);
			}
			else
			{
				m_actions[definitionsIterator.key()]->setShortcut(QKeySequence());
			}
		}
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

void ActionsManager::triggerAction(ActionIdentifier action)
{
	QAction *object = getAction(action);

	if (object)
	{
		object->trigger();
	}
}

void ActionsManager::triggerAction(const QString &action, QObject *parent)
{
	ActionsManager *manager = findManager(parent);

	if (manager)
	{
		manager->triggerAction(action);
	}
}

void ActionsManager::triggerAction(ActionIdentifier action, QObject *parent)
{
	ActionsManager *manager = findManager(parent);

	if (manager)
	{
		manager->triggerAction(action);
	}
}

void ActionsManager::setupLocalAction(QAction *globalAction, QAction *localAction, bool connectTrigger)
{
	if (!globalAction || !localAction)
	{
		return;
	}

	localAction->setCheckable(globalAction->isCheckable());
	localAction->setChecked(globalAction->isChecked());
	localAction->setEnabled(globalAction->isEnabled());
	localAction->setIcon(globalAction->icon());
	localAction->setText(globalAction->text());
	localAction->setShortcut(globalAction->shortcut());
	localAction->setObjectName(globalAction->objectName());

	if (connectTrigger)
	{
		connect(localAction, SIGNAL(triggered()), globalAction, SLOT(trigger()));
	}
}

ActionsManager* ActionsManager::findManager(QObject *parent)
{
	MainWindow *window = NULL;
	QObject *originalParent = parent;

	if (parent && m_cache.contains(parent))
	{
		window = m_cache[parent];
	}
	else
	{
		while (parent)
		{
			if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
			{
				window = qobject_cast<MainWindow*>(parent);

				break;
			}

			parent = parent->parent();
		}
	}

	if (window)
	{
		m_cache[originalParent] = window;
	}
	else
	{
		window = qobject_cast<MainWindow*>(SessionsManager::getActiveWindow());
	}

	return (window ? window->getActionsManager() : NULL);
}

QAction* ActionsManager::getAction(const QString &action)
{
	return (m_actions.contains(action) ? m_actions[action] : NULL);
}

QAction* ActionsManager::getAction(ActionIdentifier action)
{
	return (m_standardActions.contains(action) ? m_standardActions[action] : NULL);
}

QAction* ActionsManager::getAction(const QString &action, QObject *parent)
{
	ActionsManager *manager = findManager(parent);

	return (manager ? manager->getAction(action) : NULL);
}

QAction* ActionsManager::getAction(ActionIdentifier action, QObject *parent)
{
	ActionsManager *manager = findManager(parent);

	return (manager ? manager->getAction(action) : NULL);
}

QList<ActionDefinition> ActionsManager::getActions()
{
	return m_definitions.values();
}

bool ActionsManager::registerAction(const QLatin1String &name, const QString &text, const QIcon &icon, ActionIdentifier identifier, ActionScope scope)
{
	if (m_definitions.contains(name))
	{
		return false;
	}

	ActionDefinition definition;
	definition.name = name;
	definition.text = text;
	definition.icon = icon;
	definition.identifier = identifier;
	definition.scope = scope;

	m_definitions[name] = definition;

	return true;
}

}
