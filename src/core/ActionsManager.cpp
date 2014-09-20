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
#include "Application.h"
#include "ShortcutsManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>

namespace Otter
{

QHash<QString, ActionDefinition> ActionsManager::m_definitions;

ActionsManager::ActionsManager(MainWindow *parent) : QObject(parent),
	m_window(parent)
{
	if (m_definitions.isEmpty())
	{
		ShortcutsManager::createInstance(Application::getInstance());

		registerAction(QLatin1String("NewTab"), QT_TR_NOOP("New Tab"), QString(), Utils::getIcon(QLatin1String("tab-new")));
		registerAction(QLatin1String("NewTabPrivate"), QT_TR_NOOP("New Private Tab"));
		registerAction(QLatin1String("NewWindow"), QT_TR_NOOP("New Window"), QString(), Utils::getIcon(QLatin1String("window-new")));
		registerAction(QLatin1String("NewWindowPrivate"), QT_TR_NOOP("New Private Window"));
		registerAction(QLatin1String("Open"), QT_TR_NOOP("Open..."), QString(), Utils::getIcon(QLatin1String("document-open")));
		registerAction(QLatin1String("Save"), QT_TR_NOOP("Save..."), QString(), Utils::getIcon(QLatin1String("document-save")));
		registerAction(QLatin1String("CloseTab"), QT_TR_NOOP("Close Tab"), QString(), Utils::getIcon(QLatin1String("tab-close")));
		registerAction(QLatin1String("Print"), QT_TR_NOOP("Print..."), QString(), Utils::getIcon(QLatin1String("document-print")));
		registerAction(QLatin1String("PrintPreview"), QT_TR_NOOP("Print Preview"), QString(), Utils::getIcon(QLatin1String("document-print-preview")));
		registerAction(QLatin1String("WorkOffline"), QT_TR_NOOP("Work Offline"));
		registerAction(QLatin1String("ShowMenubar"), QT_TR_NOOP("Show Menubar"));
		registerAction(QLatin1String("Exit"), QT_TR_NOOP("Exit"), QString(), Utils::getIcon(QLatin1String("application-exit")));
		registerAction(QLatin1String("Undo"), QT_TR_NOOP("Undo"), QString(), Utils::getIcon(QLatin1String("edit-undo")));
		registerAction(QLatin1String("Redo"), QT_TR_NOOP("Redo"), QString(), Utils::getIcon(QLatin1String("edit-redo")));
		registerAction(QLatin1String("Cut"), QT_TR_NOOP("Cut"), QString(), Utils::getIcon(QLatin1String("edit-cut")));
		registerAction(QLatin1String("Copy"), QT_TR_NOOP("Copy"), QString(), Utils::getIcon(QLatin1String("edit-copy")));
		registerAction(QLatin1String("Paste"), QT_TR_NOOP("Paste"), QString(), Utils::getIcon(QLatin1String("edit-paste")));
		registerAction(QLatin1String("Delete"), QT_TR_NOOP("Delete"), QString(), Utils::getIcon(QLatin1String("edit-delete")));
		registerAction(QLatin1String("SelectAll"), QT_TR_NOOP("Select All"), QString(), Utils::getIcon(QLatin1String("edit-select-all")));
		registerAction(QLatin1String("Find"), QT_TR_NOOP("Find..."), QString(), Utils::getIcon(QLatin1String("edit-find")));
		registerAction(QLatin1String("FindNext"), QT_TR_NOOP("Find Next"));
		registerAction(QLatin1String("FindPrevious"), QT_TR_NOOP("Find Previous"));
		registerAction(QLatin1String("Reload"), QT_TR_NOOP("Reload"), QString(), Utils::getIcon(QLatin1String("view-refresh")));
		registerAction(QLatin1String("Stop"), QT_TR_NOOP("Stop"), QString(), Utils::getIcon(QLatin1String("process-stop")));
		registerAction(QLatin1String("ZoomIn"), QT_TR_NOOP("Zoom In"), QString(), Utils::getIcon(QLatin1String("zoom-in")));
		registerAction(QLatin1String("ZoomOut"), QT_TR_NOOP("Zoom Out"), QString(), Utils::getIcon(QLatin1String("zoom-out")));
		registerAction(QLatin1String("ZoomOriginal"), QT_TR_NOOP("Zoom Original"), QString(), Utils::getIcon(QLatin1String("zoom-original")));
		registerAction(QLatin1String("FullScreen"), QT_TR_NOOP("Full Screen"), QString(), Utils::getIcon(QLatin1String("view-fullscreen")));
		registerAction(QLatin1String("ViewSource"), QT_TR_NOOP("View Source"));
		registerAction(QLatin1String("InspectPage"), QT_TR_NOOP("Inspect Page"));
		registerAction(QLatin1String("GoBack"), QT_TR_NOOP("Back"), QString(), Utils::getIcon(QLatin1String("go-previous")));
		registerAction(QLatin1String("GoForward"), QT_TR_NOOP("Forward"), QString(), Utils::getIcon(QLatin1String("go-next")));
		registerAction(QLatin1String("Rewind"), QT_TR_NOOP("Rewind"), QString(), Utils::getIcon(QLatin1String("go-first")));
		registerAction(QLatin1String("FastForward"), QT_TR_NOOP("Fast Forward"), QString(), Utils::getIcon(QLatin1String("go-last")));
		registerAction(QLatin1String("ViewHistory"), QT_TR_NOOP("View History"), QString(), Utils::getIcon(QLatin1String("view-history")));
		registerAction(QLatin1String("ClearHistory"), QT_TR_NOOP("Clear History..."), QString(), Utils::getIcon(QLatin1String("edit-clear-history")));
		registerAction(QLatin1String("AddBookmark"), QT_TR_NOOP("Add Bookmark..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ManageBookmarks"), QT_TR_NOOP("Manage Bookmarks..."), QString(), Utils::getIcon(QLatin1String("bookmarks-organize")));
		registerAction(QLatin1String("Transfers"), QT_TR_NOOP("Transfers..."));
		registerAction(QLatin1String("Cookies"), QT_TR_NOOP("Cookies..."));
		registerAction(QLatin1String("ContentBlocking"), QT_TR_NOOP("Content Blocking..."));
		registerAction(QLatin1String("ErrorConsole"), QT_TR_NOOP("Error Console"));
		registerAction(QLatin1String("Sidebar"), QT_TR_NOOP("Sidebar"));
		registerAction(QLatin1String("Preferences"), QT_TR_NOOP("Preferences..."));
		registerAction(QLatin1String("SwitchApplicationLanguage"), QT_TR_NOOP("Switch Application Language..."));
		registerAction(QLatin1String("AboutApplication"), QT_TR_NOOP("About Otter..."), QString(), Utils::getIcon(QLatin1String("otter-browser")));
		registerAction(QLatin1String("AboutQt"), QT_TR_NOOP("About Qt..."));
		registerAction(QLatin1String("CloseWindow"), QT_TR_NOOP("Close Window"));
		registerAction(QLatin1String("OpenLinkTab"), QT_TR_NOOP("Open"));
		registerAction(QLatin1String("OpenLinkInThisTab"), QT_TR_NOOP("Open in This Tab"));
		registerAction(QLatin1String("OpenLinkInNewTab"), QT_TR_NOOP("Open in New Tab"));
		registerAction(QLatin1String("OpenLinkInNewTabBackground"), QT_TR_NOOP("Open in New Background Tab"));
		registerAction(QLatin1String("OpenLinkInNewWindow"), QT_TR_NOOP("Open in New Window"));
		registerAction(QLatin1String("OpenLinkInNewWindowBackground"), QT_TR_NOOP("Open in New Background Window"));
		registerAction(QLatin1String("ReopenTab"), QT_TR_NOOP("Reopen Previously Closed Tab"));
		registerAction(QLatin1String("CopyLinkToClipboard"), QT_TR_NOOP("Copy Link to Clipboard"));
		registerAction(QLatin1String("OpenFrameInThisTab"), QT_TR_NOOP("Open"), QT_TR_NOOP("Open Frame in This Tab"));
		registerAction(QLatin1String("OpenFrameInNewTab"), QT_TR_NOOP("Open in New Tab"), QT_TR_NOOP("Open Frame in New Tab"));
		registerAction(QLatin1String("OpenFrameInNewTabBackground"), QT_TR_NOOP("Open in New Background Tab"), QT_TR_NOOP("Open Frame in New Background Tab"));
		registerAction(QLatin1String("CopyFrameLinkToClipboard"), QT_TR_NOOP("Copy Frame Link to Clipboard"));
		registerAction(QLatin1String("ReloadFrame"), QT_TR_NOOP("Reload"), QT_TR_NOOP("Reload Frame"));
		registerAction(QLatin1String("ReloadImage"), QT_TR_NOOP("Reload Image"));
		registerAction(QLatin1String("ViewSourceFrame"), QT_TR_NOOP("View Source"));
		registerAction(QLatin1String("SaveLinkToDisk"), QT_TR_NOOP("Save Link Target As..."));
		registerAction(QLatin1String("SaveLinkToDownloads"), QT_TR_NOOP("Save to Downloads"));
		registerAction(QLatin1String("BookmarkLink"), QT_TR_NOOP("Bookmark Link..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ReloadTime"), QT_TR_NOOP("Reload Each"));
		registerAction(QLatin1String("CopyAddress"), QT_TR_NOOP("Copy Address"));
		registerAction(QLatin1String("Validate"), QT_TR_NOOP("Validate"));
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
		registerAction(QLatin1String("QuickBookmarkAccess"), QT_TR_NOOP("Quick Bookmark Access"));
		registerAction(QLatin1String("QuickFind"), QT_TR_NOOP("Quick Find"), QString(), QIcon(), QuickFindAction);
		registerAction(QLatin1String("ActivateAddressField"), QT_TR_NOOP("Activate Address Field"), QString(), QIcon(), ActivateAddressFieldAction);
		registerAction(QLatin1String("CopyAsPlainText"), QT_TR_NOOP("Copy as Plain Text"), QString(), QIcon(), CopyAsPlainTextAction);
		registerAction(QLatin1String("PasteAndGo"), QT_TR_NOOP("Paste and Go"), QString(), QIcon(), PasteAndGoAction);
		registerAction(QLatin1String("ActivateTabOnLeft"), QT_TR_NOOP("Go to Tab on Left"), QString(), QIcon(), ActivateTabOnLeftAction);
		registerAction(QLatin1String("ActivateTabOnRight"), QT_TR_NOOP("Go to Tab on Right"), QString(), QIcon(), ActivateTabOnRightAction);
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

	while (parent)
	{
		if (parent->metaObject()->className() == QLatin1String("Otter::MainWindow"))
		{
			window = qobject_cast<MainWindow*>(parent);

			break;
		}

		parent = parent->parent();
	}

	if (!window)
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

bool ActionsManager::registerAction(const QLatin1String &name, const QString &text, const QString &description , const QIcon &icon, ActionIdentifier identifier, ActionScope scope)
{
	if (m_definitions.contains(name))
	{
		return false;
	}

	ActionDefinition definition;
	definition.name = name;
	definition.text = text;
	definition.description = description;
	definition.icon = icon;
	definition.identifier = identifier;
	definition.scope = scope;

	m_definitions[name] = definition;

	return true;
}

}
