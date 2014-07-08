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
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>

namespace Otter
{

ActionsManager* ActionsManager::m_instance = NULL;
QHash<QAction*, QStringList> ActionsManager::m_applicationMacros;
QHash<QObject*, QHash<QString, QAction*> > ActionsManager::m_mainWindowActions;
QHash<QString, QAction*> ActionsManager::m_applicationActions;
QHash<QString, QList<QKeySequence> > ActionsManager::m_profileShortcuts;
QHash<QString, QKeySequence> ActionsManager::m_nativeShortcuts;
QHash<ActionIdentifier, QAction*> ActionsManager::m_windowActions;

ActionsManager::ActionsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
}

void ActionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		loadProfiles();
	}
}

void ActionsManager::createInstance(QObject *parent)
{
	m_instance = new ActionsManager(parent);
}

void ActionsManager::loadProfiles()
{
	QHash<QAction*, QStringList>::iterator macrosIterator;

	for (macrosIterator = m_applicationMacros.begin(); macrosIterator != m_applicationMacros.end(); ++macrosIterator)
	{
		const QString indentifier = macrosIterator.key()->objectName();

		if (m_applicationActions.contains(indentifier))
		{
			m_applicationActions[indentifier]->deleteLater();

			m_applicationActions.remove(indentifier);
		}
	}

	m_applicationMacros.clear();
	m_profileShortcuts.clear();

	QList<QKeySequence> shortcutsInUse;
	const QStringList shortcutsProfiles = SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder")).toStringList();

	for (int i = 0; i < shortcutsProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/keyboard/") + shortcutsProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/keyboard/") + shortcutsProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList actions = profile.childGroups();

		for (int j = 0; j < actions.count(); ++j)
		{
			const QStringList rawShortcuts = profile.value(actions.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut = ((rawShortcuts.at(k) == QLatin1String("native")) ? m_nativeShortcuts.value(actions.at(j), QKeySequence()) : QKeySequence(rawShortcuts.at(k)));

				if (!shortcut.isEmpty() && !shortcutsInUse.contains(shortcut))
				{
					actionShortcuts.append(shortcut);
					shortcutsInUse.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				m_profileShortcuts[actions.at(j)] = actionShortcuts;
			}
		}
	}

	QHash<QString, QAction*>::iterator actionsIterator;

	for (actionsIterator = m_applicationActions.begin(); actionsIterator != m_applicationActions.end(); ++actionsIterator)
	{
		if (m_profileShortcuts.contains(actionsIterator.key()))
		{
			actionsIterator.value()->setShortcuts(m_profileShortcuts[actionsIterator.key()]);
		}
		else
		{
			actionsIterator.value()->setShortcut(QKeySequence());
		}
	}

	const QStringList macrosProfiles = SettingsManager::getValue(QLatin1String("Browser/ActionMacrosProfilesOrder")).toStringList();

	for (int i = 0; i < macrosProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/macros/") + macrosProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/macros/") + macrosProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList macros = profile.childGroups();

		for (int j = 0; j < macros.count(); ++j)
		{
			const QStringList actions = profile.value(macros.at(j) + QLatin1String("/actions"), QString()).toStringList();

			if (m_applicationActions.contains(macros.at(j)) || actions.isEmpty())
			{
				continue;
			}

			QAction *action = new Action(QIcon(), profile.value(macros.at(j) + QLatin1String("/title"), QString()).toString(), SessionsManager::getActiveWindow());
			action->setObjectName(macros.at(j));

			connect(action, SIGNAL(triggered()), m_instance, SLOT(triggerMacro()));

			m_applicationActions[macros.at(j)] = action;
			m_applicationMacros[action] = actions;

			const QStringList rawShortcuts = profile.value(macros.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QList<QKeySequence> actionShortcuts;

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut(rawShortcuts.at(k));

				if (!shortcut.isEmpty() && !shortcutsInUse.contains(shortcut))
				{
					actionShortcuts.append(shortcut);
					shortcutsInUse.append(shortcut);
				}
			}

			if (!actionShortcuts.isEmpty())
			{
				m_profileShortcuts[macros.at(j)] = actionShortcuts;

				action->setShortcuts(actionShortcuts);
				action->setShortcutContext(Qt::ApplicationShortcut);
			}
		}
	}

	QList<QObject*> windows = m_mainWindowActions.keys();

	for (int i = 0; i < windows.count(); ++i)
	{
		setupWindowActions(windows.at(i));
	}
}

void ActionsManager::optionChanged(const QString &option)
{
	if ((option == QLatin1String("Browser/ActionMacrosProfilesOrder") || option == QLatin1String("Browser/KeyboardShortcutsProfilesOrder")) && m_reloadTimer == 0)
	{
		m_reloadTimer = startTimer(50);
	}
}

void ActionsManager::removeWindow(QObject *window)
{
	if (m_mainWindowActions.contains(window))
	{
		m_mainWindowActions.remove(qobject_cast<QWidget*>(window));
	}
}

void ActionsManager::registerAction(const QLatin1String &identifier, const QString &text, const QIcon &icon, ActionIdentifier windowAction)
{
	QAction *action = new Action(icon, text, m_instance);
	action->setObjectName(identifier);
	action->setShortcutContext(Qt::ApplicationShortcut);

	m_applicationActions[identifier] = action;

	if (windowAction != UnknownAction)
	{
		action->setData(windowAction);

		m_windowActions[windowAction] = action;
	}
}

void ActionsManager::registerWindow(QWidget *window, QList<QAction*> actions)
{
	if (!window)
	{
		return;
	}

	window->addActions(actions);

	if (m_profileShortcuts.isEmpty())
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

		registerAction(QLatin1String("OpenLinkInThisTab"), tr("Open"));
		registerAction(QLatin1String("OpenLinkInNewTab"), tr("Open in New Tab"));
		registerAction(QLatin1String("OpenLinkInNewTabBackground"), tr("Open in New Background Tab"));
		registerAction(QLatin1String("OpenLinkInNewWindow"), tr("Open in New Window"));
		registerAction(QLatin1String("OpenLinkInNewWindowBackground"), tr("Open in New Background Window"));
		registerAction(QLatin1String("CopyLinkToClipboard"), tr("Copy Link to Clipboard"));
		registerAction(QLatin1String("OpenFrameInThisTab"), tr("Open"));
		registerAction(QLatin1String("OpenFrameInNewTab"), tr("Open in New Tab"));
		registerAction(QLatin1String("OpenFrameInNewTabBackground"), tr("Open in New Background Tab"));
		registerAction(QLatin1String("CopyFrameLinkToClipboard"), tr("Copy Frame Link to Clipboard"));
		registerAction(QLatin1String("ReloadFrame"), tr("Reload"));
		registerAction(QLatin1String("ReloadImage"), tr("Reload Image"));
		registerAction(QLatin1String("ViewSourceFrame"), tr("View Source"));
		registerAction(QLatin1String("SaveLinkToDisk"), tr("Save Link Target As..."));
		registerAction(QLatin1String("SaveLinkToDownloads"), tr("Save to Downloads"));
		registerAction(QLatin1String("BookmarkLink"), tr("Bookmark Link..."), Utils::getIcon(QLatin1String("bookmark-new")));
		registerAction(QLatin1String("ReloadTime"), tr("Reload Each"));
		registerAction(QLatin1String("CopyAddress"), tr("Copy Address"));
		registerAction(QLatin1String("Validate"), tr("Validate"));
		registerAction(QLatin1String("ContentBlocking"), tr("Content Blocking..."));
		registerAction(QLatin1String("WebsitePreferences"), tr("Website Preferences..."));
		registerAction(QLatin1String("ImageProperties"), tr("Image Properties..."));
		registerAction(QLatin1String("OpenImageInNewTab"), tr("Open Image"));
		registerAction(QLatin1String("SaveImageToDisk"), tr("Save Image..."));
		registerAction(QLatin1String("CopyImageToClipboard"), tr("Copy Image to Clipboard"));
		registerAction(QLatin1String("CopyImageUrlToClipboard"), tr("Copy Image Link to Clipboard"));
		registerAction(QLatin1String("Search"), tr("Search"));
		registerAction(QLatin1String("SearchMenu"), tr("Search Using"));
		registerAction(QLatin1String("OpenSelectionAsLink"), tr("Go to This Address"));
		registerAction(QLatin1String("ClearAll"), tr("Clear All"));
		registerAction(QLatin1String("SpellCheck"), tr("Check Spelling"));
		registerAction(QLatin1String("CreateSearch"), tr("Create Search..."));
		registerAction(QLatin1String("InspectElement"), tr("Inspect Element..."));
		registerAction(QLatin1String("SaveMediaToDisk"), tr("Save Media..."));
		registerAction(QLatin1String("CopyMediaUrlToClipboard"), tr("Copy Media Link to Clipboard"));
		registerAction(QLatin1String("ToggleMediaControls"), tr("Show Controls"));
		registerAction(QLatin1String("ToggleMediaLoop"), tr("Looping"));
		registerAction(QLatin1String("ToggleMediaPlayPause"), tr("Play"));
		registerAction(QLatin1String("ToggleMediaMute"), tr("Mute"));
		registerAction(QLatin1String("QuickFind"), tr("Quick Find"), QIcon(), QuickFindAction);
		registerAction(QLatin1String("ActivateAddressField"), tr("Activate Address Field"), QIcon(), ActivateAddressFieldAction);
		registerAction(QLatin1String("CopyAsPlainText"), tr("Copy as Plain Text"), QIcon(), CopyAsPlainTextAction);
		registerAction(QLatin1String("PasteAndGo"), tr("Paste and Go"), QIcon(), PasteAndGoAction);
		registerAction(QLatin1String("ActivateTabOnLeft"), tr("Go to tab on left"), QIcon(), ActivateTabOnLeftAction);
		registerAction(QLatin1String("ActivateTabOnRight"), tr("Go to tab on right"), QIcon(), ActivateTabOnRightAction);

		loadProfiles();

		SessionsManager::connectActions();

		connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), m_instance, SLOT(optionChanged(QString)));
	}

	if (!m_mainWindowActions.contains(window))
	{
		m_mainWindowActions[window] = QHash<QString, QAction*>();

		connect(window, SIGNAL(destroyed(QObject*)), m_instance, SLOT(removeWindow(QObject*)));
	}

	for (int i = 0; i < actions.count(); ++i)
	{
		if (actions.at(i) && !actions.at(i)->isSeparator() && !actions.at(i)->objectName().isEmpty())
		{
			m_mainWindowActions[window][actions.at(i)->objectName().startsWith(QLatin1String("action")) ? actions.at(i)->objectName().mid(6) : actions.at(i)->objectName()] = actions.at(i);

			actions.at(i)->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		}
	}

	setupWindowActions(window);
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

void ActionsManager::triggerMacro()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		const QStringList actions = m_applicationMacros.value(action, QStringList());

		for (int i = 0; i < actions.count(); ++i)
		{
			triggerAction(actions.at(i));
		}
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

void ActionsManager::setupWindowActions(QObject *window)
{
	if (!m_mainWindowActions.contains(window))
	{
		return;
	}

	const QHash<QString, QAction*> actions = m_mainWindowActions[window];
	QHash<QString, QAction*>::const_iterator windowActionsIterator;
	QWidget *widget = qobject_cast<QWidget*>(window);

	for (windowActionsIterator = actions.constBegin(); windowActionsIterator != actions.constEnd(); ++windowActionsIterator)
	{
		if (m_profileShortcuts.contains(windowActionsIterator.key()))
		{
			windowActionsIterator.value()->setShortcuts(m_profileShortcuts[windowActionsIterator.key()]);
		}
		else
		{
			windowActionsIterator.value()->setShortcut(QKeySequence());
		}
	}

	widget->addActions(m_applicationActions.values());
}

QAction* ActionsManager::getAction(const QString &action)
{
	if (m_applicationActions.contains(action))
	{
		return m_applicationActions[action];
	}

	QWidget *window = SessionsManager::getActiveWindow();

	if (m_mainWindowActions.contains(window) && m_mainWindowActions[window].contains(action))
	{
		return m_mainWindowActions[window][action];
	}

	return NULL;
}

QAction *ActionsManager::getAction(ActionIdentifier action)
{
	return (m_windowActions.contains(action) ? m_windowActions[action] : NULL);
}

QKeySequence ActionsManager::getNativeShortcut(const QString &action)
{
	return m_nativeShortcuts.value(action, QKeySequence());
}

QStringList ActionsManager::getIdentifiers()
{
	QStringList identifiers = m_applicationActions.keys();

	if (m_mainWindowActions.contains(SessionsManager::getActiveWindow()))
	{
		identifiers << m_mainWindowActions[SessionsManager::getActiveWindow()].keys();
	}
	else
	{
		identifiers << m_profileShortcuts.keys();
	}

	return identifiers;
}

}
