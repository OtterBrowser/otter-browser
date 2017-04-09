/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ActionsManager.h"
#include "ThemesManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

namespace Otter
{

ActionsManager* ActionsManager::m_instance(nullptr);
QVector<ActionsManager::ActionDefinition> ActionsManager::m_definitions;
int ActionsManager::m_actionIdentifierEnumerator(0);

ActionsManager::ActionsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	m_definitions.reserve(ActionsManager::OtherAction);

	registerAction(RunMacroAction, QT_TRANSLATE_NOOP("actions", "Run Macro"), QT_TRANSLATE_NOOP("actions", "Run Arbitrary List of Actions"));
	registerAction(NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-new")));
	registerAction(NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-new-private")));
	registerAction(NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), QString(), ThemesManager::getIcon(QLatin1String("window-new")));
	registerAction(NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), ThemesManager::getIcon(QLatin1String("window-new-private")));
	registerAction(OpenAction, QT_TRANSLATE_NOOP("actions", "Open…"), QString(), ThemesManager::getIcon(QLatin1String("document-open")));
	registerAction(SaveAction, QT_TRANSLATE_NOOP("actions", "Save…"), QString(), ThemesManager::getIcon(QLatin1String("document-save")));
	registerAction(CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"));
	registerAction(PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"));
	registerAction(DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"));
	registerAction(MaximizeTabAction, QT_TRANSLATE_NOOP("actions", "Maximize"), QT_TRANSLATE_NOOP("actions", "Maximize Tab"));
	registerAction(MinimizeTabAction, QT_TRANSLATE_NOOP("actions", "Minimize"), QT_TRANSLATE_NOOP("actions", "Minimize Tab"));
	registerAction(RestoreTabAction, QT_TRANSLATE_NOOP("actions", "Restore"), QT_TRANSLATE_NOOP("actions", "Restore Tab"));
	registerAction(AlwaysOnTopTabAction, QT_TRANSLATE_NOOP("actions", "Stay on Top"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ClearTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local Tab History"));
	registerAction(PurgeTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Purge Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local and Global Tab History"));
	registerAction(MuteTabMediaAction, QT_TRANSLATE_NOOP("actions", "Mute Tab Media"), QString(), ThemesManager::getIcon(QLatin1String("audio-volume-medium")), ActionDefinition::NoFlags);
	registerAction(SuspendTabAction, QT_TRANSLATE_NOOP("actions", "Suspend Tab"), QString(), QIcon(), ActionDefinition::NoFlags);
	registerAction(CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-close")), ActionDefinition::NoFlags);
	registerAction(CloseOtherTabsAction, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), QString(), ThemesManager::getIcon(QLatin1String("tab-close-other")));
	registerAction(ClosePrivateTabsAction, QT_TRANSLATE_NOOP("actions", "Close All Private Tabs"), QT_TRANSLATE_NOOP("actions", "Close All Private Tabs in Current Window"), QIcon(), ActionDefinition::NoFlags);
	registerAction(ClosePrivateTabsPanicAction, QT_TRANSLATE_NOOP("actions", "Close Private Tabs and Windows"));
	registerAction(ReopenTabAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"));
	registerAction(MaximizeAllAction, QT_TRANSLATE_NOOP("actions", "Maximize All"));
	registerAction(MinimizeAllAction, QT_TRANSLATE_NOOP("actions", "Minimize All"));
	registerAction(RestoreAllAction, QT_TRANSLATE_NOOP("actions", "Restore All"));
	registerAction(CascadeAllAction, QT_TRANSLATE_NOOP("actions", "Cascade"));
	registerAction(TileAllAction, QT_TRANSLATE_NOOP("actions", "Tile"));
	registerAction(CloseWindowAction, QT_TRANSLATE_NOOP("actions", "Close Window"));
	registerAction(SessionsAction, QT_TRANSLATE_NOOP("actions", "Manage Sessions…"));
	registerAction(SaveSessionAction, QT_TRANSLATE_NOOP("actions", "Save Current Session…"));
	registerAction(OpenUrlAction, QT_TRANSLATE_NOOP("actions", "Open URL"));
	registerAction(OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), QString(), ThemesManager::getIcon(QLatin1String("document-open")));
	registerAction(OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"));
	registerAction(OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"));
	registerAction(OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"));
	registerAction(OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"));
	registerAction(OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"));
	registerAction(OpenLinkInNewPrivateTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Tab"));
	registerAction(OpenLinkInNewPrivateTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Tab"));
	registerAction(OpenLinkInNewPrivateWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Window"));
	registerAction(OpenLinkInNewPrivateWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Window"));
	registerAction(OpenLinkInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Link with External Application"), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"));
	registerAction(BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link…"), QString(), ThemesManager::getIcon(QLatin1String("bookmark-new")));
	registerAction(SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As…"));
	registerAction(SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"));
	registerAction(OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"));
	registerAction(OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"));
	registerAction(OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"));
	registerAction(OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"));
	registerAction(OpenFrameInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Frame with External Application"), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(CopyFrameLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"));
	registerAction(ReloadFrameAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"));
	registerAction(ViewFrameSourceAction, QT_TRANSLATE_NOOP("actions", "View Frame Source"));
	registerAction(OpenImageInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open Image In New Tab"));
	registerAction(OpenImageInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"));
	registerAction(SaveImageToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Image…"));
	registerAction(CopyImageToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"));
	registerAction(CopyImageUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"));
	registerAction(ReloadImageAction, QT_TRANSLATE_NOOP("actions", "Reload Image"));
	registerAction(ImagePropertiesAction, QT_TRANSLATE_NOOP("actions", "Image Properties…"));
	registerAction(SaveMediaToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Media…"));
	registerAction(CopyMediaUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"));
	registerAction(MediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(MediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(MediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"));
	registerAction(MediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"));
	registerAction(FillPasswordAction, QT_TRANSLATE_NOOP("actions", "Log In"), QString(), ThemesManager::getIcon(QLatin1String("fill-password")), ActionDefinition::NoFlags);
	registerAction(GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QString(), ThemesManager::getIcon(QLatin1String("go-jump-locationbar")));
	registerAction(GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QString(), ThemesManager::getIcon(QLatin1String("go-previous")), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QString(), ThemesManager::getIcon(QLatin1String("go-next")), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"));
	registerAction(GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), QString(), ThemesManager::getIcon(QLatin1String("go-home")));
	registerAction(GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"));
	registerAction(RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QString(), ThemesManager::getIcon(QLatin1String("go-first")));
	registerAction(FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), ThemesManager::getIcon(QLatin1String("go-last")));
	registerAction(StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), QString(), ThemesManager::getIcon(QLatin1String("process-stop")));
	registerAction(StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"));
	registerAction(StopAllAction, QT_TRANSLATE_NOOP("actions", "Stop All Pages"), QString(), ThemesManager::getIcon(QLatin1String("process-stop")));
	registerAction(ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), QString(), ThemesManager::getIcon(QLatin1String("view-refresh")));
	registerAction(ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), ThemesManager::getIcon(QLatin1String("view-refresh")));
	registerAction(ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"));
	registerAction(ReloadAllAction, QT_TRANSLATE_NOOP("actions", "Reload All Tabs"), QString(), ThemesManager::getIcon(QLatin1String("view-refresh")));
	registerAction(ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Reload Every"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(ContextMenuAction, QT_TRANSLATE_NOOP("actions", "Show Context Menu"));
	registerAction(UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), QString(), ThemesManager::getIcon(QLatin1String("edit-undo")));
	registerAction(RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), QString(), ThemesManager::getIcon(QLatin1String("edit-redo")));
	registerAction(CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), QString(), ThemesManager::getIcon(QLatin1String("edit-cut")));
	registerAction(CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), QString(), ThemesManager::getIcon(QLatin1String("edit-copy")));
	registerAction(CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"));
	registerAction(CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"));
	registerAction(CopyToNoteAction, QT_TRANSLATE_NOOP("actions", "Copy to Note"));
	registerAction(PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), QString(), ThemesManager::getIcon(QLatin1String("edit-paste")));
	registerAction(PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"));
	registerAction(PasteNoteAction, QT_TRANSLATE_NOOP("actions", "Insert Note"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), QString(), ThemesManager::getIcon(QLatin1String("edit-delete")));
	registerAction(SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), QString(), ThemesManager::getIcon(QLatin1String("edit-select-all")));
	registerAction(UnselectAction, QT_TRANSLATE_NOOP("actions", "Deselect"));
	registerAction(ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"));
	registerAction(CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(SelectDictionaryAction, QT_TRANSLATE_NOOP("actions", "Dictionaries"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(FindAction, QT_TRANSLATE_NOOP("actions", "Find…"), QString(), ThemesManager::getIcon(QLatin1String("edit-find")));
	registerAction(FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"), QString(), ThemesManager::getIcon(QLatin1String("go-down")));
	registerAction(FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"), QString(), ThemesManager::getIcon(QLatin1String("go-up")));
	registerAction(QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"));
	registerAction(SearchAction, QT_TRANSLATE_NOOP("actions", "Search"));
	registerAction(CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search…"));
	registerAction(ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), ThemesManager::getIcon(QLatin1String("zoom-in")));
	registerAction(ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), ThemesManager::getIcon(QLatin1String("zoom-out")));
	registerAction(ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), ThemesManager::getIcon(QLatin1String("zoom-original")));
	registerAction(ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"));
	registerAction(ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"));
	registerAction(ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"));
	registerAction(ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"));
	registerAction(ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"));
	registerAction(ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"));
	registerAction(StartDragScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Drag Scroll Mode"));
	registerAction(StartMoveScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Move Scroll Mode"));
	registerAction(EndScrollAction, QT_TRANSLATE_NOOP("actions", "Exit Scroll Mode"));
	registerAction(PrintAction, QT_TRANSLATE_NOOP("actions", "Print…"), QString(), ThemesManager::getIcon(QLatin1String("document-print")));
	registerAction(PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), ThemesManager::getIcon(QLatin1String("document-print-preview")));
	registerAction(ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"));
	registerAction(ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"));
	registerAction(ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"));
	registerAction(ActivatePreviouslyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Previously Used Tab"));
	registerAction(ActivateLeastRecentlyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Least Recently Used Tab"));
	registerAction(ActivateTabAction, QT_TRANSLATE_NOOP("actions", "Activate Tab"));
	registerAction(ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"));
	registerAction(ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"));
	registerAction(BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks"), QString(), ThemesManager::getIcon(QLatin1String("bookmarks-organize")));
	registerAction(BookmarkPageAction, QT_TRANSLATE_NOOP("actions", "Bookmark Page…"), QString(), ThemesManager::getIcon(QLatin1String("bookmark-new")));
	registerAction(BookmarkAllOpenPagesAction, QT_TRANSLATE_NOOP("actions", "Bookmark All Open Pages"));
	registerAction(OpenBookmarkAction, QT_TRANSLATE_NOOP("actions", "Open Bookmark"));
	registerAction(QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"));
	registerAction(CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies"));
	registerAction(LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load Plugins"), QString(), ThemesManager::getIcon(QLatin1String("preferences-plugin")));
	registerAction(EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), ActionDefinition::NoFlags);
	registerAction(OpenPageInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Current Page with External Application"), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element…"));
	registerAction(WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), ThemesManager::getIcon(QLatin1String("view-fullscreen")));
	registerAction(ShowTabSwitcherAction, QT_TRANSLATE_NOOP("actions", "Show Tab Switcher"));
	registerAction(ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ShowTabBarAction, QT_TRANSLATE_NOOP("actions", "Show Tabbar"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), ThemesManager::getIcon(QLatin1String("sidebar-show")), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Show Error Console"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), QString(), QIcon(), (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ResetToolBarsAction, QT_TRANSLATE_NOOP("actions", "Reset to Defaults…"), QT_TRANSLATE_NOOP("actions", "Reset Toolbars to Defaults…"), QIcon(), ActionDefinition::IsEnabledFlag);
	registerAction(OpenPanelAction, QT_TRANSLATE_NOOP("actions", "Open Panel as Tab"), QString(), ThemesManager::getIcon(QLatin1String("arrow-right")));
	registerAction(ClosePanelAction, QT_TRANSLATE_NOOP("actions", "Close Panel"), QString(), ThemesManager::getIcon(QLatin1String("window-close")));
	registerAction(ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking…"));
	registerAction(HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), QString(), ThemesManager::getIcon(QLatin1String("view-history")));
	registerAction(ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History…"), QString(), ThemesManager::getIcon(QLatin1String("edit-clear-history")));
	registerAction(AddonsAction, QT_TRANSLATE_NOOP("actions", "Addons"));
	registerAction(NotesAction, QT_TRANSLATE_NOOP("actions", "Notes"));
	registerAction(PasswordsAction, QT_TRANSLATE_NOOP("actions", "Passwords"));
	registerAction(TransfersAction, QT_TRANSLATE_NOOP("actions", "Transfers"));
	registerAction(PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences…"));
	registerAction(WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences…"));
	registerAction(QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"));
	registerAction(ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"), QString(), QIcon(), ActionDefinition::NoFlags);
	registerAction(WebsiteInformationAction, QT_TRANSLATE_NOOP("actions", "Website Information…"));
	registerAction(WebsiteCertificateInformationAction, QT_TRANSLATE_NOOP("actions", "Website Certificate Information…"));
	registerAction(SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language…"), QString(), ThemesManager::getIcon(QLatin1String("preferences-desktop-locale")));
	registerAction(CheckForUpdatesAction, QT_TRANSLATE_NOOP("actions", "Check for Updates…"));
	registerAction(DiagnosticReportAction, QT_TRANSLATE_NOOP("actions", "Diagnostic Report…"));
	registerAction(AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter…"), QString(), ThemesManager::getIcon(QLatin1String("otter-browser"), false));
	registerAction(AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt…"), QString(), ThemesManager::getIcon(QLatin1String("qt"), false));
	registerAction(ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), QString(), ThemesManager::getIcon(QLatin1String("application-exit")));

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int)));
}

void ActionsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ActionsManager(parent);
		m_actionIdentifierEnumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("ActionIdentifier").data());

		loadProfiles();
	}
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

void ActionsManager::loadProfiles()
{
	QHash<int, QVector<QKeySequence> > actionShortcuts;
	QVector<QKeySequence> allShortcuts;
	const QStringList shortcutProfiles(SettingsManager::getOption(SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption).toStringList());
	const bool enableSingleKeyShortcuts(SettingsManager::getOption(SettingsManager::Browser_EnableSingleKeyShortcutsOption).toBool());
	QRegularExpression functionKeyExpression(QLatin1String("F\\d+"));

	for (int i = 0; i < shortcutProfiles.count(); ++i)
	{
		const QSettings profile(SessionsManager::getReadableDataPath(QLatin1String("keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList rawActions(profile.childGroups());

		for (int j = 0; j < rawActions.count(); ++j)
		{
			const int action(ActionsManager::getActionIdentifier(rawActions.at(j)));
			const QStringList rawShortcuts(profile.value(rawActions.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts));
			QVector<QKeySequence> shortcuts;

			if (actionShortcuts.contains(action))
			{
				shortcuts = actionShortcuts[action];
			}

			for (int k = 0; k < rawShortcuts.count(); ++k)
			{
				if (!enableSingleKeyShortcuts && !functionKeyExpression.match(rawShortcuts.at(k)).hasMatch() && (!rawShortcuts.at(k).contains(QLatin1Char('+')) || rawShortcuts.at(k) == QLatin1String("+")))
				{
					continue;
				}

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

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		m_definitions[i].shortcuts = actionShortcuts.value(i);
	}

	emit m_instance->shortcutsChanged();
}

void ActionsManager::triggerAction(int identifier, QObject *parent, const QVariantMap &parameters)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent));

	if (!mainWindow)
	{
		return;
	}

	if (identifier == RunMacroAction)
	{
		const QVariantList actions(parameters.value(QLatin1String("actions")).toList());

		for (int i = 0; i < actions.count(); ++i)
		{
			QVariant actionIdentifier;
			QVariantMap actionParameters;

			if (actions.at(i).type() == QVariant::Map)
			{
				const QVariantMap action(actions.at(i).toMap());

				actionIdentifier = action.value(QLatin1String("identifier"));
				actionParameters = action.value(QLatin1String("parameters")).toMap();
			}
			else
			{
				actionIdentifier = actions.at(i);
			}

			if (actionIdentifier.type() == QVariant::Int)
			{
				triggerAction(actionIdentifier.toInt(), mainWindow, actionParameters);
			}
			else
			{
				triggerAction(getActionIdentifier(actionIdentifier.toString()), mainWindow, actionParameters);
			}
		}
	}
	else
	{
		mainWindow->triggerAction(identifier, parameters);
	}
}

void ActionsManager::registerAction(int identifier, const QString &text, const QString &description, const QIcon &icon, ActionDefinition::ActionFlags flags)
{
	ActionsManager::ActionDefinition action;
	action.description = description;
	action.defaultState.text = text;
	action.defaultState.icon = icon;
	action.defaultState.isEnabled = flags.testFlag(ActionDefinition::IsEnabledFlag);
	action.defaultState.isChecked = flags.testFlag(ActionDefinition::IsCheckedFlag);
	action.identifier = identifier;
	action.flags = flags;

	m_definitions.append(action);
}

void ActionsManager::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Browser_EnableSingleKeyShortcutsOption:
		case SettingsManager::Browser_KeyboardShortcutsProfilesOrderOption:
			if (m_reloadTimer == 0)
			{
				m_reloadTimer = startTimer(250);
			}

			break;
		default:
			break;
	}
}

ActionsManager* ActionsManager::getInstance()
{
	return m_instance;
}

Action* ActionsManager::getAction(int identifier, QObject *parent)
{
	MainWindow *window(MainWindow::findMainWindow(parent));

	return (window ? window->getAction(identifier) : nullptr);
}

QString ActionsManager::getReport()
{
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Keyboard Shortcuts:\n");

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		if (m_definitions.at(i).shortcuts.isEmpty())
		{
			continue;
		}

		stream << QLatin1Char('\t');
		stream.setFieldWidth(30);
		stream << getActionName(m_definitions.at(i).identifier);
		stream.setFieldWidth(20);

		for (int j = 0; j < m_definitions.at(i).shortcuts.count(); ++j)
		{
			stream << m_definitions.at(i).shortcuts.at(j).toString(QKeySequence::PortableText);
		}

		stream.setFieldWidth(0);
		stream << QLatin1Char('\n');
	}

	stream << QLatin1Char('\n');

	return report;
}

QString ActionsManager::getActionName(int identifier)
{
	QString name(m_instance->metaObject()->enumerator(m_actionIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(6);

		return name;
	}

	return QString();
}

QVector<ActionsManager::ActionDefinition> ActionsManager::getActionDefinitions()
{
	return m_definitions;
}

ActionsManager::ActionDefinition ActionsManager::getActionDefinition(int identifier)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return ActionsManager::ActionDefinition();
	}

	return m_definitions[identifier];
}

int ActionsManager::getActionIdentifier(const QString &name)
{
	if (!name.endsWith(QLatin1String("Action")))
	{
		return m_instance->metaObject()->enumerator(m_actionIdentifierEnumerator).keyToValue(QString(name + QLatin1String("Action")).toLatin1());
	}

	return m_instance->metaObject()->enumerator(m_actionIdentifierEnumerator).keyToValue(name.toLatin1());
}

}
