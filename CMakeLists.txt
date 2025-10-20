cmake_minimum_required(VERSION 3.7.0)

project(otter-browser)

set(MAJOR_VERSION "1")
set(MINOR_VERSION "0")
set(PATCH_VERSION "81")
set(WEEKLY_VERSION "" CACHE STRING "")

add_definitions(-DOTTER_VERSION_MAIN="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")

if ("${WEEKLY_VERSION}" STREQUAL "")
	add_definitions(-DOTTER_VERSION_CONTEXT="-dev" -DOTTER_VERSION_WEEKLY=" ")
else ()
	add_definitions(-DOTTER_VERSION_CONTEXT=" weekly ${WEEKLY_VERSION}" -DOTTER_VERSION_WEEKLY="${WEEKLY_VERSION}")
endif ()

if (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
	include(InstallRequiredSystemLibraries)

	set(CPACK_SET_DESTDIR "on")
	set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
	set(CPACK_GENERATOR "DEB;RPM")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Web browser controlled by the user, not vice-versa")
	set(CPACK_PACKAGE_VENDOR "Vendor")
	set(CPACK_PACKAGE_CONTACT "Michal Dutkiewicz <michal@emdek.pl>")
	set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
	set(CPACK_PACKAGE_NAME "otter-browser")
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
	set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libhunspell-1.5-0 (>= 1.5.1), libqt5multimedia5 (>=5.15.0), libqt5qml5 (>=5.15.0), libqt5svg5 (>=5.15.0), libqt5webkit5 (>=5.212.0~alpha2)")
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gstreamer1.0-plugins-base, gstreamer1.0-plugins-good")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "web")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
	set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
		${CMAKE_INSTALL_MANDIR}/man1
		${CMAKE_INSTALL_PREFIX}/share/applications
		${CMAKE_INSTALL_PREFIX}/share/icons
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps
		${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps
	)

	include(CPack)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)
include(GNUInstallDirs)

option(ALLOW_WITHOUT_WEB_BACKENDS "Allow to build without any web backends (build testing only)" OFF)
option(ENABLE_QTWEBENGINE "Enable QtWebEngine backend (requires QtWebEngine 5.15)" ON)
option(ENABLE_QTWEBKIT "Enable QtWebKit backend (requires QtWebKit 5.212)" ON)
option(ENABLE_CRASH_REPORTS "Enable built-in crash reporting (official builds only)" OFF)
option(ENABLE_DBUS "Enable D-Bus based integration for notifications (only freedesktop.org compatible platforms)" ON)
option(ENABLE_SPELLCHECK "Enable Hunspell based spell checking" ON)

find_package(Qt5 5.15.0 REQUIRED COMPONENTS Core Gui Multimedia Network PrintSupport Qml Svg Widgets)
find_package(Hunspell 1.5.0 QUIET)

set_package_properties(Hunspell PROPERTIES URL "https://hunspell.github.io/" DESCRIPTION "Generic spell checking support" TYPE OPTIONAL)

set(OTTER_SOURCES
	src/main.cpp
	src/core/ActionExecutor.cpp
	src/core/ActionsManager.cpp
	src/core/AdblockContentFiltersProfile.cpp
	src/core/AddonsManager.cpp
	src/core/Application.cpp
	src/core/BookmarksManager.cpp
	src/core/BookmarksModel.cpp
	src/core/ContentFiltersManager.cpp
	src/core/Console.cpp
	src/core/CookieJar.cpp
	src/core/DataExchanger.cpp
	src/core/FeedParser.cpp
	src/core/FeedsManager.cpp
	src/core/FeedsModel.cpp
	src/core/GesturesController.cpp
	src/core/GesturesManager.cpp
	src/core/HandlersManager.cpp
	src/core/HistoryManager.cpp
	src/core/HistoryModel.cpp
	src/core/IniSettings.cpp
	src/core/InputInterpreter.cpp
	src/core/ItemModel.cpp
	src/core/Job.cpp
	src/core/JsonSettings.cpp
	src/core/ListingNetworkReply.cpp
	src/core/LocalListingNetworkReply.cpp
	src/core/LongTermTimer.cpp
	src/core/Migrator.cpp
	src/core/NetworkAutomaticProxy.cpp
	src/core/NetworkCache.cpp
	src/core/NetworkManager.cpp
	src/core/NetworkManagerFactory.cpp
	src/core/NetworkProxyFactory.cpp
	src/core/NotesManager.cpp
	src/core/NotificationsManager.cpp
	src/core/PasswordsManager.cpp
	src/core/PasswordsStorageBackend.cpp
	src/core/PlatformIntegration.cpp
	src/core/SearchEnginesManager.cpp
	src/core/SearchSuggester.cpp
	src/core/SessionModel.cpp
	src/core/SessionsManager.cpp
	src/core/SettingsManager.cpp
	src/core/SpellCheckManager.cpp
	src/core/TasksManager.cpp
	src/core/ThemesManager.cpp
	src/core/ToolBarsManager.cpp
	src/core/TransfersManager.cpp
	src/core/UpdateChecker.cpp
	src/core/Updater.cpp
	src/core/UserScript.cpp
	src/core/Utils.cpp
	src/core/WebBackend.cpp
	src/ui/AcceptCookieDialog.cpp
	src/ui/Action.cpp
	src/ui/ActionComboBoxWidget.cpp
	src/ui/ActionParametersDialog.cpp
	src/ui/Animation.cpp
	src/ui/ApplicationComboBoxWidget.cpp
	src/ui/AuthenticationDialog.cpp
	src/ui/BookmarkPropertiesDialog.cpp
	src/ui/BookmarksComboBoxWidget.cpp
	src/ui/BookmarksImportOptionsWidget.cpp
	src/ui/CategoriesTabWidget.cpp
	src/ui/CertificateDialog.cpp
	src/ui/ClearHistoryDialog.cpp
	src/ui/ColorWidget.cpp
	src/ui/ComboBoxWidget.cpp
	src/ui/ContentBlockingProfileDialog.cpp
	src/ui/ContentFiltersViewWidget.cpp
	src/ui/ContentsDialog.cpp
	src/ui/ContentsWidget.cpp
	src/ui/CookiePropertiesDialog.cpp
	src/ui/DataExchangerDialog.cpp
	src/ui/DateTimeTextLabelWidget.cpp
	src/ui/DiagnosticReportDialog.cpp
	src/ui/Dialog.cpp
	src/ui/FeedPropertiesDialog.cpp
	src/ui/FeedsComboBoxWidget.cpp
	src/ui/FilePathWidget.cpp
	src/ui/HeaderWidget.cpp
	src/ui/IconWidget.cpp
	src/ui/ImagePropertiesDialog.cpp
	src/ui/InlineListWidget.cpp
	src/ui/ItemDelegate.cpp
	src/ui/ItemViewWidget.cpp
	src/ui/LineEditWidget.cpp
	src/ui/LocaleDialog.cpp
	src/ui/MainWindow.cpp
	src/ui/MasterPasswordDialog.cpp
	src/ui/Menu.cpp
	src/ui/MenuBarWidget.cpp
	src/ui/MetaDataDialog.cpp
	src/ui/NotificationDialog.cpp
	src/ui/OpenAddressDialog.cpp
	src/ui/OpenBookmarkDialog.cpp
	src/ui/OptionWidget.cpp
	src/ui/PassiveNotificationWidget.cpp
	src/ui/PreferencesDialog.cpp
	src/ui/PreviewWidget.cpp
	src/ui/ProgressBarWidget.cpp
	src/ui/ProxyModel.cpp
	src/ui/QuickResponseCodeDialog.cpp
	src/ui/QuickResponseCodeWidget.cpp
	src/ui/ReloadTimeDialog.cpp
	src/ui/ResizerWidget.cpp
	src/ui/SaveSessionDialog.cpp
	src/ui/SearchEnginePropertiesDialog.cpp
	src/ui/SessionsManagerDialog.cpp
	src/ui/SidebarWidget.cpp
	src/ui/SourceEditWidget.cpp
	src/ui/SourceViewerWebWidget.cpp
	src/ui/SplitterWidget.cpp
	src/ui/StartupDialog.cpp
	src/ui/StatusBarWidget.cpp
	src/ui/Style.cpp
	src/ui/SyntaxHighlighter.cpp
	src/ui/TabBarWidget.cpp
	src/ui/TabSwitcherWidget.cpp
	src/ui/TextBrowserWidget.cpp
	src/ui/TextEditWidget.cpp
	src/ui/TextLabelWidget.cpp
	src/ui/ToolBarDialog.cpp
	src/ui/ToolBarDropZoneWidget.cpp
	src/ui/ToolBarWidget.cpp
	src/ui/ToolButtonWidget.cpp
	src/ui/TransferDialog.cpp
	src/ui/TrayIcon.cpp
	src/ui/UpdateCheckerDialog.cpp
	src/ui/WebsiteInformationDialog.cpp
	src/ui/WebsitePreferencesDialog.cpp
	src/ui/WebWidget.cpp
	src/ui/WidgetFactory.cpp
	src/ui/Window.cpp
	src/ui/WorkspaceWidget.cpp
	src/ui/preferences/AcceptLanguageDialog.cpp
	src/ui/preferences/CookiesExceptionsDialog.cpp
	src/ui/preferences/MouseProfileDialog.cpp
	src/ui/preferences/ProxyPropertiesDialog.cpp
	src/ui/preferences/UserAgentPropertiesDialog.cpp
	src/modules/backends/passwords/file/FilePasswordsStorageBackend.cpp
	src/modules/exporters/html/HtmlBookmarksExportDataExchanger.cpp
	src/modules/exporters/xbel/XbelBookmarksExportDataExchanger.cpp
	src/modules/importers/html/HtmlBookmarksImportDataExchanger.cpp
	src/modules/importers/opera/OperaBookmarksImportDataExchanger.cpp
	src/modules/importers/opera/OperaNotesImportDataExchanger.cpp
	src/modules/importers/opera/OperaSearchEnginesImportDataExchanger.cpp
	src/modules/importers/opera/OperaSessionImportDataExchanger.cpp
	src/modules/importers/opml/OpmlImportDataExchanger.cpp
	src/modules/importers/opml/OpmlImportOptionsWidget.cpp
	src/modules/widgets/action/ActionWidget.cpp
	src/modules/widgets/address/AddressCompletionModel.cpp
	src/modules/widgets/address/AddressWidget.cpp
	src/modules/widgets/bookmark/BookmarkWidget.cpp
	src/modules/widgets/configurationOption/ConfigurationOptionWidget.cpp
	src/modules/widgets/contentBlockingInformation/ContentBlockingInformationWidget.cpp
	src/modules/widgets/errorConsole/ErrorConsoleWidget.cpp
	src/modules/widgets/menuButton/MenuButtonWidget.cpp
	src/modules/widgets/panelChooser/PanelChooserWidget.cpp
	src/modules/widgets/privateWindowIndicator/PrivateWindowIndicatorWidget.cpp
	src/modules/widgets/progressInformation/ProgressInformationWidget.cpp
	src/modules/widgets/search/SearchWidget.cpp
	src/modules/widgets/statusMessage/StatusMessageWidget.cpp
	src/modules/widgets/transfers/TransfersWidget.cpp
	src/modules/widgets/zoom/ZoomWidget.cpp
	src/modules/windows/actions/ActionsContentsWidget.cpp
	src/modules/windows/addons/AddonsContentsWidget.cpp
	src/modules/windows/addons/AddonsPage.cpp
	src/modules/windows/addons/DictionariesPage.cpp
	src/modules/windows/addons/UserScriptsPage.cpp
	src/modules/windows/bookmarks/BookmarksContentsWidget.cpp
	src/modules/windows/cache/CacheContentsWidget.cpp
	src/modules/windows/configuration/ConfigurationContentsWidget.cpp
	src/modules/windows/configuration/OverridesDialog.cpp
	src/modules/windows/contentFilters/ContentFiltersContentsWidget.cpp
	src/modules/windows/cookies/CookiesContentsWidget.cpp
	src/modules/windows/history/HistoryContentsWidget.cpp
	src/modules/windows/feeds/FeedsContentsWidget.cpp
	src/modules/windows/links/LinksContentsWidget.cpp
	src/modules/windows/notes/NotesContentsWidget.cpp
	src/modules/windows/pageInformation/PageInformationContentsWidget.cpp
	src/modules/windows/passwords/PasswordsContentsWidget.cpp
	src/modules/windows/preferences/AdvancedPreferencesPage.cpp
	src/modules/windows/preferences/ContentPreferencesPage.cpp
	src/modules/windows/preferences/GeneralPreferencesPage.cpp
	src/modules/windows/preferences/InputPreferencesPage.cpp
	src/modules/windows/preferences/PreferencesContentsWidget.cpp
	src/modules/windows/preferences/PrivacyPreferencesPage.cpp
	src/modules/windows/preferences/SearchPreferencesPage.cpp
	src/modules/windows/preferences/WebsitesPreferencesPage.cpp
	src/modules/windows/tabHistory/TabHistoryContentsWidget.cpp
	src/modules/windows/transfers/TransfersContentsWidget.cpp
	src/modules/windows/web/PasswordBarWidget.cpp
	src/modules/windows/web/PermissionBarWidget.cpp
	src/modules/windows/web/PopupsBarWidget.cpp
	src/modules/windows/web/ProgressToolBarWidget.cpp
	src/modules/windows/web/SearchBarWidget.cpp
	src/modules/windows/web/SelectPasswordDialog.cpp
	src/modules/windows/web/StartPageModel.cpp
	src/modules/windows/web/StartPagePreferencesDialog.cpp
	src/modules/windows/web/StartPageWidget.cpp
	src/modules/windows/web/WebContentsWidget.cpp
	src/modules/windows/windows/WindowsContentsWidget.cpp
	3rdparty/columnresizer/ColumnResizer.cpp
	3rdparty/mousegestures/MouseGestures.cpp
	3rdparty/qrcodegen/qrcodegen.cpp
)

qt5_add_resources(OTTER_RESOURCES
	resources/resources.qrc
)

qt5_wrap_ui(OTTER_UI
	src/ui/AcceptCookieDialog.ui
	src/ui/ActionParametersDialog.ui
	src/ui/AuthenticationDialog.ui
	src/ui/BookmarkPropertiesDialog.ui
	src/ui/BookmarksImportOptionsWidget.ui
	src/ui/CertificateDialog.ui
	src/ui/ClearHistoryDialog.ui
	src/ui/ContentBlockingProfileDialog.ui
	src/ui/CookiePropertiesDialog.ui
	src/ui/DataExchangerDialog.ui
	src/ui/DiagnosticReportDialog.ui
	src/ui/FeedPropertiesDialog.ui
	src/ui/ImagePropertiesDialog.ui
	src/ui/LocaleDialog.ui
	src/ui/MainWindow.ui
	src/ui/MasterPasswordDialog.ui
	src/ui/MetaDataDialog.ui
	src/ui/OpenAddressDialog.ui
	src/ui/OpenBookmarkDialog.ui
	src/ui/PreferencesDialog.ui
	src/ui/QuickResponseCodeDialog.ui
	src/ui/ReloadTimeDialog.ui
	src/ui/SaveSessionDialog.ui
	src/ui/SearchEnginePropertiesDialog.ui
	src/ui/SessionsManagerDialog.ui
	src/ui/SidebarWidget.ui
	src/ui/StartupDialog.ui
	src/ui/ToolBarDialog.ui
	src/ui/TransferDialog.ui
	src/ui/UpdateCheckerDialog.ui
	src/ui/WebsiteInformationDialog.ui
	src/ui/WebsitePreferencesDialog.ui
	src/ui/preferences/AcceptLanguageDialog.ui
	src/ui/preferences/CookiesExceptionsDialog.ui
	src/ui/preferences/MouseProfileDialog.ui
	src/ui/preferences/ProxyPropertiesDialog.ui
	src/ui/preferences/UserAgentPropertiesDialog.ui
	src/modules/importers/opml/OpmlImportOptionsWidget.ui
	src/modules/widgets/errorConsole/ErrorConsoleWidget.ui
	src/modules/windows/actions/ActionsContentsWidget.ui
	src/modules/windows/addons/AddonsContentsWidget.ui
	src/modules/windows/addons/AddonsPage.ui
	src/modules/windows/bookmarks/BookmarksContentsWidget.ui
	src/modules/windows/cache/CacheContentsWidget.ui
	src/modules/windows/configuration/ConfigurationContentsWidget.ui
	src/modules/windows/configuration/OverridesDialog.ui
	src/modules/windows/contentFilters/ContentFiltersContentsWidget.ui
	src/modules/windows/cookies/CookiesContentsWidget.ui
	src/modules/windows/feeds/FeedsContentsWidget.ui
	src/modules/windows/history/HistoryContentsWidget.ui
	src/modules/windows/links/LinksContentsWidget.ui
	src/modules/windows/notes/NotesContentsWidget.ui
	src/modules/windows/pageInformation/PageInformationContentsWidget.ui
	src/modules/windows/passwords/PasswordsContentsWidget.ui
	src/modules/windows/preferences/AdvancedPreferencesPage.ui
	src/modules/windows/preferences/ContentPreferencesPage.ui
	src/modules/windows/preferences/GeneralPreferencesPage.ui
	src/modules/windows/preferences/InputPreferencesPage.ui
	src/modules/windows/preferences/PreferencesContentsWidget.ui
	src/modules/windows/preferences/PrivacyPreferencesPage.ui
	src/modules/windows/preferences/SearchPreferencesPage.ui
	src/modules/windows/preferences/WebsitesPreferencesPage.ui
	src/modules/windows/tabHistory/TabHistoryContentsWidget.ui
	src/modules/windows/transfers/TransfersContentsWidget.ui
	src/modules/windows/web/PasswordBarWidget.ui
	src/modules/windows/web/PermissionBarWidget.ui
	src/modules/windows/web/PopupsBarWidget.ui
	src/modules/windows/web/SearchBarWidget.ui
	src/modules/windows/web/SelectPasswordDialog.ui
	src/modules/windows/web/StartPagePreferencesDialog.ui
	src/modules/windows/windows/WindowsContentsWidget.ui
)

foreach (_dir ${Qt5Widgets_PRIVATE_INCLUDE_DIRS})
	if (EXISTS "${_dir}/private/qpixmapfilter_p.h")
		add_definitions(-DOTTER_ENABLE_STARTPAGEBLUR)
		include_directories(${Qt5Widgets_PRIVATE_INCLUDE_DIRS})

		break ()
	endif ()
endforeach ()

if (ENABLE_QTWEBENGINE)
	include(src/modules/backends/web/qtwebengine/CMakeLists.txt)
endif ()

if (ENABLE_QTWEBKIT)
	include(src/modules/backends/web/qtwebkit/CMakeLists.txt)
endif ()

if (ENABLE_CRASH_REPORTS)
	add_definitions(-DOTTER_ENABLE_CRASH_REPORTS)
	include_directories(${CMAKE_SOURCE_DIR}/3rdparty/breakpad/src/)

	if (WIN32)
		set(OTTER_SOURCES
			${OTTER_SOURCES}
			3rdparty/breakpad/src/common/windows/guid_string.cc
			3rdparty/breakpad/src/client/windows/crash_generation/crash_generation_client.cc
			3rdparty/breakpad/src/client/windows/handler/exception_handler.cc
		)
	elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(OTTER_SOURCES
			${OTTER_SOURCES}
			3rdparty/breakpad/src/client/minidump_file_writer.cc
			3rdparty/breakpad/src/client/linux/crash_generation/crash_generation_client.cc
			3rdparty/breakpad/src/client/linux/dump_writer_common/thread_info.cc
			3rdparty/breakpad/src/client/linux/dump_writer_common/ucontext_reader.cc
			3rdparty/breakpad/src/client/linux/handler/exception_handler.cc
			3rdparty/breakpad/src/client/linux/handler/minidump_descriptor.cc
			3rdparty/breakpad/src/client/linux/log/log.cc
			3rdparty/breakpad/src/client/linux/microdump_writer/microdump_writer.cc
			3rdparty/breakpad/src/client/linux/minidump_writer/linux_core_dumper.cc
			3rdparty/breakpad/src/client/linux/minidump_writer/linux_dumper.cc
			3rdparty/breakpad/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
			3rdparty/breakpad/src/client/linux/minidump_writer/minidump_writer.cc
			3rdparty/breakpad/src/common/convert_UTF.c
			3rdparty/breakpad/src/common/md5.cc
			3rdparty/breakpad/src/common/string_conversion.cc
			3rdparty/breakpad/src/common/linux/elf_core_dump.cc
			3rdparty/breakpad/src/common/linux/elfutils.cc
			3rdparty/breakpad/src/common/linux/file_id.cc
			3rdparty/breakpad/src/common/linux/guid_creator.cc
			3rdparty/breakpad/src/common/linux/linux_libc_support.cc
			3rdparty/breakpad/src/common/linux/memory_mapped_file.cc
			3rdparty/breakpad/src/common/linux/safe_readlink.cc
		)
	endif ()
endif ()

if (TARGET Hunspell::Hunspell AND ENABLE_SPELLCHECK)
	add_definitions(-DOTTER_ENABLE_SPELLCHECK)
	include_directories(Hunspell::Hunspell)

	set(OTTER_SOURCES
		${OTTER_SOURCES}
		3rdparty/sonnet/src/core/client.cpp
		3rdparty/sonnet/src/core/loader.cpp
		3rdparty/sonnet/src/core/speller.cpp
		3rdparty/sonnet/src/core/spellerplugin.cpp
		3rdparty/sonnet/src/core/settings.cpp
		3rdparty/sonnet/src/core/textbreaks.cpp
		3rdparty/sonnet/src/core/tokenizer.cpp
		3rdparty/sonnet/src/ui/highlighter.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspellclient.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspelldebug.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspelldict.cpp
	)
endif ()

if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)

	find_package(Qt5 5.15.0 QUIET COMPONENTS WinExtras)

	if (MSVC)
		set(CMAKE_PREFIX_PATH $ENV{QTDIR})
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /GL /Zi")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /DEBUG /OPT:REF")

		if (CMAKE_CL_64)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
		else ()
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS,5.01 /ENTRY:\"mainCRTStartup\"")
		endif ()
	endif ()

	set(OTTER_SOURCES
		${OTTER_SOURCES}
		otter-browser.rc
		src/modules/platforms/windows/WindowsPlatformIntegration.cpp
		src/modules/platforms/windows/WindowsPlatformStyle.cpp
	)
elseif (APPLE)
	find_package(Qt5 5.15.0 QUIET COMPONENTS MacExtras)

	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
	set(MACOSX_BUNDLE_BUNDLE_NAME "Otter Browser")
	set(MACOSX_BUNDLE_BUNDLE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
	set(MACOSX_BUNDLE_LONG_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
	set(MACOSX_BUNDLE_ICON_FILE otter-browser.icns)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.otter-browser.otter-browser")
	set(MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2013-2025 Otter Browser Team. All rights reserved.")
	set(OTTER_SOURCES
		${OTTER_SOURCES}
		src/modules/platforms/mac/MacPlatformIntegration.mm
		src/modules/platforms/mac/MacPlatformStyle.cpp
		resources/icons/otter-browser.icns
	)

	set_source_files_properties(resources/icons/otter-browser.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif (UNIX)
	find_package(Qt5 5.15.0 QUIET COMPONENTS DBus)

	if (TARGET Qt5::DBus AND ENABLE_DBUS)
		add_definitions(-DOTTER_ENABLE_DBUS)
	endif ()

	set(OTTER_SOURCES
		${OTTER_SOURCES}
		src/modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.cpp
		src/modules/platforms/freedesktoporg/FreeDesktopOrgPlatformStyle.cpp
		3rdparty/libmimeapps/ConfigReader.cpp
		3rdparty/libmimeapps/DesktopEntry.cpp
		3rdparty/libmimeapps/Index.cpp
		3rdparty/libmimeapps/Tools.cpp
	)
endif ()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

list(LENGTH OTTER_BACKENDS_WEB OTTER_BACKENDS_WEB_LENGTH)

if (${OTTER_BACKENDS_WEB_LENGTH} EQUAL 0)
	if (ALLOW_WITHOUT_WEB_BACKENDS)
		add_definitions(-DOTTER_NO_WEB_BACKENDS)

		message(WARNING "No web backends available!")
	else ()
		message(SEND_ERROR "No web backends available!")
	endif ()
else ()
	set(OTTER_BACKENDS_WEB_SUMMARY "The following web backends will be built:\n\n")

	foreach (_backend ${OTTER_BACKENDS_WEB})
		set(OTTER_BACKENDS_WEB_SUMMARY "${OTTER_BACKENDS_WEB_SUMMARY} * ${_backend}\n")
	endforeach ()

	message(STATUS ${OTTER_BACKENDS_WEB_SUMMARY})
endif ()

string(TIMESTAMP BUILD_DATETIME UTC)

find_package(Git QUIET)

set(GIT_BRANCH "unknown")
set(GIT_DATETIME "unknown")
set(GIT_REVISION "unknown")

if (GIT_FOUND)
	macro(execute_git OUTPUT COMMAND)
		separate_arguments(COMMAND)

		execute_process(COMMAND ${GIT_EXECUTABLE} ${COMMAND}
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			OUTPUT_VARIABLE ${OUTPUT}
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endmacro()

	execute_git("GIT_BRANCH" "rev-parse;--abbrev-ref;HEAD")
	execute_git("GIT_DATETIME" "show;-s;--format=%ci")
	execute_git("GIT_REVISION" "rev-parse;--short;HEAD")
endif ()

add_definitions(-DOTTER_BUILD_DATETIME="${BUILD_DATETIME}" -DOTTER_GIT_BRANCH="${GIT_BRANCH}" -DOTTER_GIT_DATETIME="${GIT_DATETIME}" -DOTTER_GIT_REVISION="${GIT_REVISION}")

add_executable(otter-browser WIN32 MACOSX_BUNDLE
	${OTTER_UI}
	${OTTER_RESOURCES}
	${OTTER_SOURCES}
)

foreach (_library ${OTTER_LINK_LIBRARIES})
	target_link_libraries(otter-browser ${_library})
endforeach ()

if (TARGET Hunspell::Hunspell AND ENABLE_SPELLCHECK)
	target_link_libraries(otter-browser Hunspell::Hunspell)
endif ()

if (WIN32)
	target_link_libraries(otter-browser Qt5::WinExtras ole32 shell32 advapi32 user32)
elseif (APPLE)
	find_library(FRAMEWORK_Cocoa Cocoa)
	find_library(FRAMEWORK_Foundation Foundation)

	set_target_properties(otter-browser PROPERTIES OUTPUT_NAME "Otter Browser")

	target_link_libraries(otter-browser Qt5::MacExtras ${FRAMEWORK_Cocoa} ${FRAMEWORK_Foundation})
elseif (UNIX)
	if (TARGET Qt5::DBus AND ENABLE_DBUS)
		target_link_libraries(otter-browser Qt5::DBus)
	endif ()

	if (ENABLE_CRASH_REPORTS)
		target_link_libraries(otter-browser -lpthread)
	endif ()
endif ()

target_link_libraries(otter-browser Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::Network Qt5::PrintSupport Qt5::Qml Qt5::Svg Qt5::Widgets)

set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications CACHE FILEPATH "Install path for .desktop files")

file(GLOB OTTER_TRANSLATIONS resources/translations/*.qm)

foreach (_translation ${OTTER_TRANSLATIONS})
	install(FILES ${_translation} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/otter-browser/locale/)

	if (APPLE)
		file(COPY ${_translation} DESTINATION "Otter Browser.app/Contents/Resources/locale/")
	endif ()
endforeach ()

install(FILES resources/icons/otter-browser-16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)
install(FILES otter-browser.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES packaging/otter-browser.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)
install(FILES man/otter-browser.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
install(TARGETS otter-browser DESTINATION bin/)
