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
#include "GesturesManager.h"
#include "Utils.h"
#include "../ui/ContentsWidget.h"
#include "../ui/MainWindow.h"
#include "../ui/Window.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>

namespace Otter
{

ActionsManagerHelper* ActionsManager::m_helper = NULL;
Action* ActionsManager::m_dummyAction = NULL;
QMutex ActionsManager::m_mutex;

ActionsManagerHelper::ActionsManagerHelper(QObject *parent) : QObject(parent),
	reloadShortcutsTimer(0)
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
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Tabs and Windows"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Page"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Print"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Settings"));

	actionDefinitions.reserve(Action::OtherAction);

	registerAction(Action::NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), Utils::getIcon(QLatin1String("tab-new")));
	registerAction(Action::NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), Utils::getIcon(QLatin1String("tab-new-private")));
	registerAction(Action::NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), QString(), Utils::getIcon(QLatin1String("window-new")));
	registerAction(Action::NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), Utils::getIcon(QLatin1String("window-new-private")));
	registerAction(Action::OpenAction, QT_TRANSLATE_NOOP("actions", "Open..."), QString(), Utils::getIcon(QLatin1String("document-open")));
	registerAction(Action::SaveAction, QT_TRANSLATE_NOOP("actions", "Save..."), QString(), Utils::getIcon(QLatin1String("document-save")), false);
	registerAction(Action::CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"));
	registerAction(Action::PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"));
	registerAction(Action::DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"));
	registerAction(Action::CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), Utils::getIcon(QLatin1String("tab-close")));
	registerAction(Action::CloseOtherTabs, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), QString(), Utils::getIcon(QLatin1String("tab-close-other")));
	registerAction(Action::ReopenTabAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"));
	registerAction(Action::CloseWindowAction, QT_TRANSLATE_NOOP("actions", "Close Window"));
	registerAction(Action::SessionsAction, QT_TRANSLATE_NOOP("actions", "Manage Sessions..."));
	registerAction(Action::SaveSessionAction, QT_TRANSLATE_NOOP("actions", "Save Current Session..."));
	registerAction(Action::OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), QString(), Utils::getIcon(QLatin1String("document-open")));
	registerAction(Action::OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"));
	registerAction(Action::OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"));
	registerAction(Action::OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"));
	registerAction(Action::OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"));
	registerAction(Action::OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"));
	registerAction(Action::CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"));
	registerAction(Action::BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
	registerAction(Action::SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As..."));
	registerAction(Action::SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"));
	registerAction(Action::OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"));
	registerAction(Action::OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"));
	registerAction(Action::OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"));
	registerAction(Action::OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"));
	registerAction(Action::CopyFrameLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"));
	registerAction(Action::ReloadFrameAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"));
	registerAction(Action::ViewFrameSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"));
	registerAction(Action::OpenImageInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open Image"));
	registerAction(Action::SaveImageToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Image..."));
	registerAction(Action::CopyImageToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"));
	registerAction(Action::CopyImageUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"));
	registerAction(Action::ReloadImageAction, QT_TRANSLATE_NOOP("actions", "Reload Image"));
	registerAction(Action::ImagePropertiesAction, QT_TRANSLATE_NOOP("actions", "Image Properties..."));
	registerAction(Action::SaveMediaToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Media..."));
	registerAction(Action::CopyMediaUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"));
	registerAction(Action::ToggleMediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"));
	registerAction(Action::ToggleMediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"));
	registerAction(Action::ToggleMediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"));
	registerAction(Action::ToggleMediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"));
	registerAction(Action::GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QString(), Utils::getIcon(QLatin1String("go-jump-locationbar")));
	registerAction(Action::GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QString(), Utils::getIcon(QLatin1String("go-previous")));
	registerAction(Action::GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QString(), Utils::getIcon(QLatin1String("go-next")));
	registerAction(Action::GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"));
	registerAction(Action::GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), QString(), Utils::getIcon(QLatin1String("go-home")));
	registerAction(Action::GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"));
	registerAction(Action::RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QString(), Utils::getIcon(QLatin1String("go-first")));
	registerAction(Action::FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), Utils::getIcon(QLatin1String("go-last")));
	registerAction(Action::StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), QString(), Utils::getIcon(QLatin1String("process-stop")));
	registerAction(Action::StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"));
	registerAction(Action::ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), QString(), Utils::getIcon(QLatin1String("view-refresh")));
	registerAction(Action::ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), Utils::getIcon(QLatin1String("view-refresh")));
	registerAction(Action::ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"));
	registerAction(Action::ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Reload Every"));
	registerAction(Action::UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), QString(), Utils::getIcon(QLatin1String("edit-undo")));
	registerAction(Action::RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), QString(), Utils::getIcon(QLatin1String("edit-redo")));
	registerAction(Action::CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), QString(), Utils::getIcon(QLatin1String("edit-cut")));
	registerAction(Action::CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), QString(), Utils::getIcon(QLatin1String("edit-copy")));
	registerAction(Action::CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"));
	registerAction(Action::CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"));
	registerAction(Action::PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), QString(), Utils::getIcon(QLatin1String("edit-paste")));
	registerAction(Action::PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"));
	registerAction(Action::DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), QString(), Utils::getIcon(QLatin1String("edit-delete")));
	registerAction(Action::SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), QString(), Utils::getIcon(QLatin1String("edit-select-all")));
	registerAction(Action::ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"));
	registerAction(Action::CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"));
	registerAction(Action::FindAction, QT_TRANSLATE_NOOP("actions", "Find..."), QString(), Utils::getIcon(QLatin1String("edit-find")));
	registerAction(Action::FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"));
	registerAction(Action::FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"));
	registerAction(Action::QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"));
	registerAction(Action::SearchAction, QT_TRANSLATE_NOOP("actions", "Search"));
	registerAction(Action::SearchMenuAction, QT_TRANSLATE_NOOP("actions", "Search Using"));
	registerAction(Action::CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search..."));
	registerAction(Action::ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), Utils::getIcon(QLatin1String("zoom-in")));
	registerAction(Action::ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), Utils::getIcon(QLatin1String("zoom-out")));
	registerAction(Action::ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), Utils::getIcon(QLatin1String("zoom-original")));
	registerAction(Action::ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"));
	registerAction(Action::ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"));
	registerAction(Action::ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"));
	registerAction(Action::ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"));
	registerAction(Action::ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"));
	registerAction(Action::ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"));
	registerAction(Action::PrintAction, QT_TRANSLATE_NOOP("actions", "Print..."), QString(), Utils::getIcon(QLatin1String("document-print")));
	registerAction(Action::PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), Utils::getIcon(QLatin1String("document-print-preview")));
	registerAction(Action::ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"));
	registerAction(Action::ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"));
	registerAction(Action::ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"));
	registerAction(Action::ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"));
	registerAction(Action::ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"));
	registerAction(Action::BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks..."), QString(), Utils::getIcon(QLatin1String("bookmarks-organize")));
	registerAction(Action::AddBookmarkAction, QT_TRANSLATE_NOOP("actions", "Add Bookmark..."), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
	registerAction(Action::QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"));
	registerAction(Action::PopupsPolicyAction, QT_TRANSLATE_NOOP("actions", "Block pop-ups"));
	registerAction(Action::ImagesPolicyAction, QT_TRANSLATE_NOOP("actions", "Load Images"));
	registerAction(Action::CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies..."));
	registerAction(Action::CookiesPolicyAction, QT_TRANSLATE_NOOP("actions", "Cookies Policy"));
	registerAction(Action::ThirdPartyCookiesPolicyAction, QT_TRANSLATE_NOOP("actions", "Third-party Cookies Policy"));
	registerAction(Action::PluginsPolicyAction, QT_TRANSLATE_NOOP("actions", "Plugins"));
	registerAction(Action::LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load Plugins"), QString(), Utils::getIcon(QLatin1String("preferences-plugin")));
	registerAction(Action::EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), QString(), QIcon(), true, true, true);
	registerAction(Action::EnableJavaAction, QT_TRANSLATE_NOOP("actions", "Enable Java"), QString(), QIcon(), true, true, true);
	registerAction(Action::EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), QString(), QIcon(), true, true, true);
	registerAction(Action::ProxyMenuAction, QT_TRANSLATE_NOOP("actions", "Proxy"));
	registerAction(Action::EnableProxyAction, QT_TRANSLATE_NOOP("actions", "Enale Proxy"), QString(), QIcon(), true, true, true);
	registerAction(Action::ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), false, false, false);
	registerAction(Action::ValidateAction, QT_TRANSLATE_NOOP("actions", "Validate"));
	registerAction(Action::InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), true, true, false);
	registerAction(Action::InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element..."));
	registerAction(Action::WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), true, true, false);
	registerAction(Action::FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), Utils::getIcon(QLatin1String("view-fullscreen")));
	registerAction(Action::ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), true, true, true);
	registerAction(Action::ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), QIcon(), true, true, false);
	registerAction(Action::ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Error Console"), QString(), QIcon(), true, true, false);
	registerAction(Action::LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), QString(), QIcon(), true, true, false);
	registerAction(Action::ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking..."));
	registerAction(Action::HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), QString(), Utils::getIcon(QLatin1String("view-history")));
	registerAction(Action::ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History..."), QString(), Utils::getIcon(QLatin1String("edit-clear-history")));
	registerAction(Action::TransfersAction, QT_TRANSLATE_NOOP("actions", "Transfers..."));
	registerAction(Action::PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences..."));
	registerAction(Action::WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences..."));
	registerAction(Action::QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"));
	registerAction(Action::ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"));
	registerAction(Action::SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language..."), QString(), Utils::getIcon(QLatin1String("preferences-desktop-locale")));
	registerAction(Action::AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter..."), QString(), Utils::getIcon(QLatin1String("otter-browser"), false));
	registerAction(Action::AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt..."), QString(), Utils::getIcon(QLatin1String("qt"), false));
	registerAction(Action::ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), QString(), Utils::getIcon(QLatin1String("application-exit")));

	GesturesManager::createInstance(Application::getInstance());

	const QString toolBarsPath = (SessionsManager::getProfilePath() + QLatin1String("/toolBars.json"));
	QFile toolBarsFile(QFile::exists(toolBarsPath) ? toolBarsPath : QLatin1String(":/other/toolBars.json"));
	toolBarsFile.open(QFile::ReadOnly);

	const QJsonArray toolBars = QJsonDocument::fromJson(toolBarsFile.readAll()).array();

	for (int i = 0; i < toolBars.count(); ++i)
	{
		const QJsonObject toolBarObject = toolBars.at(i).toObject();
		const QJsonArray actions = toolBarObject.value(QLatin1String("actions")).toArray();
		const QString location = toolBarObject.value(QLatin1String("location")).toString();
		ToolBarDefinition toolBar;
		toolBar.name = toolBarObject.value(QLatin1String("identifier")).toString();
		toolBar.title = toolBarObject.value(QLatin1String("title")).toString();

		if (location == QLatin1String("top"))
		{
			toolBar.location = TopToolBarLocation;
		}
		else if (location == QLatin1String("bottom"))
		{
			toolBar.location = BottomBarArea;
		}
		else if (location == QLatin1String("left"))
		{
			toolBar.location = LeftToolBarLocation;
		}
		else if (location == QLatin1String("right"))
		{
			toolBar.location = RightToolBarLocation;
		}
		else if (location == QLatin1String("navigation"))
		{
			toolBar.location = NavigationToolBarLocation;
		}
		else if (location == QLatin1String("status"))
		{
			toolBar.location = StatusToolBarLocation;
		}
		else if (location == QLatin1String("tabs"))
		{
			toolBar.location = TabsToolBarLocation;
		}
		else if (location == QLatin1String("menu"))
		{
			toolBar.location = MenuToolBarLocation;
		}
		else
		{
			toolBar.location = UnknownToolBarLocation;
		}

		for (int j = 0; j < actions.count(); ++j)
		{
			ToolBarActionDefinition action;

			if (actions.at(j).isObject())
			{
				const QJsonObject actionObject = actions.at(j).toObject();

				action.action = actionObject.value(QLatin1String("identifier")).toString();
				action.options = actionObject.value(QLatin1String("options")).toObject().toVariantMap();
			}
			else
			{
				action.action = actions.at(j).toString();
			}

			toolBar.actions.append(action);
		}

		toolBarDefinitions[toolBar.name] = toolBar;
	}

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void ActionsManagerHelper::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == reloadShortcutsTimer)
	{
		killTimer(reloadShortcutsTimer);

		reloadShortcutsTimer = 0;

		ActionsManager::loadShortcuts();
	}
}

int ActionsManagerHelper::registerAction(int identifier, const QString &text, const QString &description, const QIcon &icon, bool isEnabled, bool isCheckable, bool isChecked)
{
	ActionDefinition action;
	action.text = text;
	action.description = description;
	action.icon = icon;
	action.identifier = identifier;
	action.isEnabled = isEnabled;
	action.isCheckable = isCheckable;
	action.isChecked = isChecked;

	actionDefinitions.append(action);

	return (actionDefinitions.count() - 1);
}

void ActionsManagerHelper::optionChanged(const QString &option)
{
	if ((option == QLatin1String("Browser/ActionMacrosProfilesOrder") || option == QLatin1String("Browser/KeyboardShortcutsProfilesOrder")) && reloadShortcutsTimer == 0)
	{
		reloadShortcutsTimer = startTimer(250);
	}
}

ActionsManager::ActionsManager(MainWindow *parent) : QObject(parent),
	m_mainWindow(parent),
	m_window(NULL)
{
	if (!m_helper)
	{
		initialize();
	}

	m_standardActions.fill(NULL, m_helper->actionDefinitions.count());

	updateShortcuts();

	connect(m_helper, SIGNAL(shortcutsChanged()), this, SLOT(updateShortcuts()));
}

void ActionsManager::initialize()
{
	if (!m_helper)
	{
		m_helper = new ActionsManagerHelper(QCoreApplication::instance());

		ActionsManager::loadShortcuts();
	}
}

void ActionsManager::actionTriggered()
{
	if (!m_helper)
	{
		initialize();
	}

	QShortcut *shortcut = qobject_cast<QShortcut*>(sender());

	if (shortcut)
	{
		m_mutex.lock();

		for (int i = 0; i < m_actionShortcuts.count(); ++i)
		{
			if (m_actionShortcuts[i].second.contains(shortcut))
			{
				const ActionDefinition definition = m_helper->actionDefinitions.value(m_actionShortcuts[i].first);

				if (definition.identifier >= 0)
				{
					if (definition.isCheckable)
					{
						Action *action = getAction(m_actionShortcuts[i].first);

						if (action)
						{
							action->toggle();
						}
					}
					else
					{
						triggerAction(m_actionShortcuts[i].first);
					}
				}

				break;
			}
		}

		m_mutex.unlock();

		return;
	}

	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier());
	}
}

void ActionsManager::actionTriggered(bool checked)
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		triggerAction(action->getIdentifier(), checked);
	}
}

void ActionsManager::macroTriggered()
{
	if (!m_helper)
	{
		initialize();
	}

	QShortcut *shortcut = qobject_cast<QShortcut*>(sender());

	if (shortcut)
	{
		m_mutex.lock();

		for (int i = 0; i < m_macroShortcuts.count(); ++i)
		{
			if (m_macroShortcuts[i].second.contains(shortcut))
			{
				const QVector<int> actions = m_helper->macroDefinitions.value(m_macroShortcuts[i].first).actions;

				for (int j = 0; j < actions.count(); ++j)
				{
					triggerAction(actions[j]);
				}

				break;
			}
		}

		m_mutex.unlock();
	}
}

void ActionsManager::updateShortcuts()
{
	if (!m_helper)
	{
		initialize();
	}

	for (int i = 0; i < m_actionShortcuts.count(); ++i)
	{
		qDeleteAll(m_actionShortcuts[i].second);
	}

	m_actionShortcuts.clear();

	for (int i = 0; i < m_macroShortcuts.count(); ++i)
	{
		qDeleteAll(m_macroShortcuts[i].second);
	}

	m_macroShortcuts.clear();

	m_mutex.lock();

	for (int i = 0; i < m_helper->actionDefinitions.count(); ++i)
	{
		QVector<QShortcut*> shortcuts;
		shortcuts.reserve(m_helper->actionDefinitions[i].shortcuts.count());

		for (int j = 0; j < m_helper->actionDefinitions[i].shortcuts.count(); ++j)
		{
			QShortcut *shortcut = new QShortcut(m_helper->actionDefinitions[i].shortcuts[j], m_mainWindow);

			shortcuts.append(shortcut);

			connect(shortcut, SIGNAL(activated()), this, SLOT(actionTriggered()));
		}

		m_actionShortcuts.append(qMakePair(i, shortcuts));
	}

	for (int i = 0; i < m_helper->macroDefinitions.count(); ++i)
	{
		QVector<QShortcut*> shortcuts;
		shortcuts.reserve(m_helper->macroDefinitions[i].shortcuts.count());

		for (int j = 0; j < m_helper->macroDefinitions[i].shortcuts.count(); ++j)
		{
			QShortcut *shortcut = new QShortcut(m_helper->macroDefinitions[i].shortcuts[j], m_mainWindow);

			shortcuts.append(shortcut);

			connect(shortcut, SIGNAL(activated()), this, SLOT(macroTriggered()));
		}

		m_macroShortcuts.append(qMakePair(i, shortcuts));
	}

	m_mutex.unlock();
}

void ActionsManager::triggerAction(int identifier, bool checked)
{
	m_mainWindow->triggerAction(identifier, checked);
}

void ActionsManager::triggerAction(int identifier, QObject *parent, bool checked)
{
	ActionsManager *manager = findManager(parent);

	if (manager)
	{
		manager->triggerAction(identifier, checked);
	}
}

void ActionsManager::loadShortcuts()
{
	if (!m_helper)
	{
		initialize();
	}

	QHash<int, QVector<QKeySequence> > actionShortcuts;
	QList<QKeySequence> allShortcuts;
	const QStringList shortcutProfiles = SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder")).toStringList();

	m_mutex.lock();

	for (int i = 0; i < shortcutProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList rawActions = profile.childGroups();

		for (int j = 0; j < rawActions.count(); ++j)
		{
			const int action = ActionsManager::getActionIdentifier(rawActions.at(j));
			const QStringList rawShortcuts = profile.value(rawActions.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QVector<QKeySequence> shortcuts;

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut(rawShortcuts.at(k));

				if (!shortcut.isEmpty() && !allShortcuts.contains(shortcut))
				{
					shortcuts.append(shortcut);
					allShortcuts.append(shortcut);
				}
			}

			if (!shortcuts.isEmpty())
			{
				actionShortcuts[action] = shortcuts;
			}
		}
	}

	for (int i = 0; i < m_helper->actionDefinitions.count(); ++i)
	{
		m_helper->actionDefinitions[i].shortcuts = actionShortcuts.value(i);
	}

	m_helper->macroDefinitions.clear();

	const QStringList macroProfiles = SettingsManager::getValue(QLatin1String("Browser/ActionMacrosProfilesOrder")).toStringList();

	for (int i = 0; i < macroProfiles.count(); ++i)
	{
		const QString path = SessionsManager::getProfilePath() + QLatin1String("/macros/") + macroProfiles.at(i) + QLatin1String(".ini");
		const QSettings profile((QFile::exists(path) ? path : QLatin1String(":/macros/") + macroProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList macros = profile.childGroups();

		for (int j = 0; j < macros.count(); ++j)
		{
			const QStringList rawActions = profile.value(macros.at(j) + QLatin1String("/actions"), QString()).toStringList();
			QVector<int> actions;

			for (int k = 0; k < rawActions.count(); ++k)
			{
				const int action = ActionsManager::getActionIdentifier(rawActions.at(k));

				if (action >= 0)
				{
					actions.append(action);
				}
			}

			const QStringList rawShortcuts = profile.value(macros.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
			QVector<QKeySequence> shortcuts;
			shortcuts.reserve(rawShortcuts.count());

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				const QKeySequence shortcut(rawShortcuts.at(k));

				if (!shortcut.isEmpty() && !allShortcuts.contains(shortcut))
				{
					shortcuts.append(shortcut);
					allShortcuts.append(shortcut);
				}
			}

			if (actions.isEmpty() || shortcuts.isEmpty())
			{
				continue;
			}

			MacroDefinition macro;
			macro.actions = actions;
			macro.shortcuts = shortcuts;

			m_helper->macroDefinitions.append(macro);
		}
	}

	m_mutex.unlock();

	emit m_helper->shortcutsChanged();
}

void ActionsManager::setCurrentWindow(Window *window)
{
	Window *previousWindow = m_window;

	m_window = window;

	for (int i = 0; i < m_standardActions.count(); ++i)
	{
		if (m_standardActions[i] && Action::isLocal(m_standardActions[i]->getIdentifier()))
		{
			const int identifier = m_standardActions[i]->getIdentifier();
			Action *previousAction = (previousWindow ? previousWindow->getContentsWidget()->getAction(identifier) : NULL);
			Action *currentAction = (window ? window->getContentsWidget()->getAction(identifier) : NULL);

			if (previousAction)
			{
				disconnect(previousAction, SIGNAL(changed()), m_standardActions[i], SLOT(setup()));
			}

			m_standardActions[i]->setup(currentAction);

			if (currentAction)
			{
				connect(currentAction, SIGNAL(changed()), m_standardActions[i], SLOT(setup()));
			}
		}
	}
}

ActionsManager* ActionsManager::findManager(QObject *parent)
{
	MainWindow *window = MainWindow::findMainWindow(parent);

	return (window ? window->getActionsManager() : NULL);
}

Action* ActionsManager::getAction(int identifier)
{
	if (!m_helper)
	{
		initialize();
	}

	if (identifier < 0 || identifier >= m_standardActions.count() || identifier >= m_helper->actionDefinitions.count())
	{
		return NULL;
	}

	if (!m_standardActions[identifier])
	{
		const ActionDefinition definition = m_helper->actionDefinitions[identifier];
		Action *action = new Action(identifier, m_mainWindow);

		m_standardActions[identifier] = action;

		m_mainWindow->addAction(action);

		if (definition.isCheckable)
		{
			connect(action, SIGNAL(toggled(bool)), this, SLOT(actionTriggered(bool)));
		}
		else
		{
			connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
		}
	}

	return m_standardActions.value(identifier, NULL);
}

Action* ActionsManager::getAction(int identifier, QObject *parent)
{
	ActionsManager *manager = findManager(parent);

	return (manager ? manager->getAction(identifier) : NULL);
}

QList<ActionDefinition> ActionsManager::getActionDefinitions()
{
	if (!m_helper)
	{
		initialize();
	}

	return m_helper->actionDefinitions.toList();
}

QList<MacroDefinition> ActionsManager::getMacroDefinitions()
{
	if (!m_helper)
	{
		initialize();
	}

	return m_helper->macroDefinitions.toList();
}

QList<ToolBarDefinition> ActionsManager::getToolBarDefinitions()
{
	if (!m_helper)
	{
		initialize();
	}

	return m_helper->toolBarDefinitions.values();
}

ActionDefinition ActionsManager::getActionDefinition(int identifier)
{
	if (!m_helper)
	{
		initialize();
	}

	if (identifier < 0 || identifier >= m_helper->actionDefinitions.count())
	{
		return ActionDefinition();
	}

	return m_helper->actionDefinitions[identifier];
}

ToolBarDefinition ActionsManager::getToolBarDefinition(const QString &toolBar)
{
	if (!m_helper)
	{
		initialize();
	}

	return m_helper->toolBarDefinitions.value(toolBar, ToolBarDefinition());
}

int ActionsManager::getActionIdentifier(const QString &name)
{
	if (!m_dummyAction)
	{
		m_dummyAction = new Action(-1, QCoreApplication::instance());
	}

	const int enumerator = m_dummyAction->metaObject()->indexOfEnumerator(QLatin1String("ActionIdentifier").data());

	if (!name.endsWith(QLatin1String("Action")))
	{
		const QString fullName = name + QLatin1String("Action");

		return m_dummyAction->metaObject()->enumerator(enumerator).keyToValue(fullName.toLatin1());
	}

	return m_dummyAction->metaObject()->enumerator(enumerator).keyToValue(name.toLatin1());
}

}
