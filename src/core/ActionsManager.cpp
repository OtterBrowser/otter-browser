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
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "File"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Sessions"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Import and Export"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Edit"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "View"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "User Agent"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Character Encoding"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "History"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Closed Windows"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Bookmarks"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tools"));
		Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Help"));

		ShortcutsManager::createInstance(Application::getInstance());

		registerAction(QLatin1String("NewTab"), QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), Utils::getIcon(QLatin1String("tab-new")));
		registerAction(QLatin1String("NewTabPrivate"), QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), Utils::getIcon(QLatin1String("tab-new-private")));
		registerAction(QLatin1String("NewWindow"), QT_TRANSLATE_NOOP("actions", "New Window"), QString(), Utils::getIcon(QLatin1String("window-new")));
		registerAction(QLatin1String("NewWindowPrivate"), QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), Utils::getIcon(QLatin1String("window-new-private")));
		registerAction(QLatin1String("Open"), QT_TRANSLATE_NOOP("actions", "Open..."), QString(), Utils::getIcon(QLatin1String("document-open")));
		registerAction(QLatin1String("Save"), QT_TRANSLATE_NOOP("actions", "Save..."), QString(), Utils::getIcon(QLatin1String("document-save")), false);
		registerAction(QLatin1String("CloneTab"), QT_TRANSLATE_NOOP("actions", "Clone Tab"));
		registerAction(QLatin1String("CloseTab"), QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), Utils::getIcon(QLatin1String("tab-close")));
		registerAction(QLatin1String("SaveSession"), QT_TRANSLATE_NOOP("actions", "Save Current Session..."));
		registerAction(QLatin1String("ManageSessions"), QT_TRANSLATE_NOOP("actions", "Manage Sessions..."));
		registerAction(QLatin1String("Print"), QT_TRANSLATE_NOOP("actions", "Print..."), QString(), Utils::getIcon(QLatin1String("document-print")));
		registerAction(QLatin1String("PrintPreview"), QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), Utils::getIcon(QLatin1String("document-print-preview")));
		registerAction(QLatin1String("WorkOffline"), QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), true, true, false);
		registerAction(QLatin1String("ShowMenuBar"), QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), true, true, true);
		registerAction(QLatin1String("Exit"), QT_TRANSLATE_NOOP("actions", "Exit"), QString(), Utils::getIcon(QLatin1String("application-exit")));
		registerAction(QLatin1String("Undo"), QT_TRANSLATE_NOOP("actions", "Undo"), QString(), Utils::getIcon(QLatin1String("edit-undo")), true, false, false, UndoAction);
		registerAction(QLatin1String("Redo"), QT_TRANSLATE_NOOP("actions", "Redo"), QString(), Utils::getIcon(QLatin1String("edit-redo")), true, false, false, RedoAction);
		registerAction(QLatin1String("Cut"), QT_TRANSLATE_NOOP("actions", "Cut"), QString(), Utils::getIcon(QLatin1String("edit-cut")), true, false, false, CutAction);
		registerAction(QLatin1String("Copy"), QT_TRANSLATE_NOOP("actions", "Copy"), QString(), Utils::getIcon(QLatin1String("edit-copy")), true, false, false, CopyAction);
		registerAction(QLatin1String("Paste"), QT_TRANSLATE_NOOP("actions", "Paste"), QString(), Utils::getIcon(QLatin1String("edit-paste")), true, false, false, PasteAction);
		registerAction(QLatin1String("Delete"), QT_TRANSLATE_NOOP("actions", "Delete"), QString(), Utils::getIcon(QLatin1String("edit-delete")), true, false, false, DeleteAction);
		registerAction(QLatin1String("SelectAll"), QT_TRANSLATE_NOOP("actions", "Select All"), QString(), Utils::getIcon(QLatin1String("edit-select-all")), true, false, false, SelectAllAction);
		registerAction(QLatin1String("Find"), QT_TRANSLATE_NOOP("actions", "Find..."), QString(), Utils::getIcon(QLatin1String("edit-find")), true, false, false, FindAction);
		registerAction(QLatin1String("FindNext"), QT_TRANSLATE_NOOP("actions", "Find Next"), QString(), QIcon(), true, false, false, FindNextAction);
		registerAction(QLatin1String("FindPrevious"), QT_TRANSLATE_NOOP("actions", "Find Previous"), QString(), QIcon(), true, false, false, FindPreviousAction);
		registerAction(QLatin1String("Reload"), QT_TRANSLATE_NOOP("actions", "Reload"), QString(), Utils::getIcon(QLatin1String("view-refresh")), true, false, false, ReloadAction);
		registerAction(QLatin1String("Stop"), QT_TRANSLATE_NOOP("actions", "Stop"), QString(), Utils::getIcon(QLatin1String("process-stop")), true, false, false, StopAction);
		registerAction(QLatin1String("ZoomIn"), QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), Utils::getIcon(QLatin1String("zoom-in")), true, false, false, ZoomInAction);
		registerAction(QLatin1String("ZoomOut"), QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), Utils::getIcon(QLatin1String("zoom-out")), true, false, false, ZoomOutAction);
		registerAction(QLatin1String("ZoomOriginal"), QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), Utils::getIcon(QLatin1String("zoom-original")), true, false, false, ZoomOriginalAction);
		registerAction(QLatin1String("FullScreen"), QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), Utils::getIcon(QLatin1String("view-fullscreen")));
		registerAction(QLatin1String("ViewSource"), QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), false, false, false, ViewSourceAction);
		registerAction(QLatin1String("InspectPage"), QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), true, true, false, InspectPageAction);
		registerAction(QLatin1String("Sidebar"), QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), QIcon(), true, true, false);
		registerAction(QLatin1String("GoBack"), QT_TRANSLATE_NOOP("actions", "Back"), QString(), Utils::getIcon(QLatin1String("go-previous")), true, false, false, GoBackAction);
		registerAction(QLatin1String("GoForward"), QT_TRANSLATE_NOOP("actions", "Forward"), QString(), Utils::getIcon(QLatin1String("go-next")), true, false, false, GoForwardAction);
		registerAction(QLatin1String("GoToParentDirectory"), QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"), QString(), QIcon(), true, false, false, GoToParentDirectoryAction);
		registerAction(QLatin1String("Rewind"), QT_TRANSLATE_NOOP("actions", "Rewind"), QString(), Utils::getIcon(QLatin1String("go-first")), true, false, false, RewindAction);
		registerAction(QLatin1String("FastForward"), QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), Utils::getIcon(QLatin1String("go-last")), true, false, false, FastForwardAction);
		registerAction(QLatin1String("ViewHistory"), QT_TRANSLATE_NOOP("actions", "View History"), QString(), Utils::getIcon(QLatin1String("view-history")));
		registerAction(QLatin1String("ClearHistory"), QT_TRANSLATE_NOOP("actions", "Clear History..."), QString(), Utils::getIcon(QLatin1String("edit-clear-history")));
		registerAction(QLatin1String("AddBookmark"), QT_TRANSLATE_NOOP("actions", "Add Bookmark..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ManageBookmarks"), QT_TRANSLATE_NOOP("actions", "Manage Bookmarks..."), QString(), Utils::getIcon(QLatin1String("bookmarks-organize")));
		registerAction(QLatin1String("Transfers"), QT_TRANSLATE_NOOP("actions", "Transfers..."));
		registerAction(QLatin1String("Cookies"), QT_TRANSLATE_NOOP("actions", "Cookies..."));
		registerAction(QLatin1String("ContentBlocking"), QT_TRANSLATE_NOOP("actions", "Content Blocking..."));
		registerAction(QLatin1String("ErrorConsole"), QT_TRANSLATE_NOOP("actions", "Error Console"), QString(), QIcon(), true, true, false);
		registerAction(QLatin1String("Preferences"), QT_TRANSLATE_NOOP("actions", "Preferences..."));
		registerAction(QLatin1String("SwitchApplicationLanguage"), QT_TRANSLATE_NOOP("actions", "Switch Application Language..."), QString(), Utils::getIcon(QLatin1String("preferences-desktop-locale")));
		registerAction(QLatin1String("AboutApplication"), QT_TRANSLATE_NOOP("actions", "About Otter..."), QString(), Utils::getIcon(QLatin1String("otter-browser"), false));
		registerAction(QLatin1String("AboutQt"), QT_TRANSLATE_NOOP("actions", "About Qt..."), QString(), Utils::getIcon(QLatin1String("qt"), false));
		registerAction(QLatin1String("CloseWindow"), QT_TRANSLATE_NOOP("actions", "Close Window"));
		registerAction(QLatin1String("OpenLinkTab"), QT_TRANSLATE_NOOP("actions", "Open"));
		registerAction(QLatin1String("OpenLinkInThisTab"), QT_TRANSLATE_NOOP("actions", "Open in This Tab"));
		registerAction(QLatin1String("OpenLinkInNewTab"), QT_TRANSLATE_NOOP("actions", "Open in New Tab"));
		registerAction(QLatin1String("OpenLinkInNewTabBackground"), QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"));
		registerAction(QLatin1String("OpenLinkInNewWindow"), QT_TRANSLATE_NOOP("actions", "Open in New Window"));
		registerAction(QLatin1String("OpenLinkInNewWindowBackground"), QT_TRANSLATE_NOOP("actions", "Open in New Background Window"));
		registerAction(QLatin1String("ReopenTab"), QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"));
		registerAction(QLatin1String("CopyLinkToClipboard"), QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"));
		registerAction(QLatin1String("OpenFrameInThisTab"), QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"));
		registerAction(QLatin1String("OpenFrameInNewTab"), QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"));
		registerAction(QLatin1String("OpenFrameInNewTabBackground"), QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"));
		registerAction(QLatin1String("CopyFrameLinkToClipboard"), QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"));
		registerAction(QLatin1String("ReloadFrame"), QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"));
		registerAction(QLatin1String("ReloadImage"), QT_TRANSLATE_NOOP("actions", "Reload Image"));
		registerAction(QLatin1String("ViewSourceFrame"), QT_TRANSLATE_NOOP("actions", "View Source"));
		registerAction(QLatin1String("SaveLinkToDisk"), QT_TRANSLATE_NOOP("actions", "Save Link Target As..."));
		registerAction(QLatin1String("SaveLinkToDownloads"), QT_TRANSLATE_NOOP("actions", "Save to Downloads"));
		registerAction(QLatin1String("BookmarkLink"), QT_TRANSLATE_NOOP("actions", "Bookmark Link..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ReloadTime"), QT_TRANSLATE_NOOP("actions", "Reload Every"));
		registerAction(QLatin1String("CopyAddress"), QT_TRANSLATE_NOOP("actions", "Copy Address"));
		registerAction(QLatin1String("Validate"), QT_TRANSLATE_NOOP("actions", "Validate"));
		registerAction(QLatin1String("WebsitePreferences"), QT_TRANSLATE_NOOP("actions", "Website Preferences..."), QString(), QIcon(), true, false, false, WebsitePreferencesAction);
		registerAction(QLatin1String("ImageProperties"), QT_TRANSLATE_NOOP("actions", "Image Properties..."));
		registerAction(QLatin1String("OpenImageInNewTab"), QT_TRANSLATE_NOOP("actions", "Open Image"));
		registerAction(QLatin1String("SaveImageToDisk"), QT_TRANSLATE_NOOP("actions", "Save Image..."));
		registerAction(QLatin1String("CopyImageToClipboard"), QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"));
		registerAction(QLatin1String("CopyImageUrlToClipboard"), QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"));
		registerAction(QLatin1String("Search"), QT_TRANSLATE_NOOP("actions", "Search"));
		registerAction(QLatin1String("SearchMenu"), QT_TRANSLATE_NOOP("actions", "Search Using"));
		registerAction(QLatin1String("OpenSelectionAsLink"), QT_TRANSLATE_NOOP("actions", "Go to This Address"));
		registerAction(QLatin1String("ClearAll"), QT_TRANSLATE_NOOP("actions", "Clear All"));
		registerAction(QLatin1String("SpellCheck"), QT_TRANSLATE_NOOP("actions", "Check Spelling"));
		registerAction(QLatin1String("CreateSearch"), QT_TRANSLATE_NOOP("actions", "Create Search..."));
		registerAction(QLatin1String("InspectElement"), QT_TRANSLATE_NOOP("actions", "Inspect Element..."));
		registerAction(QLatin1String("SaveMediaToDisk"), QT_TRANSLATE_NOOP("actions", "Save Media..."));
		registerAction(QLatin1String("CopyMediaUrlToClipboard"), QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"));
		registerAction(QLatin1String("ToggleMediaControls"), QT_TRANSLATE_NOOP("actions", "Show Controls"));
		registerAction(QLatin1String("ToggleMediaLoop"), QT_TRANSLATE_NOOP("actions", "Looping"));
		registerAction(QLatin1String("ToggleMediaPlayPause"), QT_TRANSLATE_NOOP("actions", "Play"));
		registerAction(QLatin1String("ToggleMediaMute"), QT_TRANSLATE_NOOP("actions", "Mute"));
		registerAction(QLatin1String("GoToPage"), QT_TRANSLATE_NOOP("actions", "Go to Page or Search"));
		registerAction(QLatin1String("QuickBookmarkAccess"), QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"));
		registerAction(QLatin1String("QuickFind"), QT_TRANSLATE_NOOP("actions", "Quick Find"), QString(), QIcon(), true, false, false, QuickFindAction);
		registerAction(QLatin1String("ActivateAddressField"), QT_TRANSLATE_NOOP("actions", "Activate Address Field"), QString(), QIcon(), true, false, false, ActivateAddressFieldAction);
		registerAction(QLatin1String("CopyAsPlainText"), QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"), QString(), QIcon(), true, false, false, CopyAsPlainTextAction);
		registerAction(QLatin1String("PasteAndGo"), QT_TRANSLATE_NOOP("actions", "Paste and Go"), QString(), QIcon(), true, false, false, PasteAndGoAction);
		registerAction(QLatin1String("ActivateTabOnLeft"), QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"), QString(), QIcon(), true, false, false, ActivateTabOnLeftAction);
		registerAction(QLatin1String("ActivateTabOnRight"), QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"), QString(), QIcon(), true, false, false, ActivateTabOnRightAction);
		registerAction(QLatin1String("ScrollToStart"), QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"), QString(), QIcon(), true, false, false, ScrollToStartAction);
		registerAction(QLatin1String("ScrollToEnd"), QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"), QString(), QIcon(), true, false, false, ScrollToEndAction);
		registerAction(QLatin1String("ScrollPageUp"), QT_TRANSLATE_NOOP("actions", "Page Up"), QString(), QIcon(), true, false, false, ScrollPageUpAction);
		registerAction(QLatin1String("ScrollPageDown"), QT_TRANSLATE_NOOP("actions", "Page Down"), QString(), QIcon(), true, false, false, ScrollPageDownAction);
		registerAction(QLatin1String("ScrollPageLeft"), QT_TRANSLATE_NOOP("actions", "Page Left"), QString(), QIcon(), true, false, false, ScrollPageLeftAction);
		registerAction(QLatin1String("ScrollPageRight"), QT_TRANSLATE_NOOP("actions", "Page Right"), QString(), QIcon(), true, false, false, ScrollPageRightAction);
		registerAction(QLatin1String("QuickPreferences"), QT_TRANSLATE_NOOP("actions", "Quick Preferences"), QString(), QIcon(), true, false, false, QuickPreferencesAction);
		registerAction(QLatin1String("LoadPlugins"), QT_TRANSLATE_NOOP("actions", "Load Plugins"), QString(), Utils::getIcon(QLatin1String("preferences-plugin")), true, false, false, LoadPluginsAction);
	}

	QHash<QString, ActionDefinition>::const_iterator definitionsIterator;

	for (definitionsIterator = m_definitions.constBegin(); definitionsIterator != m_definitions.constEnd(); ++definitionsIterator)
	{
		Action *action =  new Action(definitionsIterator.value().icon, definitionsIterator.value().text, m_window);
		action->setObjectName(definitionsIterator.key());
		action->setScope(definitionsIterator.value().scope);
		action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		action->setEnabled(definitionsIterator.value().isEnabled);
		action->setCheckable(definitionsIterator.value().isCheckable);
		action->setChecked(definitionsIterator.value().isChecked);

		if (definitionsIterator.value().identifier != UnknownAction)
		{
			action->setIdentifier(definitionsIterator.value().identifier);
			action->setData(definitionsIterator.value().identifier);

			m_standardActions[definitionsIterator.value().identifier] = action;

			connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));
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

void ActionsManager::triggerAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		m_window->getWindowsManager()->triggerAction(static_cast<ActionIdentifier>(action->data().toInt()), action->isChecked());
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
	MainWindow *window = MainWindow::findMainWindow(parent);

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

QList<ActionDefinition> ActionsManager::getActionDefinitions()
{
	return m_definitions.values();
}

ActionDefinition ActionsManager::getActionDefinition(const QString &action)
{
	return m_definitions[action];
}

bool ActionsManager::registerAction(const QLatin1String &name, const QString &text, const QString &description , const QIcon &icon, bool isEnabled, bool isCheckable, bool isChecked, ActionIdentifier identifier, ActionScope scope)
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
	definition.isEnabled = isEnabled;
	definition.isCheckable = isCheckable;
	definition.isChecked = isChecked;

	m_definitions[name] = definition;

	return true;
}

}
