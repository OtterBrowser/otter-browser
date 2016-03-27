/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include <QtCore/QVariantMap>
#include <QtWidgets/QAction>

namespace Otter
{

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
		MaximizeTabAction,
		MinimizeTabAction,
		RestoreTabAction,
		AlwaysOnTopTabAction,
		ClearTabHistoryAction,
		PurgeTabHistoryAction,
		SuspendTabAction,
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
		OpenUrlAction,
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
		OpenLinkInApplicationAction,
		CopyLinkToClipboardAction,
		BookmarkLinkAction,
		SaveLinkToDiskAction,
		SaveLinkToDownloadsAction,
		OpenSelectionAsLinkAction,
		OpenFrameInCurrentTabAction,
		OpenFrameInNewTabAction,
		OpenFrameInNewTabBackgroundAction,
		OpenFrameInApplicationAction,
		CopyFrameLinkToClipboardAction,
		ReloadFrameAction,
		ViewFrameSourceAction,
		OpenImageInNewTabAction,
		OpenImageInNewTabBackgroundAction,
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
		StopAllAction,
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
		SelectDictionaryAction,
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
		ActivatePreviouslyUsedTabAction,
		ActivateLeastRecentlyUsedTabAction,
		ActivateTabAction,
		ActivateTabOnLeftAction,
		ActivateTabOnRightAction,
		BookmarksAction,
		BookmarkPageAction,
		BookmarkAllOpenPagesAction,
		OpenBookmarkAction,
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
		OpenPageInApplicationAction,
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
		WebsiteInformationAction,
		WebsiteCertificateInformationAction,
		SwitchApplicationLanguageAction,
		CheckForUpdatesAction,
		DiagnosticReportAction,
		AboutApplicationAction,
		AboutQtAction,
		ExitAction,
		OtherAction
	};

	enum ActionFlag
	{
		NoFlags = 0,
		IsEnabledFlag = 1,
		IsCheckableFlag = 2,
		IsCheckedFlag = 4,
		IsMenuFlag = 8
	};

	Q_DECLARE_FLAGS(ActionFlags, ActionFlag)

	struct ActionDefinition
	{
		QString text;
		QString description;
		QIcon icon;
		QVector<QKeySequence> shortcuts;
		ActionFlags flags;
		int identifier;

		ActionDefinition() : flags(IsEnabledFlag), identifier(-1) {}
	};

	struct ActionEntryDefinition
	{
		QString action;
		QVariantMap options;
		QList<ActionEntryDefinition> entries;
	};

	static void createInstance(QObject *parent);
	static void loadProfiles();
	static void triggerAction(int identifier, QObject *parent, const QVariantMap &parameters = QVariantMap());
	static ActionsManager* getInstance();
	static Action* getAction(int identifier, QObject *parent);
	static QString getReport();
	static QString getActionName(int identifier);
	static QVector<ActionDefinition> getActionDefinitions();
	static ActionDefinition getActionDefinition(int identifier);
	static int registerAction(int identifier, const QString &text, const QString &description = QString(), const QIcon &icon = QIcon(), ActionFlags flags = IsEnabledFlag);
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

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::ActionsManager::ActionFlags)

#endif
