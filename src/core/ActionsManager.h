/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include "AddonsManager.h"

#include <QtCore/QVariantMap>
#include <QtGui/QIcon>

namespace Otter
{

class KeyboardProfile final : public Addon
{
public:
	enum LoadMode
	{
		StandardMode = 0,
		MetaDataOnlyMode,
		FullMode
	};

	struct Action final
	{
		QVariantMap parameters;
		QVector<QKeySequence> shortcuts;
		int action = -1;

		bool operator ==(const Action &other) const;
	};

	explicit KeyboardProfile(const QString &identifier = {}, LoadMode mode = StandardMode);

	void setTitle(const QString &title);
	void setDescription(const QString &description);
	void setAuthor(const QString &author);
	void setVersion(const QString &version);
	void setDefinitions(const QHash<int, QVector<Action> > &definitions);
	void setModified(bool isModified);
	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getAuthor() const;
	QString getVersion() const override;
	QHash<int, QVector<Action> > getDefinitions() const;
	bool isModified() const;
	bool save();

private:
	QString m_identifier;
	QString m_title;
	QString m_description;
	QString m_author;
	QString m_version;
	QHash<int, QVector<Action> > m_definitions;
	bool m_isModified;
};

class ActionsManager final : public QObject
{
	Q_OBJECT
	Q_ENUMS(ActionIdentifier)

public:
	enum ShortcutCheck
	{
		AllChecks = 0,
		DisallowSingleKeyShortcutCheck,
		DisallowStandardShortcutCheck
	};

	enum GesturesContext
	{
		UnknownContext = 0,
		GenericContext
	};

	enum TriggerType
	{
		UnknownTrigger = 0,
		KeyboardTrigger,
		MouseTrigger
	};

	enum ActionIdentifier
	{
		RunMacroAction = 0,
		SetOptionAction,
		NewTabAction,
		NewTabPrivateAction,
		NewWindowAction,
		NewWindowPrivateAction,
		OpenAction,
		SaveAction,
		CloneTabAction,
		PeekTabAction,
		PinTabAction,
		DetachTabAction,
		MaximizeTabAction,
		MinimizeTabAction,
		RestoreTabAction,
		AlwaysOnTopTabAction,
		ClearTabHistoryAction,
		PurgeTabHistoryAction,
		MuteTabMediaAction,
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
		ReopenWindowAction,
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
		CopyLinkToClipboardAction,
		BookmarkLinkAction,
		SaveLinkToDiskAction,
		SaveLinkToDownloadsAction,
		OpenSelectionAsLinkAction,
		OpenFrameAction,
		OpenFrameInCurrentTabAction,
		OpenFrameInNewTabAction,
		OpenFrameInNewTabBackgroundAction,
		CopyFrameLinkToClipboardAction,
		ReloadFrameAction,
		ViewFrameSourceAction,
		OpenImageAction,
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
		MediaPlaybackRateAction,
		FillPasswordAction,
		GoAction,
		GoBackAction,
		GoForwardAction,
		GoToHistoryIndexAction,
		GoToPageAction,
		GoToHomePageAction,
		GoToParentDirectoryAction,
		RewindAction,
		FastForwardAction,
		RemoveHistoryIndexAction,
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
		DeleteAction,
		SelectAllAction,
		UnselectAction,
		ClearAllAction,
		CheckSpellingAction,
		FindAction,
		FindNextAction,
		FindPreviousAction,
		QuickFindAction,
		SearchAction,
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
		TakeScreenshotAction,
		ActivateAddressFieldAction,
		ActivateSearchFieldAction,
		ActivateContentAction,
		ActivatePreviouslyUsedTabAction,
		ActivateLeastRecentlyUsedTabAction,
		ActivateTabAction,
		ActivateTabOnLeftAction,
		ActivateTabOnRightAction,
		ActivateWindowAction,
		BookmarksAction,
		BookmarkPageAction,
		BookmarkAllOpenPagesAction,
		OpenBookmarkAction,
		QuickBookmarkAccessAction,
		CookiesAction,
		LoadPluginsAction,
		EnableJavaScriptAction,
		EnableReferrerAction,
		ViewSourceAction,
		InspectPageAction,
		InspectElementAction,
		WorkOfflineAction,
		FullScreenAction,
		ShowTabSwitcherAction,
		ShowToolBarAction,
		ShowMenuBarAction,
		ShowTabBarAction,
		ShowSidebarAction,
		ShowErrorConsoleAction,
		LockToolBarsAction,
		ResetToolBarsAction,
		ShowPanelAction,
		OpenPanelAction,
		ContentBlockingAction,
		HistoryAction,
		ClearHistoryAction,
		AddonsAction,
		NotesAction,
		PasswordsAction,
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

	struct ActionDefinition final
	{
		enum ActionCategory
		{
			OtherCategory = 0,
			PageCategory,
			NavigationCategory,
			EditingCategory,
			LinkCategory,
			FrameCategory,
			ImageCategory,
			MediaCategory,
			BookmarkCategory,
			UserCategory
		};

		enum ActionFlag
		{
			NoFlags = 0,
			IsEnabledFlag = 1,
			IsCheckableFlag = 2,
			IsDeprecatedFlag = 4,
			IsImmutableFlag = 8,
			RequiresParameters = 16
		};

		Q_DECLARE_FLAGS(ActionFlags, ActionFlag)

		enum ActionScope
		{
			OtherScope = 0,
			EditorScope,
			WindowScope,
			MainWindowScope,
			ApplicationScope
		};

		struct Parameter final
		{
			QString name;
			QString title;
			bool isRequired;
		};

		struct State final
		{
			QString text;
			QIcon icon;
			bool isEnabled = true;
			bool isChecked = false;
		};

		QString description;
		State defaultState;
		QVector<Parameter> parameters;
		ActionFlags flags = IsEnabledFlag;
		ActionCategory category = OtherCategory;
		ActionScope scope = OtherScope;
		int identifier = -1;

		QString getText(bool preferDescription = false) const
		{
			return QCoreApplication::translate("actions", ((preferDescription && !description.isEmpty()) ? description : defaultState.text).toUtf8().constData());
		}

		State getDefaultState() const
		{
			State state(defaultState);
			state.text = getText(false);

			return state;
		}

		bool isValid() const
		{
			return (identifier >= 0);
		}
	};

	static void createInstance();
	static void loadProfiles();
	static ActionsManager* getInstance();
	static QString createReport();
	static QString getActionName(int identifier);
	static QKeySequence getActionShortcut(int identifier, const QVariantMap &parameters = {});
	static QVector<ActionDefinition> getActionDefinitions();
	static QVector<KeyboardProfile::Action> getShortcutDefinitions();
	static ActionDefinition getActionDefinition(int identifier);
	static int getActionIdentifier(const QString &name);
	static bool isShortcutAllowed(const QKeySequence &shortcut, ShortcutCheck check = AllChecks, bool areSingleKeyShortcutsAllowed = true);

protected:
	explicit ActionsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void registerAction(int identifier, const QString &text, const QString &description = {}, const QIcon &icon = {}, ActionDefinition::ActionScope scope = ActionDefinition::OtherScope, ActionDefinition::ActionFlags flags = ActionDefinition::IsEnabledFlag, ActionDefinition::ActionCategory category = ActionDefinition::OtherCategory);

protected slots:
	void handleOptionChanged(int identifier);

signals:
	void shortcutsChanged();

private:
	int m_reloadTimer;

	static ActionsManager *m_instance;
	static QMap<int, QVector<QKeySequence> > m_shortcuts;
	static QMultiMap<int, QPair<QVariantMap, QVector<QKeySequence> > > m_extraShortcuts;
	static QVector<QKeySequence> m_disallowedShortcuts;
	static QVector<ActionDefinition> m_definitions;
	static int m_actionIdentifierEnumerator;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::ActionsManager::ActionDefinition::ActionFlags)

#endif
