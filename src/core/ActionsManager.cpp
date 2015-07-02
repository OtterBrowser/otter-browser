/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ActionsManager.h"
#include "Utils.h"
#include "../ui/MainWindow.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>

namespace Otter
{

ActionsManager* ActionsManager::m_instance = NULL;
QVector<ActionDefinition> ActionsManager::m_definitions;


Action::Action(int identifier, QObject *parent) : QAction(parent),
	m_identifier(identifier),
	m_isOverridingText(false)
{
	update(true);
}

void Action::setup(Action *action)
{
	if (!action)
	{
		action = qobject_cast<Action*>(sender());
	}

	if (!action)
	{
		update(true);
		setEnabled(false);

		return;
	}

	m_identifier = action->getIdentifier();

	setEnabled(action->isEnabled());
	setText(action->text());
	setIcon(action->icon());
	setCheckable(action->isCheckable());
	setChecked(action->isChecked());
}

void Action::update(bool reset)
{
	if (m_identifier < 0)
	{
		return;
	}

	const ActionDefinition action = ActionsManager::getActionDefinition(m_identifier);
	QString text = QCoreApplication::translate("actions", (m_isOverridingText ? m_overrideText : action.text).toUtf8().constData());

	if (!action.shortcuts.isEmpty())
	{
		text += QLatin1Char('\t') + action.shortcuts.first().toString(QKeySequence::NativeText);
	}

	setText(text);

	if (reset)
	{
		setEnabled(action.isEnabled);
		setCheckable(action.isCheckable);
		setChecked(action.isChecked);
		setIcon(action.icon);
	}
}

void Action::setOverrideText(const QString &text)
{
	m_overrideText = text;
	m_isOverridingText = true;

	update();
}

QString Action::getText() const
{
	return QCoreApplication::translate("actions", (m_isOverridingText ? m_overrideText : ActionsManager::getActionDefinition(m_identifier).text).toUtf8().constData());
}

QList<QKeySequence> Action::getShortcuts() const
{
	return ActionsManager::getActionDefinition(m_identifier).shortcuts.toList();
}

int Action::getIdentifier() const
{
	return m_identifier;
}

bool Action::event(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		update();
	}

	return QAction::event(event);
}

bool Action::isLocal(int identifier)
{
	switch (identifier)
	{
		case ActionsManager::SaveAction:
		case ActionsManager::CloneTabAction:
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenLinkInNewWindowAction:
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::BookmarkLinkAction:
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
		case ActionsManager::OpenSelectionAsLinkAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
		case ActionsManager::SaveMediaToDiskAction:
		case ActionsManager::CopyMediaUrlToClipboardAction:
		case ActionsManager::MediaControlsAction:
		case ActionsManager::MediaLoopAction:
		case ActionsManager::MediaPlayPauseAction:
		case ActionsManager::MediaMuteAction:
		case ActionsManager::GoBackAction:
		case ActionsManager::GoForwardAction:
		case ActionsManager::RewindAction:
		case ActionsManager::FastForwardAction:
		case ActionsManager::StopAction:
		case ActionsManager::StopScheduledReloadAction:
		case ActionsManager::ReloadAction:
		case ActionsManager::ReloadOrStopAction:
		case ActionsManager::ReloadAndBypassCacheAction:
		case ActionsManager::ScheduleReloadAction:
		case ActionsManager::UndoAction:
		case ActionsManager::RedoAction:
		case ActionsManager::CutAction:
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyAddressAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::PasteAction:
		case ActionsManager::PasteAndGoAction:
		case ActionsManager::PasteNoteAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::SelectAllAction:
		case ActionsManager::ClearAllAction:
		case ActionsManager::CheckSpellingAction:
		case ActionsManager::FindAction:
		case ActionsManager::FindNextAction:
		case ActionsManager::FindPreviousAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::SearchAction:
		case ActionsManager::SearchMenuAction:
		case ActionsManager::CreateSearchAction:
		case ActionsManager::ZoomInAction:
		case ActionsManager::ZoomOutAction:
		case ActionsManager::ZoomOriginalAction:
		case ActionsManager::ScrollToStartAction:
		case ActionsManager::ScrollToEndAction:
		case ActionsManager::ScrollPageUpAction:
		case ActionsManager::ScrollPageDownAction:
		case ActionsManager::ScrollPageLeftAction:
		case ActionsManager::ScrollPageRightAction:
		case ActionsManager::StartDragScrollAction:
		case ActionsManager::StartMoveScrollAction:
		case ActionsManager::EndScrollAction:
		case ActionsManager::ActivateContentAction:
		case ActionsManager::AddBookmarkAction:
		case ActionsManager::QuickBookmarkAccessAction:
		case ActionsManager::PopupsPolicyAction:
		case ActionsManager::ImagesPolicyAction:
		case ActionsManager::CookiesPolicyAction:
		case ActionsManager::ThirdPartyCookiesPolicyAction:
		case ActionsManager::PluginsPolicyAction:
		case ActionsManager::LoadPluginsAction:
		case ActionsManager::EnableJavaScriptAction:
		case ActionsManager::EnableJavaAction:
		case ActionsManager::EnableReferrerAction:
		case ActionsManager::ProxyMenuAction:
		case ActionsManager::EnableProxyAction:
		case ActionsManager::ViewSourceAction:
		case ActionsManager::ValidateAction:
		case ActionsManager::InspectPageAction:
		case ActionsManager::InspectElementAction:
		case ActionsManager::WebsitePreferencesAction:
		case ActionsManager::QuickPreferencesAction:
		case ActionsManager::ResetQuickPreferencesAction:
			return true;
		default:
			break;
	}

	return false;
}

ActionsManager::ActionsManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "File"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Sessions"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Import and Export"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Edit"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "View"));
	Q_UNUSED(QT_TRANSLATE_NOOP("actions", "Toolbars"));
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

	m_definitions.reserve(ActionsManager::OtherAction);

	registerAction(NewTabAction, QT_TRANSLATE_NOOP("actions", "New Tab"), QString(), Utils::getIcon(QLatin1String("tab-new")));
	registerAction(NewTabPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Tab"), QString(), Utils::getIcon(QLatin1String("tab-new-private")));
	registerAction(NewWindowAction, QT_TRANSLATE_NOOP("actions", "New Window"), QString(), Utils::getIcon(QLatin1String("window-new")));
	registerAction(NewWindowPrivateAction, QT_TRANSLATE_NOOP("actions", "New Private Window"), QString(), Utils::getIcon(QLatin1String("window-new-private")));
	registerAction(OpenAction, QT_TRANSLATE_NOOP("actions", "Open…"), QString(), Utils::getIcon(QLatin1String("document-open")));
	registerAction(SaveAction, QT_TRANSLATE_NOOP("actions", "Save…"), QString(), Utils::getIcon(QLatin1String("document-save")));
	registerAction(CloneTabAction, QT_TRANSLATE_NOOP("actions", "Clone Tab"));
	registerAction(PinTabAction, QT_TRANSLATE_NOOP("actions", "Pin Tab"));
	registerAction(DetachTabAction, QT_TRANSLATE_NOOP("actions", "Detach Tab"));
	registerAction(MaximizeTabAction, QT_TRANSLATE_NOOP("actions", "Maximize"), QT_TRANSLATE_NOOP("actions", "Maximize Tab"));
	registerAction(MinimizeTabAction, QT_TRANSLATE_NOOP("actions", "Minimize"), QT_TRANSLATE_NOOP("actions", "Minimize Tab"));
	registerAction(RestoreTabAction, QT_TRANSLATE_NOOP("actions", "Restore"), QT_TRANSLATE_NOOP("actions", "Restore Tab"));
	registerAction(AlwaysOnTopTabAction, QT_TRANSLATE_NOOP("actions", "Stay on Top"), QString(), QIcon(), true, true);
	registerAction(CloseTabAction, QT_TRANSLATE_NOOP("actions", "Close Tab"), QString(), Utils::getIcon(QLatin1String("tab-close")), false);
	registerAction(CloseOtherTabsAction, QT_TRANSLATE_NOOP("actions", "Close Other Tabs"), QString(), Utils::getIcon(QLatin1String("tab-close-other")));
	registerAction(ClosePrivateTabsAction, QT_TRANSLATE_NOOP("actions", "Close All Private Tabs"), QT_TRANSLATE_NOOP("actions", "Close All Private Tabs in Current Window"), QIcon(), false);
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
	registerAction(OpenLinkAction, QT_TRANSLATE_NOOP("actions", "Open"), QString(), Utils::getIcon(QLatin1String("document-open")));
	registerAction(OpenLinkInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open in This Tab"));
	registerAction(OpenLinkInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"));
	registerAction(OpenLinkInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"));
	registerAction(OpenLinkInNewWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Window"));
	registerAction(OpenLinkInNewWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Window"));
	registerAction(OpenLinkInNewPrivateTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Tab"));
	registerAction(OpenLinkInNewPrivateTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Tab"));
	registerAction(OpenLinkInNewPrivateWindowAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Window"));
	registerAction(OpenLinkInNewPrivateWindowBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Private Background Window"));
	registerAction(CopyLinkToClipboardAction, QT_TRANSLATE_NOOP("actions", "Copy Link to Clipboard"));
	registerAction(BookmarkLinkAction, QT_TRANSLATE_NOOP("actions", "Bookmark Link…"), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
	registerAction(SaveLinkToDiskAction, QT_TRANSLATE_NOOP("actions", "Save Link Target As…"));
	registerAction(SaveLinkToDownloadsAction, QT_TRANSLATE_NOOP("actions", "Save to Downloads"));
	registerAction(OpenSelectionAsLinkAction, QT_TRANSLATE_NOOP("actions", "Go to This Address"));
	registerAction(OpenFrameInCurrentTabAction, QT_TRANSLATE_NOOP("actions", "Open"), QT_TRANSLATE_NOOP("actions", "Open Frame in This Tab"));
	registerAction(OpenFrameInNewTabAction, QT_TRANSLATE_NOOP("actions", "Open in New Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Tab"));
	registerAction(OpenFrameInNewTabBackgroundAction, QT_TRANSLATE_NOOP("actions", "Open in New Background Tab"), QT_TRANSLATE_NOOP("actions", "Open Frame in New Background Tab"));
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
	registerAction(MediaControlsAction, QT_TRANSLATE_NOOP("actions", "Show Controls"), QString(), QIcon(), true, true, true);
	registerAction(MediaLoopAction, QT_TRANSLATE_NOOP("actions", "Looping"), QString(), QIcon(), true, true, false);
	registerAction(MediaPlayPauseAction, QT_TRANSLATE_NOOP("actions", "Play"));
	registerAction(MediaMuteAction, QT_TRANSLATE_NOOP("actions", "Mute"));
	registerAction(GoAction, QT_TRANSLATE_NOOP("actions", "Go"), QString(), Utils::getIcon(QLatin1String("go-jump-locationbar")));
	registerAction(GoBackAction, QT_TRANSLATE_NOOP("actions", "Back"), QString(), Utils::getIcon(QLatin1String("go-previous")));
	registerAction(GoForwardAction, QT_TRANSLATE_NOOP("actions", "Forward"), QString(), Utils::getIcon(QLatin1String("go-next")));
	registerAction(GoToPageAction, QT_TRANSLATE_NOOP("actions", "Go to Page or Search"));
	registerAction(GoToHomePageAction, QT_TRANSLATE_NOOP("actions", "Go to Home Page"), QString(), Utils::getIcon(QLatin1String("go-home")));
	registerAction(GoToParentDirectoryAction, QT_TRANSLATE_NOOP("actions", "Go to Parent Directory"));
	registerAction(RewindAction, QT_TRANSLATE_NOOP("actions", "Rewind"), QString(), Utils::getIcon(QLatin1String("go-first")));
	registerAction(FastForwardAction, QT_TRANSLATE_NOOP("actions", "Fast Forward"), QString(), Utils::getIcon(QLatin1String("go-last")));
	registerAction(StopAction, QT_TRANSLATE_NOOP("actions", "Stop"), QString(), Utils::getIcon(QLatin1String("process-stop")));
	registerAction(StopScheduledReloadAction, QT_TRANSLATE_NOOP("actions", "Stop Scheduled Page Reload"));
	registerAction(ReloadAction, QT_TRANSLATE_NOOP("actions", "Reload"), QString(), Utils::getIcon(QLatin1String("view-refresh")));
	registerAction(ReloadOrStopAction, QT_TRANSLATE_NOOP("actions", "Reload"), QT_TRANSLATE_NOOP("actions", "Reload or Stop"), Utils::getIcon(QLatin1String("view-refresh")));
	registerAction(ReloadAndBypassCacheAction, QT_TRANSLATE_NOOP("actions", "Reload and Bypass Cache"));
	registerAction(ReloadAllAction, QT_TRANSLATE_NOOP("actions", "Reload All Tabs"), QString(), Utils::getIcon(QLatin1String("view-refresh")));
	registerAction(ScheduleReloadAction, QT_TRANSLATE_NOOP("actions", "Reload Every"));
	registerAction(ContextMenuAction, QT_TRANSLATE_NOOP("actions", "Show Context Menu"));
	registerAction(UndoAction, QT_TRANSLATE_NOOP("actions", "Undo"), QString(), Utils::getIcon(QLatin1String("edit-undo")));
	registerAction(RedoAction, QT_TRANSLATE_NOOP("actions", "Redo"), QString(), Utils::getIcon(QLatin1String("edit-redo")));
	registerAction(CutAction, QT_TRANSLATE_NOOP("actions", "Cut"), QString(), Utils::getIcon(QLatin1String("edit-cut")));
	registerAction(CopyAction, QT_TRANSLATE_NOOP("actions", "Copy"), QString(), Utils::getIcon(QLatin1String("edit-copy")));
	registerAction(CopyPlainTextAction, QT_TRANSLATE_NOOP("actions", "Copy as Plain Text"));
	registerAction(CopyAddressAction, QT_TRANSLATE_NOOP("actions", "Copy Address"));
	registerAction(CopyToNoteAction, QT_TRANSLATE_NOOP("actions", "Copy to Note"));
	registerAction(PasteAction, QT_TRANSLATE_NOOP("actions", "Paste"), QString(), Utils::getIcon(QLatin1String("edit-paste")));
	registerAction(PasteAndGoAction, QT_TRANSLATE_NOOP("actions", "Paste and Go"));
	registerAction(PasteNoteAction, QT_TRANSLATE_NOOP("actions", "Insert Note"));
	registerAction(DeleteAction, QT_TRANSLATE_NOOP("actions", "Delete"), QString(), Utils::getIcon(QLatin1String("edit-delete")));
	registerAction(SelectAllAction, QT_TRANSLATE_NOOP("actions", "Select All"), QString(), Utils::getIcon(QLatin1String("edit-select-all")));
	registerAction(ClearAllAction, QT_TRANSLATE_NOOP("actions", "Clear All"));
	registerAction(CheckSpellingAction, QT_TRANSLATE_NOOP("actions", "Check Spelling"));
	registerAction(FindAction, QT_TRANSLATE_NOOP("actions", "Find…"), QString(), Utils::getIcon(QLatin1String("edit-find")));
	registerAction(FindNextAction, QT_TRANSLATE_NOOP("actions", "Find Next"));
	registerAction(FindPreviousAction, QT_TRANSLATE_NOOP("actions", "Find Previous"));
	registerAction(QuickFindAction, QT_TRANSLATE_NOOP("actions", "Quick Find"));
	registerAction(SearchAction, QT_TRANSLATE_NOOP("actions", "Search"));
	registerAction(SearchMenuAction, QT_TRANSLATE_NOOP("actions", "Search Using"));
	registerAction(CreateSearchAction, QT_TRANSLATE_NOOP("actions", "Create Search…"));
	registerAction(ZoomInAction, QT_TRANSLATE_NOOP("actions", "Zoom In"), QString(), Utils::getIcon(QLatin1String("zoom-in")));
	registerAction(ZoomOutAction, QT_TRANSLATE_NOOP("actions", "Zoom Out"), QString(), Utils::getIcon(QLatin1String("zoom-out")));
	registerAction(ZoomOriginalAction, QT_TRANSLATE_NOOP("actions", "Zoom Original"), QString(), Utils::getIcon(QLatin1String("zoom-original")));
	registerAction(ScrollToStartAction, QT_TRANSLATE_NOOP("actions", "Go to Start of the Page"));
	registerAction(ScrollToEndAction, QT_TRANSLATE_NOOP("actions", "Go to the End of the Page"));
	registerAction(ScrollPageUpAction, QT_TRANSLATE_NOOP("actions", "Page Up"));
	registerAction(ScrollPageDownAction, QT_TRANSLATE_NOOP("actions", "Page Down"));
	registerAction(ScrollPageLeftAction, QT_TRANSLATE_NOOP("actions", "Page Left"));
	registerAction(ScrollPageRightAction, QT_TRANSLATE_NOOP("actions", "Page Right"));
	registerAction(StartDragScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Drag Scroll Mode"));
	registerAction(StartMoveScrollAction, QT_TRANSLATE_NOOP("actions", "Enter Move Scroll Mode"));
	registerAction(EndScrollAction, QT_TRANSLATE_NOOP("actions", "Exit Scroll Mode"));
	registerAction(PrintAction, QT_TRANSLATE_NOOP("actions", "Print…"), QString(), Utils::getIcon(QLatin1String("document-print")));
	registerAction(PrintPreviewAction, QT_TRANSLATE_NOOP("actions", "Print Preview"), QString(), Utils::getIcon(QLatin1String("document-print-preview")));
	registerAction(ActivateAddressFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Address Field"));
	registerAction(ActivateSearchFieldAction, QT_TRANSLATE_NOOP("actions", "Activate Search Field"));
	registerAction(ActivateContentAction, QT_TRANSLATE_NOOP("actions", "Activate Content"));
	registerAction(ActivateTabOnLeftAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Left"));
	registerAction(ActivateTabOnRightAction, QT_TRANSLATE_NOOP("actions", "Go to Tab on Right"));
	registerAction(BookmarksAction, QT_TRANSLATE_NOOP("actions", "Manage Bookmarks"), QString(), Utils::getIcon(QLatin1String("bookmarks-organize")));
	registerAction(AddBookmarkAction, QT_TRANSLATE_NOOP("actions", "Add Bookmark…"), QString(), Utils::getIcon(QLatin1String("bookmark-new")));
	registerAction(QuickBookmarkAccessAction, QT_TRANSLATE_NOOP("actions", "Quick Bookmark Access"));
	registerAction(PopupsPolicyAction, QT_TRANSLATE_NOOP("actions", "Block pop-ups"));
	registerAction(ImagesPolicyAction, QT_TRANSLATE_NOOP("actions", "Load Images"));
	registerAction(CookiesAction, QT_TRANSLATE_NOOP("actions", "Cookies"));
	registerAction(CookiesPolicyAction, QT_TRANSLATE_NOOP("actions", "Cookies Policy"));
	registerAction(ThirdPartyCookiesPolicyAction, QT_TRANSLATE_NOOP("actions", "Third-party Cookies Policy"));
	registerAction(PluginsPolicyAction, QT_TRANSLATE_NOOP("actions", "Plugins"));
	registerAction(LoadPluginsAction, QT_TRANSLATE_NOOP("actions", "Load Plugins"), QString(), Utils::getIcon(QLatin1String("preferences-plugin")));
	registerAction(EnableJavaScriptAction, QT_TRANSLATE_NOOP("actions", "Enable JavaScript"), QString(), QIcon(), true, true, true);
	registerAction(EnableJavaAction, QT_TRANSLATE_NOOP("actions", "Enable Java"), QString(), QIcon(), true, true, true);
	registerAction(EnableReferrerAction, QT_TRANSLATE_NOOP("actions", "Enable Referrer"), QString(), QIcon(), true, true, true);
	registerAction(ProxyMenuAction, QT_TRANSLATE_NOOP("actions", "Proxy"));
	registerAction(EnableProxyAction, QT_TRANSLATE_NOOP("actions", "Enable Proxy"), QString(), QIcon(), true, true, true);
	registerAction(ViewSourceAction, QT_TRANSLATE_NOOP("actions", "View Source"), QString(), QIcon(), true, false, false);
	registerAction(ValidateAction, QT_TRANSLATE_NOOP("actions", "Validate"));
	registerAction(InspectPageAction, QT_TRANSLATE_NOOP("actions", "Inspect Page"), QString(), QIcon(), true, true, false);
	registerAction(InspectElementAction, QT_TRANSLATE_NOOP("actions", "Inspect Element…"));
	registerAction(WorkOfflineAction, QT_TRANSLATE_NOOP("actions", "Work Offline"), QString(), QIcon(), true, true, false);
	registerAction(FullScreenAction, QT_TRANSLATE_NOOP("actions", "Full Screen"), QString(), Utils::getIcon(QLatin1String("view-fullscreen")));
	registerAction(ShowTabSwitcherAction, QT_TRANSLATE_NOOP("actions", "Show Tab Switcher"));
	registerAction(ShowMenuBarAction, QT_TRANSLATE_NOOP("actions", "Show Menubar"), QString(), QIcon(), true, true, true);
	registerAction(ShowTabBarAction, QT_TRANSLATE_NOOP("actions", "Show Tabbar"), QString(), QIcon(), true, true, true);
	registerAction(ShowSidebarAction, QT_TRANSLATE_NOOP("actions", "Show Sidebar"), QString(), QIcon(), true, true, false);
	registerAction(ShowErrorConsoleAction, QT_TRANSLATE_NOOP("actions", "Show Error Console"), QString(), QIcon(), true, true, false);
	registerAction(LockToolBarsAction, QT_TRANSLATE_NOOP("actions", "Lock Toolbars"), QString(), QIcon(), true, true, false);
	registerAction(OpenPanelAction, QT_TRANSLATE_NOOP("actions", "Open Panel as Tab"), QString(), Utils::getIcon(QLatin1String("arrow-right")), true, false, false);
	registerAction(ClosePanelAction, QT_TRANSLATE_NOOP("actions", "Close Panel"), QString(), Utils::getIcon(QLatin1String("window-close")), true, false, false);
	registerAction(ContentBlockingAction, QT_TRANSLATE_NOOP("actions", "Content Blocking…"));
	registerAction(HistoryAction, QT_TRANSLATE_NOOP("actions", "View History"), QString(), Utils::getIcon(QLatin1String("view-history")));
	registerAction(ClearHistoryAction, QT_TRANSLATE_NOOP("actions", "Clear History…"), QString(), Utils::getIcon(QLatin1String("edit-clear-history")));
	registerAction(NotesAction, QT_TRANSLATE_NOOP("actions", "Notes"));
	registerAction(TransfersAction, QT_TRANSLATE_NOOP("actions", "Transfers"));
	registerAction(PreferencesAction, QT_TRANSLATE_NOOP("actions", "Preferences…"));
	registerAction(WebsitePreferencesAction, QT_TRANSLATE_NOOP("actions", "Website Preferences…"));
	registerAction(QuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Quick Preferences"));
	registerAction(ResetQuickPreferencesAction, QT_TRANSLATE_NOOP("actions", "Reset Options"));
	registerAction(SwitchApplicationLanguageAction, QT_TRANSLATE_NOOP("actions", "Switch Application Language…"), QString(), Utils::getIcon(QLatin1String("preferences-desktop-locale")));
	registerAction(CheckForUpdatesAction, QT_TRANSLATE_NOOP("actions", "Check for Updates…"));
	registerAction(AboutApplicationAction, QT_TRANSLATE_NOOP("actions", "About Otter…"), QString(), Utils::getIcon(QLatin1String("otter-browser"), false));
	registerAction(AboutQtAction, QT_TRANSLATE_NOOP("actions", "About Qt…"), QString(), Utils::getIcon(QLatin1String("qt"), false));
	registerAction(ExitAction, QT_TRANSLATE_NOOP("actions", "Exit"), QString(), Utils::getIcon(QLatin1String("application-exit")));

	m_reloadTimer = startTimer(250);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void ActionsManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new ActionsManager(parent);
	}
}

void ActionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		ActionsManager::loadProfiles();
	}
}

void ActionsManager::optionChanged(const QString &option)
{
	if ((option == QLatin1String("Browser/KeyboardShortcutsProfilesOrder") || option == QLatin1String("Browser/EnableSingleKeyShortcuts")) && m_reloadTimer == 0)
	{
		m_reloadTimer = startTimer(250);
	}
}

void ActionsManager::loadProfiles()
{
	QHash<int, QVector<QKeySequence> > actionShortcuts;
	QVector<QKeySequence> allShortcuts;
	const QStringList shortcutProfiles = SettingsManager::getValue(QLatin1String("Browser/KeyboardShortcutsProfilesOrder")).toStringList();
	const bool enableSingleKeyShortcuts = SettingsManager::getValue(QLatin1String("Browser/EnableSingleKeyShortcuts")).toBool();
	QRegularExpression functionKeyExpression(QLatin1String("F\\d+"));

	for (int i = 0; i < shortcutProfiles.count(); ++i)
	{
		const QSettings profile(SessionsManager::getReadableDataPath(QLatin1String("keyboard/") + shortcutProfiles.at(i) + QLatin1String(".ini")), QSettings::IniFormat);
		const QStringList rawActions = profile.childGroups();

		for (int j = 0; j < rawActions.count(); ++j)
		{
			const int action = ActionsManager::getActionIdentifier(rawActions.at(j));
			const QStringList rawShortcuts = profile.value(rawActions.at(j) + QLatin1String("/shortcuts"), QString()).toString().split(QLatin1Char(' '), QString::SkipEmptyParts);
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

void ActionsManager::triggerAction(int identifier, QObject *parent, bool checked)
{
	MainWindow *window = MainWindow::findMainWindow(parent);

	if (window)
	{
		window->triggerAction(identifier, checked);
	}
}

ActionsManager* ActionsManager::getInstance()
{
	return m_instance;
}

Action* ActionsManager::getAction(int identifier, QObject *parent)
{
	MainWindow *window = MainWindow::findMainWindow(parent);

	return (window ? window->getAction(identifier) : NULL);
}

QString ActionsManager::getActionName(int identifier)
{
	QString name = m_instance->metaObject()->enumerator(m_instance->metaObject()->indexOfEnumerator(QLatin1String("ActionIdentifier").data())).valueToKey(identifier);

	if (!name.isEmpty())
	{
		name.chop(6);

		return name;
	}

	return QString();
}

QVector<ActionDefinition> ActionsManager::getActionDefinitions()
{
	return m_definitions;
}

ActionDefinition ActionsManager::getActionDefinition(int identifier)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return ActionDefinition();
	}

	return m_definitions[identifier];
}

int ActionsManager::registerAction(int identifier, const QString &text, const QString &description, const QIcon &icon, bool isEnabled, bool isCheckable, bool isChecked)
{
	ActionDefinition action;
	action.text = text;
	action.description = description;
	action.icon = icon;
	action.identifier = identifier;
	action.isEnabled = isEnabled;
	action.isCheckable = isCheckable;
	action.isChecked = isChecked;

	m_definitions.append(action);

	return (m_definitions.count() - 1);
}

int ActionsManager::getActionIdentifier(const QString &name)
{
	const int enumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("ActionIdentifier").data());

	if (!name.endsWith(QLatin1String("Action")))
	{
		const QString fullName = name + QLatin1String("Action");

		return m_instance->metaObject()->enumerator(enumerator).keyToValue(fullName.toLatin1());
	}

	return m_instance->metaObject()->enumerator(enumerator).keyToValue(name.toLatin1());
}

}
