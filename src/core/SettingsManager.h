/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_SETTINGSMANAGER_H
#define OTTER_SETTINGSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

namespace Otter
{

class SettingsManager : public QObject
{
	Q_OBJECT
	Q_ENUMS(OptionIdentifier)
	Q_ENUMS(OptionType)

public:
	enum OptionIdentifier
	{
		AddressField_CompletionDisplayModeOption = 0,
		AddressField_CompletionModeOption,
		AddressField_DropActionOption,
		AddressField_EnableHistoryDropdownOption,
		AddressField_HostLookupTimeoutOption,
		AddressField_PasteAndGoOnMiddleClickOption,
		AddressField_SelectAllOnFocusOption,
		AddressField_ShowBookmarkIconOption,
		AddressField_ShowCompletionCategoriesOption,
		AddressField_ShowFeedsIconOption,
		AddressField_ShowLoadPluginsIconOption,
		AddressField_ShowUrlIconOption,
		AddressField_SuggestBookmarksOption,
		AddressField_SuggestHistoryOption,
		AddressField_SuggestLocalPathsOption,
		AddressField_SuggestSearchOption,
		AddressField_SuggestSpecialPagesOption,
		Backends_PasswordsOption,
		Backends_WebOption,
		Browser_AlwaysAskWhereToSaveDownloadOption,
		Browser_DelayRestoringOfBackgroundTabsOption,
		Browser_EnableFullScreenOption,
		Browser_EnableGeolocationOption,
		Browser_EnableImagesOption,
		Browser_EnableJavaOption,
		Browser_EnableJavaScriptOption,
		Browser_EnableLocalStorageOption,
		Browser_EnableMediaCaptureAudioOption,
		Browser_EnableMediaCaptureVideoOption,
		Browser_EnableMediaPlaybackAudioOption,
		Browser_EnableMouseGesturesOption,
		Browser_EnableNotificationsOption,
		Browser_EnableOfflineStorageDatabaseOption,
		Browser_EnableOfflineWebApplicationCacheOption,
		Browser_EnablePluginsOption,
		Browser_EnablePointerLockOption,
		Browser_EnableSingleKeyShortcutsOption,
		Browser_EnableSpellCheckOption,
		Browser_EnableTrayIconOption,
		Browser_EnableWebglOption,
		Browser_HomePageOption,
		Browser_JavaScriptCanAccessClipboardOption,
		Browser_JavaScriptCanChangeWindowGeometryOption,
		Browser_JavaScriptCanCloseWindowsOption,
		Browser_JavaScriptCanDisableContextMenuOption,
		Browser_JavaScriptCanOpenWindowsOption,
		Browser_JavaScriptCanShowStatusMessagesOption,
		Browser_KeyboardShortcutsProfilesOrderOption,
		Browser_LocaleOption,
		Browser_MigrationsOption,
		Browser_MouseProfilesOrderOption,
		Browser_OfflineStorageLimitOption,
		Browser_OfflineWebApplicationCacheLimitOption,
		Browser_OpenLinksInNewTabOption,
		Browser_PrivateModeOption,
		Browser_RememberPasswordsOption,
		Browser_ReuseCurrentTabOption,
		Browser_ShowSelectionContextMenuOnDoubleClickOption,
		Browser_SpellCheckDictionaryOption,
		Browser_StartupBehaviorOption,
		Browser_TabCrashingActionOption,
		Browser_ToolTipsModeOption,
		Browser_TransferStartingActionOption,
		Cache_DiskCacheLimitOption,
		Cache_PagesInMemoryLimitOption,
		Choices_WarnFormResendOption,
		Choices_WarnLowDiskSpaceOption,
		Choices_WarnOpenBookmarkFolderOption,
		Choices_WarnOpenMultipleDroppedUrlsOption,
		Choices_WarnQuitOption,
		Choices_WarnQuitTransfersOption,
		Content_BackgroundColorOption,
		Content_CursiveFontOption,
		Content_DefaultCharacterEncodingOption,
		Content_DefaultFixedFontSizeOption,
		Content_DefaultFontSizeOption,
		Content_DefaultZoomOption,
		Content_FantasyFontOption,
		Content_FixedFontOption,
		Content_LinkColorOption,
		Content_MinimumFontSizeOption,
		Content_PageReloadTimeOption,
		Content_PopupsPolicyOption,
		Content_SansSerifFontOption,
		Content_SerifFontOption,
		Content_StandardFontOption,
		Content_TextColorOption,
		Content_UserStyleSheetOption,
		Content_VisitedLinkColorOption,
		Content_ZoomTextOnlyOption,
		ContentBlocking_CosmeticFiltersModeOption,
		ContentBlocking_EnableContentBlockingOption,
		ContentBlocking_EnableWildcardsOption,
		ContentBlocking_ProfilesOption,
		History_BrowsingLimitAmountGlobalOption,
		History_BrowsingLimitAmountWindowOption,
		History_BrowsingLimitPeriodOption,
		History_ClearOnCloseOption,
		History_DownloadsLimitPeriodOption,
		History_ExpandBranchesOption,
		History_ManualClearOptionsOption,
		History_ManualClearPeriodOption,
		History_RememberBrowsingOption,
		History_RememberDownloadsOption,
		History_StoreFaviconsOption,
		Interface_DateTimeFormatOption,
		Interface_IconThemePathOption,
		Interface_LastTabClosingActionOption,
		Interface_LockToolBarsOption,
		Interface_MaximizeNewWindowsOption,
		Interface_NewTabOpeningActionOption,
		Interface_NotificationVisibilityDurationOption,
		Interface_ShowScrollBarsOption,
		Interface_ShowSizeGripOption,
		Interface_StyleSheetOption,
		Interface_UseNativeNotificationsOption,
		Interface_UseSystemIconThemeOption,
		Interface_WidgetStyleOption,
		Network_AcceptLanguageOption,
		Network_CookiesKeepModeOption,
		Network_CookiesPolicyOption,
		Network_DoNotTrackPolicyOption,
		Network_EnableReferrerOption,
		Network_ProxyModeOption,
		Network_ThirdPartyCookiesAcceptedHostsOption,
		Network_ThirdPartyCookiesPolicyOption,
		Network_ThirdPartyCookiesRejectedHostsOption,
		Network_UserAgentOption,
		Network_WorkOfflineOption,
		Paths_DownloadsOption,
		Paths_OpenFileOption,
		Paths_SaveFileOption,
		Proxy_AutomaticConfigurationPathOption,
		Proxy_CommonPortOption,
		Proxy_CommonServersOption,
		Proxy_ExceptionsOption,
		Proxy_FtpPortOption,
		Proxy_FtpServersOption,
		Proxy_HttpPortOption,
		Proxy_HttpServersOption,
		Proxy_HttpsPortOption,
		Proxy_HttpsServersOption,
		Proxy_SocksPortOption,
		Proxy_SocksServersOption,
		Proxy_UseCommonOption,
		Proxy_UseFtpOption,
		Proxy_UseHttpOption,
		Proxy_UseHttpsOption,
		Proxy_UseSocksOption,
		Proxy_UseSystemAuthenticationOption,
		Search_DefaultQuickSearchEngineOption,
		Search_DefaultSearchEngineOption,
		Search_EnableFindInPageAsYouTypeOption,
		Search_ReuseLastQuickFindQueryOption,
		Search_SearchEnginesOrderOption,
		Search_SearchEnginesSuggestionsOption,
		Security_CiphersOption,
		Security_IgnoreSslErrorsOption,
		Sessions_OpenInExistingWindowOption,
		Sessions_OptionsExludedFromInheritingOption,
		Sessions_OptionsExludedFromSavingOption,
		Sidebar_CurrentPanelOption,
		Sidebar_PanelsOption,
		Sidebar_ReverseOption,
		Sidebar_ShowToggleEdgeOption,
		Sidebar_VisibleOption,
		Sidebar_WidthOption,
		SourceViewer_ShowLineNumbersOption,
		SourceViewer_WrapLinesOption,
		StartPage_BackgroundColorOption,
		StartPage_BackgroundModeOption,
		StartPage_BackgroundPathOption,
		StartPage_BookmarksFolderOption,
		StartPage_EnableStartPageOption,
		StartPage_ShowAddTileOption,
		StartPage_ShowSearchFieldOption,
		StartPage_TileBackgroundModeOption,
		StartPage_TileHeightOption,
		StartPage_TileWidthOption,
		StartPage_TilesPerRowOption,
		StartPage_ZoomLevelOption,
		TabBar_EnablePreviewsOption,
		TabBar_EnableThumbnailsOption,
		TabBar_MaximumTabHeightOption,
		TabBar_MinimumTabHeightOption,
		TabBar_MaximumTabWidthOption,
		TabBar_MinimumTabWidthOption,
		TabBar_OpenNextToActiveOption,
		TabBar_PreviewsAnimationDurationOption,
		TabBar_RequireModifierToSwitchTabOnScrollOption,
		TabBar_ShowCloseButtonOption,
		TabBar_ShowUrlIconOption,
		Updates_ActiveChannelsOption,
		Updates_AutomaticInstallOption,
		Updates_CheckIntervalOption,
		Updates_LastCheckOption,
		Updates_ServerUrlOption
	};

	enum OptionType
	{
		UnknownType = 0,
		BooleanType,
		ColorType,
		EnumerationType,
		FontType,
		IconType,
		IntegerType,
		ListType,
		PathType,
		PasswordType,
		StringType
	};

	enum OptionFlag
	{
		NoFlags = 0,
		IsEnabledFlag = 1,
		IsVisibleFlag = 2,
		IsBuiltInFlag = 4
	};

	Q_DECLARE_FLAGS(OptionFlags, OptionFlag)

	struct OptionDefinition
	{
		QVariant defaultValue;
		QStringList choices;
		OptionType type = UnknownType;
		OptionFlags flags = static_cast<OptionFlags>(IsEnabledFlag | IsVisibleFlag);
		int identifier = -1;
	};

	static void createInstance(const QString &path, QObject *parent = nullptr);
	static void removeOverride(const QUrl &url, const QString &key = QString());
	static void updateOptionDefinition(int identifier, const OptionDefinition &definition);
	static void setValue(int identifier, const QVariant &value, const QUrl &url = QUrl());
	static SettingsManager* getInstance();
	static QString getOptionName(int identifier);
	static QString getReport();
	static QVariant getValue(int identifier, const QUrl &url = QUrl());
	static QStringList getOptions();
	static OptionDefinition getOptionDefinition(int identifier);
	static int registerOption(const QString &name, const QVariant &defaultValue, OptionType type, const QStringList &choices = QStringList());
	static int getOptionIdentifier(const QString &name);
	static bool hasOverride(const QUrl &url, int identifier = -1);

protected:
	explicit SettingsManager(QObject *parent = nullptr);

	static QString getHost(const QUrl &url);
	static void registerOption(int identifier, const QVariant &defaultValue, OptionType type, const QStringList &choices = QStringList());

private:
	static SettingsManager *m_instance;
	static QString m_globalPath;
	static QString m_overridePath;
	static QVector<OptionDefinition> m_definitions;
	static QHash<QString, int> m_customOptions;
	static int m_identifierCounter;
	static int m_optionIdentifierEnumerator;
	static bool m_hasWildcardedOverrides;

signals:
	void valueChanged(int identifier, const QVariant &value);
	void valueChanged(int identifier, const QVariant &value, const QUrl &url);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::SettingsManager::OptionFlags)

#endif
