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

#include "SettingsManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
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

void SettingsManager::createInstance(const QString &path, QObject *parent)
{
	if (m_instance)
	{
		return;
	}

	m_instance = new SettingsManager(parent);
	m_globalPath = path + QLatin1String("/otter.conf");
	m_overridePath = path + QLatin1String("/override.ini");
	m_optionIdentifierEnumerator = m_instance->metaObject()->indexOfEnumerator(QLatin1String("OptionIdentifier").data());
	m_identifierCounter = m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).keyCount();

	m_definitions.reserve(m_identifierCounter);

	registerOption(AddressField_CompletionDisplayModeOption, QLatin1String("compact"), EnumerationType, QStringList({QLatin1String("compact"), QLatin1String("columns")}));
	registerOption(AddressField_CompletionModeOption, QLatin1String("inlineAndPopup"), EnumerationType, QStringList({QLatin1String("none"), QLatin1String("inline"), QLatin1String("popup"), QLatin1String("inlineAndPopup")}));
	registerOption(AddressField_DropActionOption, QLatin1String("replace"), EnumerationType, QStringList({QLatin1String("replace"), QLatin1String("paste"), QLatin1String("pasteAndGo")}));
	registerOption(AddressField_HostLookupTimeoutOption, 200, IntegerType);
	registerOption(AddressField_LayoutOption, QStringList({QLatin1String("websiteInformation"), QLatin1String("address"), QLatin1String("fillPassword"), QLatin1String("loadPlugins"), QLatin1String("listFeeds"), QLatin1String("bookmark"), QLatin1String("historyDropdown")}), ListType);
	registerOption(AddressField_PasteAndGoOnMiddleClickOption, true, BooleanType);
	registerOption(AddressField_SelectAllOnFocusOption, true, BooleanType);
	registerOption(AddressField_ShowCompletionCategoriesOption, true, BooleanType);
	registerOption(AddressField_SuggestBookmarksOption, true, BooleanType);
	registerOption(AddressField_SuggestHistoryOption, true, BooleanType);
	registerOption(AddressField_SuggestLocalPathsOption, true, BooleanType);
	registerOption(AddressField_SuggestSearchOption, true, BooleanType);
	registerOption(AddressField_SuggestSpecialPagesOption, true, BooleanType);
	registerOption(Backends_PasswordsOption, QLatin1String("file"), EnumerationType, QStringList(QLatin1String("file")));
	registerOption(Backends_WebOption, QLatin1String("qtwebkit"), EnumerationType, QStringList(QLatin1String("qtwebkit")));
	registerOption(Browser_AlwaysAskWhereToSaveDownloadOption, true, BooleanType);
	registerOption(Browser_DelayRestoringOfBackgroundTabsOption, true, BooleanType);
	registerOption(Browser_EnableMouseGesturesOption, true, BooleanType);
	registerOption(Browser_EnableSingleKeyShortcutsOption, true, BooleanType);
	registerOption(Browser_EnableSpellCheckOption, true, BooleanType);
	registerOption(Browser_EnableTrayIconOption, true, BooleanType);
	registerOption(Browser_HomePageOption, QString(), StringType);
	registerOption(Browser_InactiveTabTimeUntilSuspendOption, -1, IntegerType);
	registerOption(Browser_KeyboardShortcutsProfilesOrderOption, QStringList({QLatin1String("platform"), QLatin1String("default")}), ListType);
	registerOption(Browser_LocaleOption, QLatin1String("system"), StringType);
	registerOption(Browser_MigrationsOption, QStringList(), ListType);
	registerOption(Browser_MouseProfilesOrderOption, QStringList(QLatin1String("default")), ListType);
	registerOption(Browser_OfflineStorageLimitOption, 10240, IntegerType);
	registerOption(Browser_OfflineWebApplicationCacheLimitOption, 10240, IntegerType);
	registerOption(Browser_OpenLinksInNewTabOption, true, BooleanType);
	registerOption(Browser_PrivateModeOption, false, BooleanType);
	registerOption(Browser_RememberPasswordsOption, false, BooleanType);
	registerOption(Browser_ReuseCurrentTabOption, false, BooleanType);
	registerOption(Browser_ShowSelectionContextMenuOnDoubleClickOption, false, BooleanType);
	registerOption(Browser_SpellCheckDictionaryOption, QString(), StringType);
	registerOption(Browser_StartupBehaviorOption, QLatin1String("continuePrevious"), EnumerationType, QStringList({QLatin1String("continuePrevious"), QLatin1String("showDialog"), QLatin1String("startHomePage"), QLatin1String("startStartPage"), QLatin1String("startEmpty")}));
	registerOption(Browser_TabCrashingActionOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("close"), QLatin1String("reload")}));
	registerOption(Browser_ToolTipsModeOption, QLatin1String("extended"), EnumerationType, QStringList({QLatin1String("disabled"), QLatin1String("standard"), QLatin1String("extended")}));
	registerOption(Browser_TransferStartingActionOption, QLatin1String("openTab"), EnumerationType, QStringList({QLatin1String("openTab"), QLatin1String("openBackgroundTab"), QLatin1String("openPanel"), QLatin1String("doNothing")}));
	registerOption(Cache_DiskCacheLimitOption, 51200, IntegerType);
	registerOption(Cache_PagesInMemoryLimitOption, 5, IntegerType);
	registerOption(Choices_WarnFormResendOption, true, BooleanType);
	registerOption(Choices_WarnLowDiskSpaceOption, QLatin1String("warn"), EnumerationType, QStringList({QLatin1String("warn"), QLatin1String("continueReadOnly"), QLatin1String("continueReadWrite")}));
	registerOption(Choices_WarnOpenBookmarkFolderOption, true, BooleanType);
	registerOption(Choices_WarnOpenMultipleDroppedUrlsOption, true, BooleanType);
	registerOption(Choices_WarnQuitOption, QLatin1String("noWarn"), EnumerationType, QStringList({QLatin1String("alwaysWarn"), QLatin1String("warnOpenTabs"), QLatin1String("noWarn")}));
	registerOption(Choices_WarnQuitTransfersOption, true, BooleanType);
	registerOption(Content_BackgroundColorOption, QLatin1String("#FFFFFF"), ColorType);
	registerOption(Content_CursiveFontOption, QLatin1String("Impact"), FontType);
	registerOption(Content_DefaultCharacterEncodingOption, QLatin1String("auto"), StringType);
	registerOption(Content_DefaultFixedFontSizeOption, 16, IntegerType);
	registerOption(Content_DefaultFontSizeOption, 16, IntegerType);
	registerOption(Content_DefaultZoomOption, 100, IntegerType);
	registerOption(Content_FantasyFontOption, QLatin1String("Comic Sans MS"), FontType);
	registerOption(Content_FixedFontOption, QLatin1String("DejaVu Sans Mono"), FontType);
	registerOption(Content_LinkColorOption, QLatin1String("#0000EE"), ColorType);
	registerOption(Content_MinimumFontSizeOption, -1, IntegerType);
	registerOption(Content_PageReloadTimeOption, -1, IntegerType);
	registerOption(Content_SansSerifFontOption, QLatin1String("DejaVu Sans"), FontType);
	registerOption(Content_SerifFontOption, QLatin1String("DejaVu Serif"), FontType);
	registerOption(Content_StandardFontOption, QLatin1String("DejaVu Serif"), FontType);
	registerOption(Content_TextColorOption, QLatin1String("#000000"), ColorType);
	registerOption(Content_UserStyleSheetOption, QString(), PathType);
	registerOption(Content_VisitedLinkColorOption, QLatin1String("#551A8B"), ColorType);
	registerOption(Content_ZoomTextOnlyOption, false, BooleanType);
	registerOption(ContentBlocking_CosmeticFiltersModeOption, QLatin1String("all"), EnumerationType, QStringList({QLatin1String("all"), QLatin1String("domainOnly"), QLatin1String("none")}));
	registerOption(ContentBlocking_EnableContentBlockingOption, true, BooleanType);
	registerOption(ContentBlocking_EnableWildcardsOption, true, BooleanType);
	registerOption(ContentBlocking_ProfilesOption, QStringList(), ListType);
	registerOption(History_BrowsingLimitAmountGlobalOption, 1000, IntegerType);
	registerOption(History_BrowsingLimitAmountWindowOption, 50, IntegerType);
	registerOption(History_BrowsingLimitPeriodOption, 30, IntegerType);
	registerOption(History_ClearOnCloseOption, QStringList(), ListType);
	registerOption(History_DownloadsLimitPeriodOption, 7, IntegerType);
	registerOption(History_ExpandBranchesOption, QLatin1String("first"), EnumerationType, QStringList({QLatin1String("first"), QLatin1String("all"), QLatin1String("none")}));
	registerOption(History_ManualClearOptionsOption, QStringList({QLatin1String("browsing"), QLatin1String("cookies"), QLatin1String("forms"), QLatin1String("downloads"), QLatin1String("caches")}), ListType);
	registerOption(History_ManualClearPeriodOption, 1, IntegerType);
	registerOption(History_RememberBrowsingOption, true, BooleanType);
	registerOption(History_RememberDownloadsOption, true, BooleanType);
	registerOption(History_StoreFaviconsOption, true, BooleanType);
	registerOption(Interface_DateTimeFormatOption, QString(), StringType);
	registerOption(Interface_EnableSmoothScrollingOption, false, BooleanType);
	registerOption(Interface_IconThemePathOption, QString(), PathType);
	registerOption(Interface_LastTabClosingActionOption, QLatin1String("openTab"), EnumerationType, QStringList({QLatin1String("openTab"), QLatin1String("closeWindow"), QLatin1String("closeWindowIfNotLast"), QLatin1String("doNothing")}));
	registerOption(Interface_LockToolBarsOption, false, BooleanType);
	registerOption(Interface_MaximizeNewWindowsOption, true, BooleanType);
	registerOption(Interface_NewTabOpeningActionOption, QLatin1String("maximizeTab"), EnumerationType, QStringList({QLatin1String("doNothing"), QLatin1String("maximizeTab"), QLatin1String("cascadeAll"), QLatin1String("tileAll")}));
	registerOption(Interface_NotificationVisibilityDurationOption, 5, IntegerType);
	registerOption(Interface_ShowScrollBarsOption, true, BooleanType);
	registerOption(Interface_ShowSizeGripOption, true, BooleanType);
	registerOption(Interface_StyleSheetOption, QString(), PathType);
	registerOption(Interface_UseNativeNotificationsOption, true, BooleanType);
	registerOption(Interface_UseSystemIconThemeOption, false, BooleanType);
	registerOption(Interface_WidgetStyleOption, QString(), StringType);
	registerOption(Network_AcceptLanguageOption, QLatin1String("system,*;q=0.9"), StringType);
	registerOption(Network_CookiesKeepModeOption, QLatin1String("keepUntilExpires"), EnumerationType, QStringList({QLatin1String("keepUntilExpires"), QLatin1String("keepUntilExit"), QLatin1String("ask")}));
	registerOption(Network_CookiesPolicyOption, QLatin1String("acceptAll"), EnumerationType, QStringList({QLatin1String("acceptAll"), QLatin1String("acceptExisting"), QLatin1String("readOnly"), QLatin1String("ignore")}));
	registerOption(Network_DoNotTrackPolicyOption, QLatin1String("skip"), EnumerationType, QStringList({QLatin1String("skip"), QLatin1String("allow"), QLatin1String("doNotAllow")}));
	registerOption(Network_EnableReferrerOption, true, BooleanType);
	registerOption(Network_ProxyOption, QLatin1String("system"), StringType);
	registerOption(Network_ThirdPartyCookiesAcceptedHostsOption, QStringList(), ListType);
	registerOption(Network_ThirdPartyCookiesPolicyOption, QLatin1String("acceptAll"), EnumerationType, QStringList({QLatin1String("acceptAll"), QLatin1String("acceptExisting"), QLatin1String("ignore")}));
	registerOption(Network_ThirdPartyCookiesRejectedHostsOption, QStringList(), ListType);
	registerOption(Network_UserAgentOption, QLatin1String("default"), StringType);
	registerOption(Network_WorkOfflineOption, false, BooleanType);
	registerOption(Paths_DownloadsOption, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), PathType);
	registerOption(Paths_OpenFileOption, QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), PathType);
	registerOption(Paths_SaveFileOption, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), PathType);
	registerOption(Permissions_EnableFullScreenOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableGeolocationOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableImagesOption, QLatin1String("enabled"), EnumerationType, QStringList({QLatin1String("enabled"), QLatin1String("onlyCached"), QLatin1String("disabled")}));
	registerOption(Permissions_EnableJavaScriptOption, true, BooleanType);
	registerOption(Permissions_EnableLocalStorageOption, true, BooleanType);
	registerOption(Permissions_EnableMediaCaptureAudioOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableMediaCaptureVideoOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableMediaPlaybackAudioOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableNotificationsOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableOfflineStorageDatabaseOption, false, BooleanType);
	registerOption(Permissions_EnableOfflineWebApplicationCacheOption, false, BooleanType);
	registerOption(Permissions_EnablePluginsOption, QLatin1String("onDemand"), EnumerationType, QStringList({QLatin1String("enabled"), QLatin1String("onDemand"), QLatin1String("disabled")}));
	registerOption(Permissions_EnablePointerLockOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_EnableWebglOption, true, BooleanType);
	registerOption(Permissions_ScriptsCanAccessClipboardOption, false, BooleanType);
	registerOption(Permissions_ScriptsCanChangeWindowGeometryOption, true, BooleanType);
	registerOption(Permissions_ScriptsCanCloseSelfOpenedWindowsOption, true, BooleanType);
	registerOption(Permissions_ScriptsCanCloseWindowsOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("allow"), QLatin1String("disallow")}));
	registerOption(Permissions_ScriptsCanOpenWindowsOption, QLatin1String("ask"), EnumerationType, QStringList({QLatin1String("ask"), QLatin1String("blockAll"), QLatin1String("openAll"), QLatin1String("openAllInBackground")}));
	registerOption(Permissions_ScriptsCanReceiveRightClicksOption, true, BooleanType);
	registerOption(Permissions_ScriptsCanShowStatusMessagesOption, false, BooleanType);
	registerOption(Search_DefaultQuickSearchEngineOption, QLatin1String("duckduckgo"), EnumerationType, QStringList(QLatin1String("duckduckgo")));
	registerOption(Search_DefaultSearchEngineOption, QLatin1String("duckduckgo"), EnumerationType, QStringList(QLatin1String("duckduckgo")));
	registerOption(Search_EnableFindInPageAsYouTypeOption, true, BooleanType);
	registerOption(Search_ReuseLastQuickFindQueryOption, false, BooleanType);
	registerOption(Search_SearchEnginesOrderOption, QStringList({QLatin1String("duckduckgo"), QLatin1String("wikipedia"), QLatin1String("startpage"), QLatin1String("google"), QLatin1String("yahoo"), QLatin1String("bing"), QLatin1String("youtube")}), ListType);
	registerOption(Search_SearchEnginesSuggestionsOption, false, BooleanType);
	registerOption(Security_AllowMixedContentOption, false, BooleanType);
	registerOption(Security_CiphersOption, QStringList(QLatin1String("default")), ListType);
	registerOption(Security_IgnoreSslErrorsOption, QStringList(), ListType);
	registerOption(Sessions_OpenInExistingWindowOption, false, BooleanType);
	registerOption(Sessions_OptionsExludedFromInheritingOption, QStringList(QLatin1String("Content/PageReloadTime")), ListType);
	registerOption(Sessions_OptionsExludedFromSavingOption, QStringList(), ListType);
	registerOption(SourceViewer_ShowLineNumbersOption, true, BooleanType);
	registerOption(SourceViewer_WrapLinesOption, false, BooleanType);
	registerOption(StartPage_BackgroundColorOption, QString(), ColorType);
	registerOption(StartPage_BackgroundModeOption, QLatin1String("standard"), EnumerationType, QStringList({QLatin1String("standard"), QLatin1String("bestFit"), QLatin1String("center"), QLatin1String("stretch"), QLatin1String("tile")}));
	registerOption(StartPage_BackgroundPathOption, QString(), StringType);
	registerOption(StartPage_BookmarksFolderOption, QLatin1String("/Start Page/"), StringType);
	registerOption(StartPage_EnableStartPageOption, true, BooleanType);
	registerOption(StartPage_ShowAddTileOption, true, BooleanType);
	registerOption(StartPage_ShowSearchFieldOption, true, BooleanType);
	registerOption(StartPage_TileBackgroundModeOption, QLatin1String("thumbnail"), EnumerationType, QStringList({QLatin1String("none"), QLatin1String("thumbnail"), QLatin1String("favicon")}));
	registerOption(StartPage_TileHeightOption, 190, IntegerType);
	registerOption(StartPage_TileWidthOption, 270, IntegerType);
	registerOption(StartPage_TilesPerRowOption, 0, IntegerType);
	registerOption(StartPage_ZoomLevelOption, 100, IntegerType);
	registerOption(TabBar_EnablePreviewsOption, true, BooleanType);
	registerOption(TabBar_EnableThumbnailsOption, false, BooleanType);
	registerOption(TabBar_MaximumTabHeightOption, -1, IntegerType);
	registerOption(TabBar_MinimumTabHeightOption, -1, IntegerType);
	registerOption(TabBar_MaximumTabWidthOption, 250, IntegerType);
	registerOption(TabBar_MinimumTabWidthOption, -1, IntegerType);
	registerOption(TabBar_OpenNextToActiveOption, true, BooleanType);
	registerOption(TabBar_PreviewsAnimationDurationOption, 250, IntegerType);
	registerOption(TabBar_RequireModifierToSwitchTabOnScrollOption, true, BooleanType);
	registerOption(TabBar_ShowCloseButtonOption, true, BooleanType);
	registerOption(TabBar_ShowUrlIconOption, true, BooleanType);
	registerOption(Updates_ActiveChannelsOption, QStringList(), ListType);
	registerOption(Updates_AutomaticInstallOption, false, BooleanType);
	registerOption(Updates_CheckIntervalOption, 7, IntegerType);
	registerOption(Updates_LastCheckOption, QString(), StringType);
	registerOption(Updates_ServerUrlOption, QLatin1String("https://www.otter-browser.org/updates/update.json"), StringType);

	const QStringList hosts(QSettings(m_overridePath, QSettings::IniFormat).childGroups());

	for (int i = 0; i < hosts.count(); ++i)
	{
		if (hosts.at(i).startsWith(QLatin1Char('*')))
		{
			m_hasWildcardedOverrides = true;

			break;
		}
	}
}

void SettingsManager::removeOverride(const QUrl &url, const QString &key)
{
	if (key.isEmpty())
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove(getHost(url));
	}
	else
	{
		QSettings(m_overridePath, QSettings::IniFormat).remove(getHost(url) + QLatin1Char('/') + key);
	}
}

void SettingsManager::registerOption(int identifier, const QVariant &defaultValue, SettingsManager::OptionType type, const QStringList &choices)
{
	OptionDefinition definition;
	definition.defaultValue = defaultValue;
	definition.choices = choices;
	definition.type = type;
	definition.flags = (IsEnabledFlag | IsVisibleFlag | IsBuiltInFlag);
	definition.identifier = identifier;

	m_definitions.append(definition);
}

void SettingsManager::updateOptionDefinition(int identifier, const SettingsManager::OptionDefinition &definition)
{
	if (identifier >= 0 && identifier < m_definitions.count())
	{
		m_definitions[identifier].defaultValue = definition.defaultValue;
		m_definitions[identifier].choices = definition.choices;
	}
}

void SettingsManager::setValue(int identifier, const QVariant &value, const QUrl &url)
{
	const QString name(getOptionName(identifier));

	if (!url.isEmpty())
	{
		const QString overrideName(getHost(url) + QLatin1Char('/') + name);

		if (value.isNull())
		{
			QSettings(m_overridePath, QSettings::IniFormat).remove(overrideName);
		}
		else
		{
			QSettings(m_overridePath, QSettings::IniFormat).setValue(overrideName, value);
		}

		if (!m_hasWildcardedOverrides && overrideName.startsWith(QLatin1Char('*')))
		{
			m_hasWildcardedOverrides = true;
		}

		emit m_instance->optionChanged(identifier, value, url);

		return;
	}

	if (getOption(identifier) != value)
	{
		QSettings(m_globalPath, QSettings::IniFormat).setValue(name, value);

		emit m_instance->optionChanged(identifier, value);
	}
}

SettingsManager* SettingsManager::getInstance()
{
	return m_instance;
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
	QString name(m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(6);

		return name.replace(QLatin1Char('_'), QLatin1Char('/'));
	}

	return m_customOptions.key(identifier);
}

QString SettingsManager::getHost(const QUrl &url)
{
	return (url.isLocalFile() ? QLatin1String("localhost") : url.host());
}

QString SettingsManager::getReport()
{
	QString report;
	QTextStream stream(&report);
	stream.setFieldAlignment(QTextStream::AlignLeft);
	stream << QLatin1String("Settings:\n");

	QHash<QString, int> overridenValues;
	QSettings overrides(m_overridePath, QSettings::IniFormat);
	const QStringList overridesGroups(overrides.childGroups());

	for (int i = 0; i < overridesGroups.count(); ++i)
	{
		overrides.beginGroup(overridesGroups.at(i));

		const QStringList keys(overrides.allKeys());

		for (int j = 0; j < keys.count(); ++j)
		{
			if (overridenValues.contains(keys.at(j)))
			{
				++overridenValues[keys.at(j)];
			}
			else
			{
				overridenValues[keys.at(j)] = 1;
			}
		}

		overrides.endGroup();
	}

	QStringList options;

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		options.append(getOptionName(i));
	}

	options.sort();

	for (int i = 0; i < options.count(); ++i)
	{
		const OptionDefinition definition(getOptionDefinition(getOptionIdentifier(options.at(i))));

		stream << QLatin1Char('\t');
		stream.setFieldWidth(50);
		stream << options.at(i);
		stream.setFieldWidth(20);

		if (definition.type == StringType || definition.type == PathType)
		{
			stream << QLatin1Char('-');
		}
		else
		{
			stream << definition.defaultValue.toString();
		}

		stream << ((definition.defaultValue == getOption(definition.identifier)) ? QLatin1String("default") : QLatin1String("non default"));
		stream << (overridenValues.contains(options.at(i)) ? QStringLiteral("%1 override(s)").arg(overridenValues[options.at(i)]) : QLatin1String("no overrides"));
		stream.setFieldWidth(0);
		stream << QLatin1Char('\n');
	}

	stream << QLatin1Char('\n');

	return report;
}

QVariant SettingsManager::getOption(int identifier, const QUrl &url)
{
	if (identifier < 0 || identifier >= m_definitions.count())
	{
		return QVariant();
	}

	const QString name(getOptionName(identifier));

	if (url.isEmpty())
	{
		return QSettings(m_globalPath, QSettings::IniFormat).value(name, m_definitions.at(identifier).defaultValue);
	}

	const QString host(getHost(url));
	const QString overrideName(host + QLatin1Char('/') + name);
	const QSettings overrides(m_overridePath, QSettings::IniFormat);

	if (m_hasWildcardedOverrides && !overrides.contains(overrideName))
	{
		const QStringList hostParts(host.split(QLatin1Char('.')));

		for (int i = 1; i < hostParts.count(); ++i)
		{
			const QString wildcardedName(QLatin1String("*.") + QStringList(hostParts.mid(i)).join(QLatin1Char('.')) + QLatin1Char('/') + name);

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

	for (int i = 0; i < m_definitions.count(); ++i)
	{
		options.append(getOptionName(i));
	}

	options.sort();

	return options;
}

SettingsManager::OptionDefinition SettingsManager::getOptionDefinition(int identifier)
{
	if (identifier >= 0 && identifier < m_definitions.count())
	{
		return m_definitions.at(identifier);
	}

	return OptionDefinition();
}

int SettingsManager::registerOption(const QString &name, const QVariant &defaultValue, OptionType type, const QStringList &choices)
{
	if (name.isEmpty() || getOptionIdentifier(name) >= 0)
	{
		return -1;
	}

	const int identifier(m_identifierCounter);

	++m_identifierCounter;

	OptionDefinition definition;
	definition.defaultValue = defaultValue;
	definition.choices = choices;
	definition.type = type;
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

	return m_instance->metaObject()->enumerator(m_optionIdentifierEnumerator).keyToValue(mutableName.toLatin1());
}

bool SettingsManager::hasOverride(const QUrl &url, int identifier)
{
	if (identifier < 0)
	{
		return QSettings(m_overridePath, QSettings::IniFormat).childGroups().contains(getHost(url));
	}

	return QSettings(m_overridePath, QSettings::IniFormat).contains(getHost(url) + QLatin1Char('/') + getOptionName(identifier));
}

}
