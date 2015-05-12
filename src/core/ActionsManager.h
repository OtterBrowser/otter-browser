/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include <QtWidgets/QAction>

namespace Otter
{

struct ActionDefinition
{
	QString text;
	QString description;
	QIcon icon;
	QVector<QKeySequence> shortcuts;
	int identifier;
	bool isCheckable;
	bool isChecked;
	bool isEnabled;

	ActionDefinition() : identifier(-1), isCheckable(false), isChecked(false), isEnabled(true) {}
};

class Action : public QAction
{
	Q_OBJECT

public:
	explicit Action(int identifier, QObject *parent = NULL);

	void setOverrideText(const QString &text);
	QString getText() const;
	QList<QKeySequence> getShortcuts() const;
	int getIdentifier() const;
	bool event(QEvent *event);
	static bool isLocal(int identifier);

public slots:
	void setup(Action *action = NULL);

protected:
	void update(bool reset = false);

private:
	QString m_overrideText;
	int m_identifier;
	bool m_isOverridingText;
};

class ActionsManager : public QObject
{
	Q_OBJECT
	Q_ENUMS(ActionIdentifier)

public:
	enum ActionIdentifier
	{
		NewTabAction = 0,
		NewTabPrivateAction,
		NewWindowAction,
		NewWindowPrivateAction,
		OpenAction,
		SaveAction,
		CloneTabAction,
		PinTabAction,
		DetachTabAction,
		CloseTabAction,
		CloseOtherTabsAction,
		ClosePrivateTabsAction,
		ClosePrivateTabsPanicAction,
		ReopenTabAction,
		MaximizeAllAction,
		MinimizeAllAction,
		RestoreAllAction,
		CascadeAllAction,
		TileAllAction,
		CloseWindowAction,
		SessionsAction,
		SaveSessionAction,
		OpenLinkAction,
		OpenLinkInCurrentTabAction,
		OpenLinkInNewTabAction,
		OpenLinkInNewTabBackgroundAction,
		OpenLinkInNewWindowAction,
		OpenLinkInNewWindowBackgroundAction,
		OpenLinkInNewPrivateTabAction,
		OpenLinkInNewPrivateTabBackgroundAction,
		OpenLinkInNewPrivateWindowAction,
		OpenLinkInNewPrivateWindowBackgroundAction,
		CopyLinkToClipboardAction,
		BookmarkLinkAction,
		SaveLinkToDiskAction,
		SaveLinkToDownloadsAction,
		OpenSelectionAsLinkAction,
		OpenFrameInCurrentTabAction,
		OpenFrameInNewTabAction,
		OpenFrameInNewTabBackgroundAction,
		CopyFrameLinkToClipboardAction,
		ReloadFrameAction,
		ViewFrameSourceAction,
		OpenImageInNewTabAction,
		SaveImageToDiskAction,
		CopyImageToClipboardAction,
		CopyImageUrlToClipboardAction,
		ReloadImageAction,
		ImagePropertiesAction,
		SaveMediaToDiskAction,
		CopyMediaUrlToClipboardAction,
		MediaControlsAction,
		MediaLoopAction,
		MediaPlayPauseAction,
		MediaMuteAction,
		GoAction,
		GoBackAction,
		GoForwardAction,
		GoToPageAction,
		GoToHomePageAction,
		GoToParentDirectoryAction,
		RewindAction,
		FastForwardAction,
		StopAction,
		StopScheduledReloadAction,
		ReloadAction,
		ReloadOrStopAction,
		ReloadAndBypassCacheAction,
		ReloadAllAction,
		ScheduleReloadAction,
		ContextMenuAction,
		UndoAction,
		RedoAction,
		CutAction,
		CopyAction,
		CopyPlainTextAction,
		CopyAddressAction,
		CopyToNoteAction,
		PasteAction,
		PasteAndGoAction,
		PasteNoteAction,
		DeleteAction,
		SelectAllAction,
		ClearAllAction,
		CheckSpellingAction,
		FindAction,
		FindNextAction,
		FindPreviousAction,
		QuickFindAction,
		SearchAction,
		SearchMenuAction,
		CreateSearchAction,
		ZoomInAction,
		ZoomOutAction,
		ZoomOriginalAction,
		ScrollToStartAction,
		ScrollToEndAction,
		ScrollPageUpAction,
		ScrollPageDownAction,
		ScrollPageLeftAction,
		ScrollPageRightAction,
		StartDragScrollAction,
		StartMoveScrollAction,
		EndScrollAction,
		PrintAction,
		PrintPreviewAction,
		ActivateAddressFieldAction,
		ActivateSearchFieldAction,
		ActivateContentAction,
		ActivateTabOnLeftAction,
		ActivateTabOnRightAction,
		BookmarksAction,
		AddBookmarkAction,
		QuickBookmarkAccessAction,
		PopupsPolicyAction,
		ImagesPolicyAction,
		CookiesAction,
		CookiesPolicyAction,
		ThirdPartyCookiesPolicyAction,
		PluginsPolicyAction,
		LoadPluginsAction,
		EnableJavaScriptAction,
		EnableJavaAction,
		EnableReferrerAction,
		ProxyMenuAction,
		EnableProxyAction,
		ViewSourceAction,
		ValidateAction,
		InspectPageAction,
		InspectElementAction,
		WorkOfflineAction,
		FullScreenAction,
		ShowTabSwitcherAction,
		ShowMenuBarAction,
		ShowTabBarAction,
		ShowSidebarAction,
		ShowErrorConsoleAction,
		LockToolBarsAction,
		OpenPanelAction,
		ClosePanelAction,
		ContentBlockingAction,
		HistoryAction,
		ClearHistoryAction,
		NotesAction,
		TransfersAction,
		PreferencesAction,
		WebsitePreferencesAction,
		QuickPreferencesAction,
		ResetQuickPreferencesAction,
		SwitchApplicationLanguageAction,
		AboutApplicationAction,
		AboutQtAction,
		ExitAction,
		OtherAction
	};

	static void createInstance(QObject *parent);
	static void loadProfiles();
	static void triggerAction(int identifier, QObject *parent, bool checked = false);
	static ActionsManager* getInstance();
	static Action* getAction(int identifier, QObject *parent);
	static QString getActionName(int identifier);
	static QVector<ActionDefinition> getActionDefinitions();
	static ActionDefinition getActionDefinition(int identifier);
	static int registerAction(int identifier, const QString &text, const QString &description = QString(), const QIcon &icon = QIcon(), bool isEnabled = true, bool isCheckable = false, bool isChecked = false);
	static int getActionIdentifier(const QString &name);

protected:
	explicit ActionsManager(QObject *parent);

	void timerEvent(QTimerEvent *event);

protected slots:
	void optionChanged(const QString &option);

signals:
	void shortcutsChanged();

private:
	int m_reloadTimer;

	static ActionsManager *m_instance;
	static QVector<ActionDefinition> m_definitions;
};

}

#endif
