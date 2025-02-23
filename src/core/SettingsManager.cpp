/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SettingsManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QVector>

namespace Otter
{

SettingsManager* SettingsManager::m_instance(nullptr);
QString SettingsManager::m_globalPath;
QString SettingsManager::m_overridePath;
QVector<SettingsManager::OptionDefinition> SettingsManager::m_definitions;
QHash<QString, int> SettingsManager::m_customOptions;
int SettingsManager::m_identifierCounter(-1);
int SettingsManager::m_optionIdentifierEnumerator(0);
bool SettingsManager::m_hasWildcardedOverrides(false);

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
}

void SettingsManager::createInstance(const QString &path)
{
	if (m_instance)
	{
		return;
	}

	m_instance = new SettingsManager(QCoreApplication::instance());
	m_globalPath = QDir::toNativeSeparators(path + QLatin1String("/otter.conf"));
	m_overridePath = QDir::toNativeSeparators(path + QLatin1String("/override.ini"));
	m_optionIdentifierEnumerator = staticMetaObject.indexOfEnumerator(QLatin1String("OptionIdentifier").data());
	m_identifierCounter = staticMetaObject.enumerator(m_optionIdentifierEnumerator).keyCount();

	m_definitions.reserve(m_identifierCounter);

	registerOption(AddressField_CompletionDisplayModeOption, EnumerationType, QLatin1String("compact"), {QLatin1String("compact"), QLatin1String("columns")});
	registerOption(AddressField_CompletionModeOption, EnumerationType, QLatin1String("inlineAndPopup"), {QLatin1String("none"), QLatin1String("inline"), QLatin1String("popup"), QLatin1String("inlineAndPopup")});
	registerOption(AddressField_DropActionOption, EnumerationType, QLatin1String("replace"), {QLatin1String("replace"), QLatin1String("paste"), QLatin1String("pasteAndGo")});
	registerOption(AddressField_HostLookupTimeoutOption, IntegerType, 200);
	registerOption(AddressField_LayoutOption, ListType, QStringList({QLatin1String("websiteInformation"), QLatin1String("address"), QLatin1String("fillPassword"), QLatin1String("loadPlugins"), QLatin1String("listFeeds"), QLatin1String("bookmark"), QLatin1String("historyDropdown")}));
	registerOption(AddressField_PasteAndGoOnMiddleClickOption, BooleanType, true);
	registerOption(AddressField_SelectAllOnFocusOption, BooleanType, false);
	registerOption(AddressField_ShowCompletionCategoriesOption, BooleanType, true);
	registerOption(AddressField_ShowSuggestionsOnFocusOption, BooleanType, true);
	registerOption(AddressField_SuggestBookmarksOption, BooleanType, true);
	registerOption(AddressField_SuggestHistoryOption, BooleanType, true);
	registerOption(AddressField_SuggestLocalPathsOption, BooleanType, true);
	registerOption(AddressField_SuggestSearchOption, BooleanType, true);
	registerOption(AddressField_SuggestSpecialPagesOption, BooleanType, true);
	registerOption(Backends_PasswordsOption, EnumerationType, QLatin1String("file"), {QLatin1String("file")});
	registerOption(Backends_WebOption, EnumerationType, QLatin1String("qtwebkit"), {QLatin1String("qtwebkit")}, (OptionDefinition::IsEnabledFlag | OptionDefinition::IsVisibleFlag | OptionDefinition::RequiresRestartFlag));
	registerOption(Browser_AlwaysAskWhereToSaveDownloadOption, BooleanType, true);
	registerOption(Browser_EnableMouseGesturesOption, BooleanType, true);
	registerOption(Browser_EnableSingleKeyShortcutsOption, BooleanType, true);
	registerOption(Browser_EnableSpellCheckOption, BooleanType, true);
	registerOption(Browser_EnableTrayIconOption, BooleanType, true);
	registerOption(Browser_HomePageOption, StringType, QString());
	registerOption(Browser_InactiveTabTimeUntilSuspendOption, IntegerType, -1);
	registerOption(Browser_KeyboardShortcutsProfilesOrderOption, ListType, QStringList(QLatin1String("default")));
	registerOption(Browser_LocaleOption, StringType, QLatin1String("system"));
	registerOption(Browser_MessagesOption, ListType, QStringList());
	registerOption(Browser_MigrationsOption, ListType, QStringList());
	registerOption(Browser_MouseProfilesOrderOption, ListType, QStringList(QLatin1String("default")));
	registerOption(Browser_OfflineStorageLimitOption, IntegerType, 10240);
	registerOption(Browser_OfflineWebApplicationCacheLimitOption, IntegerType, 10240);
	registerOption(Browser_OpenLinksInNewTabOption, BooleanType, true);
	registerOption(Browser_PrintElementBackgroundsOption, BooleanType, true);
	registerOption(Browser_PrivateModeOption, BooleanType, false);
	registerOption(Browser_RememberPasswordsOption, BooleanType, false);
	registerOption(Browser_ReuseCurrentTabOption, BooleanType, false);
	registerOption(Browser_ShowSelectionContextMenuOnDoubleClickOption, BooleanType, false);
	registerOption(Browser_SpellCheckDictionaryOption, ListType, QStringList());
	registerOption(Browser_SpellCheckIgnoreDctionariesOption, StringType, QStringList());
	registerOption(Browser_StartupBehaviorOption, EnumerationType, QLatin1String("continuePrevious"), {QLatin1String("continuePrevious"), QLatin1String("showDialog"), QLatin1String("startHomePage"), QLatin1String("startStartPage"), QLatin1String("startEmpty")});
	registerOption(Browser_TransferStartingActionOption, EnumerationType, QLatin1String("doNothing"), {QLatin1String("openTab"), QLatin1String("openBackgroundTab"), QLatin1String("openPanel"), QLatin1String("doNothing")});
	registerOption(Browser_ValidatorsOrderOption, ListType, QStringList({QLatin1String("w3c-markup"), QLatin1String("w3c-css")}));
	registerOption(Cache_DiskCacheLimitOption, IntegerType, 51200);
	registerOption(Cache_PagesInMemoryLimitOption, IntegerType, 5);
	registerOption(Choices_WarnFormResendOption, BooleanType, true);
	registerOption(Choices_WarnLowDiskSpaceOption, EnumerationType, QLatin1String("warn"), {QLatin1String("warn"), QLatin1String("continueReadOnly"), QLatin1String("continueReadWrite")});
	registerOption(Choices_WarnOpenBookmarkFolderOption, BooleanType, true);
	registerOption(Choices_WarnOpenMultipleDroppedUrlsOption, BooleanType, true);
	registerOption(Choices_WarnQuitOption, EnumerationType, QLatin1String("noWarn"), {QLatin1String("alwaysWarn"), QLatin1String("warnOpenTabs"), QLatin1String("noWarn")});
	registerOption(Choices_WarnQuitTransfersOption, BooleanType, true);
	registerOption(Content_BackgroundColorOption, ColorType, QColor(0xFF, 0xFF, 0xFF));
	registerOption(Content_CursiveFontOption, FontType, QLatin1String("Comic Sans MS"));
	registerOption(Content_DefaultCharacterEncodingOption, StringType, QLatin1String("auto"));
	registerOption(Content_DefaultFixedFontSizeOption, IntegerType, 16);
	registerOption(Content_DefaultFontSizeOption, IntegerType, 16);
	registerOption(Content_DefaultZoomOption, IntegerType, 100);
	registerOption(Content_FantasyFontOption, FontType, QLatin1String("Impact"));
	registerOption(Content_FixedFontOption, FontType, QLatin1String("DejaVu Sans Mono"));
	registerOption(Content_LinkColorOption, ColorType, QColor(0, 0, 0xEE));
	registerOption(Content_MinimumFontSizeOption, IntegerType, -1);
	registerOption(Content_PageReloadTimeOption, IntegerType, -1);
	registerOption(Content_SansSerifFontOption, FontType, QLatin1String("DejaVu Sans"));
	registerOption(Content_SerifFontOption, FontType, QLatin1String("DejaVu Serif"));
	registerOption(Content_StandardFontOption, FontType, QLatin1String("DejaVu Serif"));
	registerOption(Content_TextColorOption, ColorType, QColor(0, 0, 0));
	registerOption(Content_UserStyleSheetOption, PathType, QString());
	registerOption(Content_VisitedLinkColorOption, ColorType, QColor(0x55, 0x1A, 0x8B));
	registerOption(Content_ZoomTextOnlyOption, BooleanType, false);
	registerOption(ContentBlocking_EnableContentBlockingOption, BooleanType, true);
	registerOption(ContentBlocking_IgnoreHostsOption, ListType, QStringList());
	registerOption(ContentBlocking_ProfilesOption, ListType, QStringList());
	registerOption(History_BrowsingLimitAmountGlobalOption, IntegerType, 1000);
	registerOption(History_BrowsingLimitAmountWindowOption, IntegerType, 50);
	registerOption(History_BrowsingLimitPeriodOption, IntegerType, 30);
	registerOption(History_ClearOnCloseOption, ListType, QStringList());
	registerOption(History_ClosedTabsLimitAmountOption, IntegerType, 50);
	registerOption(History_ClosedWindowsLimitAmountOption, IntegerType, 10);
	registerOption(History_DownloadsLimitPeriodOption, IntegerType, 7);
	registerOption(History_ExpandBranchesOption, EnumerationType, QLatin1String("first"), {QLatin1String("first"), QLatin1String("all"), QLatin1String("none")});
	registerOption(History_ManualClearOptionsOption, ListType, QStringList({QLatin1String("browsing"), QLatin1String("cookies"), QLatin1String("forms"), QLatin1String("downloads"), QLatin1String("caches")}));
	registerOption(History_ManualClearPeriodOption, IntegerType, 1);
	registerOption(History_RememberBrowsingOption, BooleanType, true);
	registerOption(History_RememberClosedPrivateTabsOption, BooleanType, false);
	registerOption(History_RememberDownloadsOption, BooleanType, true);
	registerOption(History_StoreFaviconsOption, BooleanType, true);
	registerOption(Interface_ColorSchemeOption, ListType, QStringList(QLatin1String("default")));
	registerOption(Interface_DateTimeFormatOption, StringType, QString());
	registerOption(Interface_EnableSmoothScrollingOption, BooleanType, false);
	registerOption(Interface_EnableToolTipsOption, BooleanType, true);
	registerOption(Interface_IconThemePathOption, PathType, QString());
	registerOption(Interface_LastTabClosingActionOption, EnumerationType, QLatin1String("openTab"), {QLatin1String("openTab"), QLatin1String("closeWindow"), QLatin1String("closeWindowIfNotLast"), QLatin1String("doNothing")});
	registerOption(Interface_LockToolBarsOption, BooleanType, false);
	registerOption(Interface_NewTabOpeningActionOption, EnumerationType, QLatin1String("maximizeTab"), {QLatin1String("doNothing"), QLatin1String("maximizeTab"), QLatin1String("cascadeAll"), QLatin1String("tileAll")});
	registerOption(Interface_NotificationVisibilityDurationOption, IntegerType, 5);
	registerOption(Interface_ShowScrollBarsOption, BooleanType, true);
	registerOption(Interface_StyleSheetOption, PathType, QString());
	registerOption(Interface_TabCrashingActionOption, EnumerationType, QLatin1String("ask"), {QLatin1String("ask"), QLatin1String("close"), QLatin1String("reload")});
	registerOption(Interface_ToolTipLayoutOption, ListType, QStringList({QLatin1String("title"), QLatin1String("link")}), QStringList({QLatin1String("link"), QLatin1String("lastVisited"), QLatin1String("title")}));
	registerOption(Interface_UseFancyDateTimeFormatOption, BooleanType, true);
	registerOption(Interface_UseNativeNotificationsOption, BooleanType, true);
	registerOption(Interface_UseSystemIconThemeOption, BooleanType, false);
	registerOption(Interface_WidgetStyleOption, StringType, QString());
	registerOption(Network_AcceptLanguageOption, StringType, QLatin1String("system,*;q=0.9"));
	registerOption(Network_CookiesKeepModeOption, EnumerationType, QLatin1String("keepUntilExpires"), {QLatin1String("keepUntilExpires"), QLatin1String("keepUntilExit"), QLatin1String("ask")});
	registerOption(Network_CookiesPolicyOption, EnumerationType, QLatin1String("acceptAll"), {QLatin1String("acceptAll"), QLatin1String("acceptExisting"), QLatin1String("readOnly"), QLatin1String("ignore")});
	registerOption(Network_DoNotTrackPolicyOption, EnumerationType, QLatin1String("skip"), {QLatin1String("skip"), QLatin1String("allow"), QLatin1String("doNotAllow")});
	registerOption(Network_EnableDnsPrefetchOption, BooleanType, true);
	registerOption(Network_EnableReferrerOption, BooleanType, true);
	registerOption(Network_ProxyOption, EnumerationType, QLatin1String("system"), {QLatin1String("system")});
	registerOption(Network_ThirdPartyCookiesAcceptedHostsOption, ListType, QStringList());
	registerOption(Network_ThirdPartyCookiesPolicyOption, EnumerationType, QLatin1String("ignore"), QStringList({QLatin1String("acceptAll"), QLatin1String("acceptExisting"), QLatin1String("ignore")}));
	registerOption(Network_ThirdPartyCookiesRejectedHostsOption, ListType, QStringList());
	registerOption(Network_UserAgentOption, EnumerationType, QLatin1String("default"), QStringList(QLatin1String("default")));
	registerOption(Network_WorkOfflineOption, BooleanType, false);
	registerOption(Paths_DownloadsOption, PathType, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	registerOption(Paths_OpenFileOption, PathType, Utils::getStandardLocation(QStandardPaths::HomeLocation));
	registerOption(Paths_SaveFileOption, PathType, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	registerOption(Permissions_EnableFullScreenOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableGeolocationOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableImagesOption, EnumerationType, QLatin1String("enabled"), QStringList({QLatin1String("enabled"), QLatin1String("onlyCached"), QLatin1String("disabled")}));
	registerOption(Permissions_EnableJavaScriptOption, BooleanType, true);
	registerOption(Permissions_EnableLocalStorageOption, BooleanType, true);
	registerOption(Permissions_EnableMediaCaptureAudioOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableMediaCaptureVideoOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableMediaPlaybackAudioOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableNotificationsOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableOfflineStorageDatabaseOption, BooleanType, false);
	registerOption(Permissions_EnableOfflineWebApplicationCacheOption, BooleanType, false);
	registerOption(Permissions_EnablePluginsOption, EnumerationType, QLatin1String("onDemand"), QStringList({QLatin1String("enabled"), QLatin1String("onDemand"), QLatin1String("disabled")}));
	registerOption(Permissions_EnablePointerLockOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableWebglOption, BooleanType, true);
	registerOption(Permissions_ScriptsCanAccessClipboardOption, BooleanType, false);
	registerOption(Permissions_ScriptsCanChangeWindowGeometryOption, BooleanType, true);
	registerOption(Permissions_ScriptsCanCloseSelfOpenedWindowsOption, BooleanType, true);
	registerOption(Permissions_ScriptsCanCloseWindowsOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_ScriptsCanOpenWindowsOption, EnumerationType, QLatin1String("ask"), QStringList({QLatin1String("ask"), QLatin1String("blockAll"), QLatin1String("openAll"), QLatin1String("openAllInBackground")}));
	registerOption(Permissions_ScriptsCanReceiveRightClicksOption, BooleanType, true);
	registerOption(Permissions_ScriptsCanShowStatusMessagesOption, BooleanType, false);
	registerOption(Search_DefaultQuickSearchEngineOption, EnumerationType, QLatin1String("duckduckgo"), QStringList(QLatin1String("duckduckgo")));
	registerOption(Search_DefaultSearchEngineOption, EnumerationType, QLatin1String("duckduckgo"), QStringList(QLatin1String("duckduckgo")));
	registerOption(Search_EnableFindInPageAsYouTypeOption, BooleanType, true);
	registerOption(Search_EnableFindInPageHighlightAllOption, BooleanType, false);
	registerOption(Search_ReuseLastQuickFindQueryOption, BooleanType, false);
	registerOption(Search_SearchEnginesOrderOption, ListType, QStringList({QLatin1String("duckduckgo"), QLatin1String("wikipedia"), QLatin1String("startpage"), QLatin1String("google"), QLatin1String("yahoo"), QLatin1String("bing"), QLatin1String("youtube")}));
	registerOption(Search_SearchEnginesSuggestionsModeOption, EnumerationType, QLatin1String("nonPrivateTabsOnly"), QStringList({QLatin1String("enabled"), QLatin1String("nonPrivateTabsOnly"), QLatin1String("disabled")}));
	registerOption(Security_AllowMixedContentOption, BooleanType, false);
	registerOption(Security_CiphersOption, ListType, QStringList(QLatin1String("default")));
	registerOption(Security_EnableFraudCheckingOption, BooleanType, true);
	registerOption(Security_IgnoreSslErrorsOption, ListType, QStringList());
	registerOption(Sessions_DeferTabsLoadingOption, BooleanType, true);
	registerOption(Sessions_OpenInExistingWindowOption, BooleanType, false);
	registerOption(Sessions_OptionsExludedFromInheritingOption, ListType, QStringList(QLatin1String("Content/PageReloadTime")));
	registerOption(Sessions_OptionsExludedFromSavingOption, ListType, QStringList());
	registerOption(SourceViewer_ShowLineNumbersOption, BooleanType, true);
	registerOption(SourceViewer_WrapLinesOption, BooleanType, false);
	registerOption(StartPage_BackgroundColorOption, ColorType, QColor());
	registerOption(StartPage_BackgroundModeOption, EnumerationType, QLatin1String("standard"), QStringList({QLatin1String("standard"), QLatin1String("none"), QLatin1String("bestFit"), QLatin1String("center"), QLatin1String("stretch"), QLatin1String("tile")}));
	registerOption(StartPage_BackgroundPathOption, StringType, QString());
	registerOption(StartPage_BookmarksFolderOption, StringType, QLatin1String("/Start Page/"));
	registerOption(StartPage_EnableStartPageOption, BooleanType, true);
	registerOption(StartPage_ShowAddTileOption, BooleanType, true);
	registerOption(StartPage_ShowSearchFieldOption, BooleanType, true);
	registerOption(StartPage_TileBackgroundModeOption, EnumerationType, QLatin1String("thumbnail"), QStringList({QLatin1String("none"), QLatin1String("thumbnail"), QLatin1String("favicon")}));
	registerOption(StartPage_TileHeightOption, IntegerType, 190);
	registerOption(StartPage_TileWidthOption, IntegerType, 270);
	registerOption(StartPage_TilesPerRowOption, IntegerType, 0);
	registerOption(StartPage_ZoomLevelOption, IntegerType, 100);
	registerOption(TabBar_EnablePreviewsOption, BooleanType, true);
	registerOption(TabBar_EnableThumbnailsOption, BooleanType, false);
	registerOption(TabBar_MaximumTabHeightOption, IntegerType, -1);
	registerOption(TabBar_MinimumTabHeightOption, IntegerType, -1);
	registerOption(TabBar_MaximumTabWidthOption, IntegerType, 250);
	registerOption(TabBar_MinimumTabWidthOption, IntegerType, -1);
	registerOption(TabBar_OpenNextToActiveOption, BooleanType, true);
	registerOption(TabBar_PrependPinnedTabOption, BooleanType, true);
	registerOption(TabBar_PreviewsAnimationDurationOption, IntegerType, 250);
	registerOption(TabBar_RequireModifierToSwitchTabOnScrollOption, BooleanType, true);
	registerOption(TabBar_ShowCloseButtonOption, BooleanType, true);
	registerOption(TabBar_ShowUrlIconOption, BooleanType, true);
	registerOption(TabSwitcher_IgnoreMinimizedTabsOption, BooleanType, true);
	registerOption(TabSwitcher_OrderByLastActivityOption, BooleanType, true);
	registerOption(TabSwitcher_ShowListOption, BooleanType, true);
	registerOption(Updates_ActiveChannelsOption, ListType, QStringList());
	registerOption(Updates_AutomaticInstallOption, BooleanType, false);
	registerOption(Updates_CheckIntervalOption, IntegerType, 7);
	registerOption(Updates_LastCheckOption, StringType, QString());
	registerOption(Updates_ServerUrlOption, StringType, QLatin1String("https://www.otter-browser.org/updates/update.json"));

	const QStringList hosts(getOverrideHosts());

	for (int i = 0; i < hosts.count(); ++i)
	{
		if (hosts.at(i).startsWith(QLatin1Char('*')))
		{
			m_hasWildcardedOverrides = true;

			break;
		}
	}
}

void SettingsManager::removeOverride(const QString &host, int identifier)
{
	QSettings settings(m_overridePath, QSettings::IniFormat);

	if (identifier >= 0)
	{
		settings.remove(host + QLatin1Char('/') + getOptionName(identifier));
		settings.sync();

		emit m_instance->hostOptionChanged(identifier, getOption(identifier), host);

		return;
	}

	settings.beginGroup(host);

	const QStringList groups(settings.childGroups());

	if (groups.isEmpty())
	{
		return;
	}

	QVector<int> options;
	options.reserve(groups.count());

	for (int i = 0; i < groups.count(); ++i)
	{
		const QString group(groups.at(i));

		settings.beginGroup(group);

		const QStringList rawOptions(settings.childKeys());

		for (int j = 0; j < rawOptions.count(); ++j)
		{
			const int option(getOptionIdentifier(group + QLatin1Char('/') + rawOptions.at(j)));

			if (option >= 0)
			{
				options.append(option);
			}
		}

		settings.endGroup();
	}

	settings.endGroup();
	settings.remove(host);
	settings.sync();

	for (int i = 0; i < options.count(); ++i)
	{
		emit m_instance->hostOptionChanged(identifier, getOption(options.at(i)), host);
	}
}

void SettingsManager::registerOption(int identifier, OptionType type, const QVariant &defaultValue, const QStringList &choices, OptionDefinition::OptionFlags flags)
{
	OptionDefinition definition;
	definition.defaultValue = defaultValue;
	definition.setChoices(choices);
	definition.type = type;
	definition.flags = flags;
	definition.identifier = identifier;

	m_definitions.append(definition);
}

void SettingsManager::saveOption(const QString &path, const QString &key, const QVariant &value, OptionType type)
{
	QSettings settings(path, QSettings::IniFormat);

	if (value.isNull())
	{
		settings.remove(key);
	}
	else if (type == ColorType)
	{
		const QColor color(value.value<QColor>());

		settings.setValue(key, (color.isValid() ? color.name(QColor::HexArgb).toUpper() : QString()));
	}
	else
	{
		settings.setValue(key, value);
	}
}

void SettingsManager::updateOptionDefinition(int identifier, const SettingsManager::OptionDefinition &definition)
{
	if (identifier >= 0 && identifier < m_definitions.count())
	{
		m_definitions[identifier].defaultValue = definition.defaultValue;
		m_definitions[identifier].choices = definition.choices;
	}
}

void SettingsManager::setOption(int identifier, const QVariant &value, const QString &host)
{
	const QString name(getOptionName(identifier));
	const OptionType type(getOptionDefinition(identifier).type);

	if (!host.isEmpty())
	{
		const QString overrideName(host + QLatin1Char('/') + name);

		if (value.isNull())
		{
			QSettings(m_overridePath, QSettings::IniFormat).remove(overrideName);
		}
		else
		{
			saveOption(m_overridePath, overrideName, value, type);
		}

		if (!m_hasWildcardedOverrides && overrideName.startsWith(QLatin1Char('*')))
		{
			m_hasWildcardedOverrides = true;
		}

		emit m_instance->hostOptionChanged(identifier, value, host);

		return;
	}

	if (getOption(identifier) != value)
	{
		saveOption(m_globalPath, name, value, type);

		emit m_instance->optionChanged(identifier, value);
	}
}

SettingsManager* SettingsManager::getInstance()
{
	return m_instance;
}

QString SettingsManager::createDisplayValue(int identifier, const QVariant &value)
{
	const OptionDefinition definition(getOptionDefinition(identifier));

	switch (definition.type)
	{
		case BooleanType:
			return (value.toBool() ? tr("Yes") : tr("No"));
		case ColorType:
			{
				const QColor color(value.value<QColor>());

				return (color.isValid() ? color.name((color.alpha() < 255) ? QColor::HexArgb : QColor::HexRgb).toUpper() : tr("Invalid"));
			}
		case EnumerationType:
			{
				const QString key(value.toString());

				for (int i = 0; i < definition.choices.count(); ++i)
				{
					const OptionDefinition::Choice choice(definition.choices.at(i));

					if (choice.value == key)
					{
						return choice.getTitle();
					}
				}
			}

			break;
		case ListType:
			return value.toStringList().join(QLatin1String(", "));
		case PasswordType:
			if (!value.toString().isEmpty())
			{
				return QString(5, QChar(0x2022));
			}

			break;
		case PathType:
			return QDir::toNativeSeparators(value.toString());
		default:
			break;
	}

	return value.toString();
}

QString SettingsManager::getGlobalPath()
{
	return m_globalPath;
}

QString SettingsManager::getOverridePath()
{
	return m_overridePath;
}

QString SettingsManager::getOptionName(int identifier)
{
	QString name(staticMetaObject.enumerator(m_optionIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(6);

		return name.replace(QLatin1Char('_'), QLatin1Char('/'));
	}

	return m_customOptions.key(identifier);
}

QVariant SettingsManager::getOption(int identifier, const QString &host)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return {};
	}

	const QString name(getOptionName(identifier));

	if (host.isEmpty())
	{
		return QSettings(m_globalPath, QSettings::IniFormat).value(name, m_definitions.at(identifier).defaultValue);
	}

	const QString overrideName(host + QLatin1Char('/') + name);
	const QSettings overrides(m_overridePath, QSettings::IniFormat);

	if (m_hasWildcardedOverrides && !overrides.contains(overrideName))
	{
		const QStringList hostParts(host.split(QLatin1Char('.')));

		for (int i = 1; i < hostParts.count(); ++i)
		{
			const QString wildcardedName(QLatin1String("*.") + hostParts.mid(i).join(QLatin1Char('.')) + QLatin1Char('/') + name);

			if (overrides.contains(wildcardedName))
			{
				return QSettings(m_overridePath, QSettings::IniFormat).value(wildcardedName, getOption(identifier));
			}
		}
	}

	return QSettings(m_overridePath, QSettings::IniFormat).value(overrideName, getOption(identifier));
}

QStringList SettingsManager::getOptions()
{
	QStringList options;
	options.reserve(m_definitions.count());

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		options.append(getOptionName(i));
	}

	options.sort();

	return options;
}

QStringList SettingsManager::getOverrideHosts(int identifier)
{
	if (identifier < 0)
	{
		return QSettings(m_overridePath, QSettings::IniFormat).childGroups();
	}

	const QSettings overrides(m_overridePath, QSettings::IniFormat);
	const QStringList overridesGroups(overrides.childGroups());
	QStringList hosts;
	const QString optionName(getOptionName(identifier));

	for (int i = 0; i < overridesGroups.count(); ++i)
	{
		const QString group(overridesGroups.at(i));

		if (overrides.contains(group + QLatin1Char('/') + optionName))
		{
			hosts.append(group);
		}
	}

	return hosts;
}

QStringList SettingsManager::getOverridesHierarchy(const QString &host)
{
	QStringList hierarchy;

	if (!m_hasWildcardedOverrides)
	{
		return hierarchy;
	}

	const QStringList groups(QSettings(m_overridePath, QSettings::IniFormat).childGroups());
	const QStringList hostParts(host.split(QLatin1Char('.')));

	for (int i = 0; i < hostParts.count(); ++i)
	{
		const QString explicitHost(hostParts.mid(i).join(QLatin1Char('.')));

		if (i > 0)
		{
			const QString wildcardedHost(QLatin1String("*.") + explicitHost);

			if (groups.contains(wildcardedHost))
			{
				hierarchy.append(wildcardedHost);
			}
		}

		if (groups.contains(explicitHost))
		{
			hierarchy.append(explicitHost);
		}
	}

	return hierarchy;
}

DiagnosticReport::Section SettingsManager::createReport()
{
	QHash<QString, int> overridenValues;
	QSettings overrides(m_overridePath, QSettings::IniFormat);
	const QStringList overridesGroups(overrides.childGroups());

	for (int i = 0; i < overridesGroups.count(); ++i)
	{
		overrides.beginGroup(overridesGroups.at(i));

		const QStringList keys(overrides.allKeys());

		for (int j = 0; j < keys.count(); ++j)
		{
			const QString key(keys.at(j));

			if (overridenValues.contains(key))
			{
				++overridenValues[key];
			}
			else
			{
				overridenValues[key] = 1;
			}
		}

		overrides.endGroup();
	}

	const QStringList options(getOptions());
	DiagnosticReport::Section report;
	report.title = QLatin1String("Settings");
	report.fieldWidths = {50, 20, 20, 0};
	report.entries.reserve(options.count());

	for (int i = 0; i < options.count(); ++i)
	{
		const QString option(options.at(i));
		const OptionDefinition definition(getOptionDefinition(getOptionIdentifier(option)));
		QString value;

		switch (definition.type)
		{
			case ListType:
			case PathType:
			case StringType:
				value = QLatin1Char('-');

				break;
			default:
				value = definition.defaultValue.toString();

				break;
		}

		report.entries.append({option, value, (isDefault(definition.identifier) ? QLatin1String("default") : QLatin1String("non default")), (overridenValues.contains(option) ? QStringLiteral("%1 override(s)").arg(overridenValues[option]) : QLatin1String("no overrides"))});
	}

	return report;
}

SettingsManager::OptionDefinition SettingsManager::getOptionDefinition(int identifier)
{
	if (identifier >= 0 && identifier < m_definitions.count())
	{
		return m_definitions.at(identifier);
	}

	return {};
}

int SettingsManager::registerOption(const QString &name, OptionType type, const QVariant &defaultValue, const QStringList &choices, OptionDefinition::OptionFlags flags)
{
	if (name.isEmpty() || getOptionIdentifier(name) >= 0)
	{
		return -1;
	}

	const int identifier(m_identifierCounter);

	++m_identifierCounter;

	OptionDefinition definition;
	definition.defaultValue = defaultValue;
	definition.setChoices(choices);
	definition.type = type;
	definition.flags = flags;
	definition.identifier = identifier;

	m_customOptions[name] = identifier;

	m_definitions.append(definition);

	return identifier;
}

int SettingsManager::getOptionIdentifier(const QString &name)
{
	QString mutableName(name);
	mutableName.replace(QLatin1Char('/'), QLatin1Char('_'));

	if (!name.endsWith(QLatin1String("Option")))
	{
		mutableName.append(QLatin1String("Option"));
	}

	if (m_customOptions.contains(name))
	{
		return m_customOptions[name];
	}

	return staticMetaObject.enumerator(m_optionIdentifierEnumerator).keyToValue(mutableName.toLatin1());
}

int SettingsManager::getOverridesCount(int identifier)
{
	const QSettings overrides(m_overridePath, QSettings::IniFormat);
	const QStringList overridesGroups(overrides.childGroups());
	const QString optionName(getOptionName(identifier));
	int amount(0);

	for (int i = 0; i < overridesGroups.count(); ++i)
	{
		if (overrides.contains(overridesGroups.at(i) + QLatin1Char('/') + optionName))
		{
			++amount;
		}
	}

	return amount;
}

bool SettingsManager::hasOverride(const QString &host, int identifier)
{
	if (identifier < 0)
	{
		return QSettings(m_overridePath, QSettings::IniFormat).childGroups().contains(host);
	}

	return QSettings(m_overridePath, QSettings::IniFormat).contains(host + QLatin1Char('/') + getOptionName(identifier));
}

bool SettingsManager::isDefault(int identifier)
{
	return (getOption(identifier) == getOptionDefinition(identifier).defaultValue);
}

}
