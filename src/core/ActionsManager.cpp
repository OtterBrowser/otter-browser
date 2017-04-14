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

	registerAction(RunMacroAction, QT_TRANSLATE_NOOP("actions", "Run Macro"), QT_TRANSLATE_NOOP("actions", "Run Arbitrary List of Actions"), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-new")), ActionDefinition::MainWindowScope);
	registerAction(NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-new-private")), ActionDefinition::MainWindowScope);
	registerAction(NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), QString(), ThemesManager::getIcon(QLatin1String("window-new")), ActionDefinition::ApplicationScope);
	registerAction(NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), ThemesManager::getIcon(QLatin1String("window-new-private")), ActionDefinition::ApplicationScope);
	registerAction(OpenAction, QT_TRANSLATE_NOOP("actions", "Open…"), QString(), ThemesManager::getIcon(QLatin1String("document-open")), ActionDefinition::MainWindowScope);
	registerAction(SaveAction, QT_TRANSLATE_NOOP("actions", "Save…"), QString(), ThemesManager::getIcon(QLatin1String("document-save")), ActionDefinition::WindowScope);
	registerAction(CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(MaximizeTabAction, QT_TRANSLATE_NOOP("actions", "Maximize"), QT_TRANSLATE_NOOP("actions", "Maximize Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(MinimizeTabAction, QT_TRANSLATE_NOOP("actions", "Minimize"), QT_TRANSLATE_NOOP("actions", "Minimize Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(RestoreTabAction, QT_TRANSLATE_NOOP("actions", "Restore"), QT_TRANSLATE_NOOP("actions", "Restore Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(AlwaysOnTopTabAction, QT_TRANSLATE_NOOP("actions", "Stay on Top"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ClearTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local Tab History"), QIcon(), ActionDefinition::WindowScope);
	registerAction(PurgeTabHistoryAction, QT_TRANSLATE_NOOP("actions", "Purge Tab History"), QT_TRANSLATE_NOOP("actions", "Remove Local and Global Tab History"), QIcon(), ActionDefinition::WindowScope);
	registerAction(MuteTabMediaAction, QT_TRANSLATE_NOOP("actions", "Mute Tab Media"), QString(), ThemesManager::getIcon(QLatin1String("audio-volume-medium")), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(SuspendTabAction, QT_TRANSLATE_NOOP("actions", "Suspend Tab"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), ThemesManager::getIcon(QLatin1String("tab-close")), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(CloseOtherTabsAction, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), QString(), ThemesManager::getIcon(QLatin1String("tab-close-other")), ActionDefinition::MainWindowScope);
	registerAction(ClosePrivateTabsAction, QT_TRANSLATE_NOOP("actions", "Close All Private Tabs"), QT_TRANSLATE_NOOP("actions", "Close All Private Tabs in Current Window"), QIcon(), ActionDefinition::MainWindowScope, ActionDefinition::NoFlags);
	registerAction(ClosePrivateTabsPanicAction, QT_TRANSLATE_NOOP("actions", "Close Private Tabs and Windows"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(ReopenTabAction, QT_TRANSLATE_NOOP("actions", "Reopen Previously Closed Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(MaximizeAllAction, QT_TRANSLATE_NOOP("actions", "Maximize All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(MinimizeAllAction, QT_TRANSLATE_NOOP("actions", "Minimize All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(RestoreAllAction, QT_TRANSLATE_NOOP("actions", "Restore All"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CascadeAllAction, QT_TRANSLATE_NOOP("actions", "Cascade"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(TileAllAction, QT_TRANSLATE_NOOP("actions", "Tile"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CloseWindowAction, QT_TRANSLATE_NOOP("actions", "Close Window"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(SessionsAction, QT_TRANSLATE_NOOP("actions", "Manage Sessions…"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(SaveSessionAction, QT_TRANSLATE_NOOP("actions", "Save Current Session…"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(OpenUrlAction, QT_TRANSLATE_NOOP("actions", "Open URL"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), QString(), ThemesManager::getIcon(QLatin1String("document-open")), ActionDefinition::WindowScope);
	registerAction(OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewPrivateTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewPrivateTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewPrivateWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Window"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInNewPrivateWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Window"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenLinkInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Link with External Application"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link…"), QString(), ThemesManager::getIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope);
	registerAction(SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenFrameInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Frame with External Application"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(CopyFrameLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Frame Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ReloadFrameAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload Frame"), QIcon(), ActionDefinition::WindowScope);
	registerAction(ViewFrameSourceAction, QT_TRANSLATE_NOOP("actions", "View Frame Source"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenImageInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open Image In New Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(OpenImageInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SaveImageToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Image…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyImageToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyImageUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Image Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ReloadImageAction, QT_TRANSLATE_NOOP("actions", "Reload Image"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ImagePropertiesAction, QT_TRANSLATE_NOOP("actions", "Image Properties…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SaveMediaToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Media…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyMediaUrlToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Media Link to Clipboard"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(MediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"), QT_TRANSLATE_NOOP("actions", "Show Media Controls"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(MediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"), QT_TRANSLATE_NOOP("actions", "Playback Looping"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(MediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"), QT_TRANSLATE_NOOP("actions", "Play Media"), QIcon(), ActionDefinition::WindowScope);
	registerAction(MediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"), QT_TRANSLATE_NOOP("actions", "Mute Media"), QIcon(), ActionDefinition::WindowScope);
	registerAction(FillPasswordAction, QT_TRANSLATE_NOOP("actions", "Log In"), QString(), ThemesManager::getIcon(QLatin1String("fill-password")), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QT_TRANSLATE_NOOP("actions", "Go to URL"), ThemesManager::getIcon(QLatin1String("go-jump-locationbar")), ActionDefinition::MainWindowScope);
	registerAction(GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QT_TRANSLATE_NOOP("actions", "Go Back"), ThemesManager::getIcon(QLatin1String("go-previous")), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QT_TRANSLATE_NOOP("actions", "Go Forward"), ThemesManager::getIcon(QLatin1String("go-next")), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), QString(), ThemesManager::getIcon(QLatin1String("go-home")), ActionDefinition::WindowScope);
	registerAction(GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QT_TRANSLATE_NOOP("actions", "Rewind History"), ThemesManager::getIcon(QLatin1String("go-first")), ActionDefinition::WindowScope);
	registerAction(FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), ThemesManager::getIcon(QLatin1String("go-last")), ActionDefinition::WindowScope);
	registerAction(StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), QString(), ThemesManager::getIcon(QLatin1String("process-stop")), ActionDefinition::WindowScope);
	registerAction(StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StopAllAction, QT_TRANSLATE_NOOP("actions", "Stop All Pages"), QString(), ThemesManager::getIcon(QLatin1String("process-stop")), ActionDefinition::MainWindowScope);
	registerAction(ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), QString(), ThemesManager::getIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope);
	registerAction(ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), ThemesManager::getIcon(QLatin1String("view-refresh")), ActionDefinition::WindowScope);
	registerAction(ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ReloadAllAction, QT_TRANSLATE_NOOP("actions", "Reload All Tabs"), QString(), ThemesManager::getIcon(QLatin1String("view-refresh")), ActionDefinition::MainWindowScope);
	registerAction(ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Reload Every"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(ContextMenuAction, QT_TRANSLATE_NOOP("actions", "Show Context Menu"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), QString(), ThemesManager::getIcon(QLatin1String("edit-undo")), ActionDefinition::WindowScope);
	registerAction(RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), QString(), ThemesManager::getIcon(QLatin1String("edit-redo")), ActionDefinition::WindowScope);
	registerAction(CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), QString(), ThemesManager::getIcon(QLatin1String("edit-cut")), ActionDefinition::WindowScope);
	registerAction(CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), QString(), ThemesManager::getIcon(QLatin1String("edit-copy")), ActionDefinition::WindowScope);
	registerAction(CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CopyToNoteAction, QT_TRANSLATE_NOOP("actions", "Copy to Note"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), QString(), ThemesManager::getIcon(QLatin1String("edit-paste")), ActionDefinition::WindowScope);
	registerAction(PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PasteNoteAction, QT_TRANSLATE_NOOP("actions", "Insert Note"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), QString(), ThemesManager::getIcon(QLatin1String("edit-delete")), ActionDefinition::WindowScope);
	registerAction(SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), QString(), ThemesManager::getIcon(QLatin1String("edit-select-all")), ActionDefinition::WindowScope);
	registerAction(UnselectAction, QT_TRANSLATE_NOOP("actions", "Deselect"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(SelectDictionaryAction, QT_TRANSLATE_NOOP("actions", "Dictionaries"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(FindAction, QT_TRANSLATE_NOOP("actions", "Find…"), QString(), ThemesManager::getIcon(QLatin1String("edit-find")), ActionDefinition::WindowScope);
	registerAction(FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"), QString(), ThemesManager::getIcon(QLatin1String("go-down")), ActionDefinition::WindowScope);
	registerAction(FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"), QString(), ThemesManager::getIcon(QLatin1String("go-up")), ActionDefinition::WindowScope);
	registerAction(QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SearchAction, QT_TRANSLATE_NOOP("actions", "Search"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), ThemesManager::getIcon(QLatin1String("zoom-in")), ActionDefinition::WindowScope);
	registerAction(ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), ThemesManager::getIcon(QLatin1String("zoom-out")), ActionDefinition::WindowScope);
	registerAction(ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), ThemesManager::getIcon(QLatin1String("zoom-original")), ActionDefinition::WindowScope);
	registerAction(ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StartDragScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Drag Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(StartMoveScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Move Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(EndScrollAction, QT_TRANSLATE_NOOP("actions", "Exit Scroll Mode"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(PrintAction, QT_TRANSLATE_NOOP("actions", "Print…"), QString(), ThemesManager::getIcon(QLatin1String("document-print")), ActionDefinition::WindowScope);
	registerAction(PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), ThemesManager::getIcon(QLatin1String("document-print-preview")), ActionDefinition::WindowScope);
	registerAction(ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ActivatePreviouslyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Previously Used Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateLeastRecentlyUsedTabAction, QT_TRANSLATE_NOOP("actions", "Go to Least Recently Used Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateTabAction, QT_TRANSLATE_NOOP("actions", "Activate Tab"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks"), QString(), ThemesManager::getIcon(QLatin1String("bookmarks-organize")), ActionDefinition::MainWindowScope);
	registerAction(BookmarkPageAction, QT_TRANSLATE_NOOP("actions", "Bookmark Page…"), QString(), ThemesManager::getIcon(QLatin1String("bookmark-new")), ActionDefinition::WindowScope);
	registerAction(BookmarkAllOpenPagesAction, QT_TRANSLATE_NOOP("actions", "Bookmark All Open Pages"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(OpenBookmarkAction, QT_TRANSLATE_NOOP("actions", "Open Bookmark"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load Plugins"), QString(), ThemesManager::getIcon(QLatin1String("preferences-plugin")), ActionDefinition::WindowScope);
	registerAction(EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(OpenPageInApplicationAction, QT_TRANSLATE_NOOP("actions", "Open with…"), QT_TRANSLATE_NOOP("actions", "Open Current Page with External Application"), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::HasMenuFlag));
	registerAction(InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), ActionDefinition::WindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), ThemesManager::getIcon(QLatin1String("view-fullscreen")), ActionDefinition::MainWindowScope);
	registerAction(ShowTabSwitcherAction, QT_TRANSLATE_NOOP("actions", "Show Tab Switcher"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ShowTabBarAction, QT_TRANSLATE_NOOP("actions", "Show Tabbar"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag | ActionDefinition::IsCheckedFlag));
	registerAction(ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), ThemesManager::getIcon(QLatin1String("sidebar-show")), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Show Error Console"), QString(), QIcon(), ActionDefinition::MainWindowScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), QString(), QIcon(), ActionDefinition::ApplicationScope, (ActionDefinition::IsEnabledFlag | ActionDefinition::IsCheckableFlag));
	registerAction(ResetToolBarsAction, QT_TRANSLATE_NOOP("actions", "Reset to Defaults…"), QT_TRANSLATE_NOOP("actions", "Reset Toolbars to Defaults…"), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(OpenPanelAction, QT_TRANSLATE_NOOP("actions", "Open Panel as Tab"), QString(), ThemesManager::getIcon(QLatin1String("arrow-right")), ActionDefinition::MainWindowScope);
	registerAction(ClosePanelAction, QT_TRANSLATE_NOOP("actions", "Close Panel"), QString(), ThemesManager::getIcon(QLatin1String("window-close")), ActionDefinition::MainWindowScope);
	registerAction(ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking…"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), QString(), ThemesManager::getIcon(QLatin1String("view-history")), ActionDefinition::MainWindowScope);
	registerAction(ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History…"), QString(), ThemesManager::getIcon(QLatin1String("edit-clear-history")), ActionDefinition::MainWindowScope);
	registerAction(AddonsAction, QT_TRANSLATE_NOOP("actions", "Addons"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(NotesAction, QT_TRANSLATE_NOOP("actions", "Notes"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(PasswordsAction, QT_TRANSLATE_NOOP("actions", "Passwords"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(TransfersAction, QT_TRANSLATE_NOOP("actions", "Transfers"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences…"), QString(), QIcon(), ActionDefinition::MainWindowScope);
	registerAction(WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"), QString(), QIcon(), ActionDefinition::WindowScope, ActionDefinition::NoFlags);
	registerAction(WebsiteInformationAction, QT_TRANSLATE_NOOP("actions", "Website Information…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(WebsiteCertificateInformationAction, QT_TRANSLATE_NOOP("actions", "Website Certificate Information…"), QString(), QIcon(), ActionDefinition::WindowScope);
	registerAction(SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language…"), QString(), ThemesManager::getIcon(QLatin1String("preferences-desktop-locale")), ActionDefinition::ApplicationScope);
	registerAction(CheckForUpdatesAction, QT_TRANSLATE_NOOP("actions", "Check for Updates…"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(DiagnosticReportAction, QT_TRANSLATE_NOOP("actions", "Diagnostic Report…"), QString(), QIcon(), ActionDefinition::ApplicationScope);
	registerAction(AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter…"), QString(), ThemesManager::getIcon(QLatin1String("otter-browser")), ActionDefinition::ApplicationScope);
	registerAction(AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt…"), QString(), ThemesManager::getIcon(QLatin1String("qt")), ActionDefinition::ApplicationScope);
	registerAction(ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), QString(), ThemesManager::getIcon(QLatin1String("application-exit")), ActionDefinition::ApplicationScope);

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

void ActionsManager::registerAction(int identifier, const QString &text, const QString &description, const QIcon &icon, ActionDefinition::ActionScope scope, ActionDefinition::ActionFlags flags)
{
	ActionsManager::ActionDefinition action;
	action.description = description;
	action.defaultState.text = text;
	action.defaultState.icon = icon;
	action.defaultState.isEnabled = flags.testFlag(ActionDefinition::IsEnabledFlag);
	action.defaultState.isChecked = flags.testFlag(ActionDefinition::IsCheckedFlag);
	action.identifier = identifier;
	action.flags = flags;
	action.scope = scope;

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
