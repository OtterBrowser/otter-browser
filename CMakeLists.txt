cmake_minimum_required(VERSION 3.0.0)

if (POLICY CMP0020)
	cmake_policy(SET CMP0020 NEW)
endif (POLICY CMP0020)

if (POLICY CMP0043)
	cmake_policy(SET CMP0043 NEW)
endif (POLICY CMP0043)

project(otter-browser)

set(MAJOR_VERSION "0")
set(MINOR_VERSION "9")
set(PATCH_VERSION "91")

add_definitions(-DOTTER_VERSION_MAIN="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
add_definitions(-DOTTER_VERSION_WEEKLY=" ")
add_definitions(-DOTTER_VERSION_CONTEXT="-dev")
add_definitions(-DOTTER_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

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
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libhunspell-1.3-0 (>= 1.3.3), libqt5multimedia5 (>=5.3.0), libqt5qml5 (>=5.3.0), libqt5webkit5 (>=5.3.0), libqt5xmlpatterns5 (>=5.3.0)")
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gstreamer0.10-plugins-base, gstreamer0.10-plugins-good")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "web")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
	set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
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
endif (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include(CheckCXXCompilerFlag)
include(FeatureSummary)
include(GNUInstallDirs)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

if (COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif (COMPILER_SUPPORTS_CXX11)

option(ENABLE_QTWEBENGINE "Enable QtWebEngine backend (requires Qt 5.6)" ON)
option(ENABLE_QTWEBKIT "Enable QtWebKit backend (requires Qt 5.3)" ON)
option(ENABLE_CRASHREPORTS "Enable built-in crash reporting (only for official builds)" OFF)

find_package(Qt5 5.3.0 REQUIRED COMPONENTS Core DBus Gui Multimedia Network PrintSupport Qml Widgets XmlPatterns)
find_package(Qt5WebEngineWidgets 5.6.0 QUIET)
find_package(Qt5WebKitWidgets 5.3.0 QUIET)
find_package(Hunspell 1.3.0 QUIET)

set_package_properties(Qt5WebEngineCore PROPERTIES URL "http://qt.io/" DESCRIPTION "QtWebEngine based backend (core)" TYPE OPTIONAL)
set_package_properties(Qt5WebEngineWidgets PROPERTIES URL "http://qt.io/" DESCRIPTION "QtWebEngine based backend (widgets)" TYPE OPTIONAL)
set_package_properties(Qt5WebKit PROPERTIES URL "http://qt.io/" DESCRIPTION "QtWebKit based backend (core)" TYPE OPTIONAL)
set_package_properties(Qt5WebKitWidgets PROPERTIES URL "http://qt.io/" DESCRIPTION "QtWebKit based backend (widgets)" TYPE OPTIONAL)
set_package_properties(Hunspell PROPERTIES URL "http://hunspell.github.io/" DESCRIPTION "Generic spell checking support" TYPE OPTIONAL)

set(otter_src
	src/main.cpp
	src/core/ActionsManager.cpp
	src/core/AddonsManager.cpp
	src/core/AddressCompletionModel.cpp
	src/core/Application.cpp
	src/core/BookmarksImporter.cpp
	src/core/BookmarksManager.cpp
	src/core/BookmarksModel.cpp
	src/core/ContentBlockingManager.cpp
	src/core/ContentBlockingProfile.cpp
	src/core/Console.cpp
	src/core/CookieJar.cpp
	src/core/FileSystemCompleterModel.cpp
	src/core/GesturesManager.cpp
	src/core/HandlersManager.cpp
	src/core/HistoryManager.cpp
	src/core/HistoryModel.cpp
	src/core/Importer.cpp
	src/core/InputInterpreter.cpp
	src/core/LocalListingNetworkReply.cpp
	src/core/LongTermTimer.cpp
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
	src/core/Settings.cpp
	src/core/SettingsManager.cpp
	src/core/SpellCheckManager.cpp
	src/core/ThemesManager.cpp
	src/core/ToolBarsManager.cpp
	src/core/TransfersManager.cpp
	src/core/UpdateChecker.cpp
	src/core/Updater.cpp
	src/core/UserScript.cpp
	src/core/Utils.cpp
	src/core/WebBackend.cpp
	src/core/WindowsManager.cpp
	src/ui/AcceptCookieDialog.cpp
	src/ui/ActionComboBoxWidget.cpp
	src/ui/ApplicationComboBoxWidget.cpp
	src/ui/AuthenticationDialog.cpp
	src/ui/BookmarkPropertiesDialog.cpp
	src/ui/BookmarksComboBoxWidget.cpp
	src/ui/BookmarksImporterWidget.cpp
	src/ui/CertificateDialog.cpp
	src/ui/ClearHistoryDialog.cpp
	src/ui/ColorWidget.cpp
	src/ui/ComboBoxWidget.cpp
	src/ui/ConsoleWidget.cpp
	src/ui/ContentsDialog.cpp
	src/ui/ContentsWidget.cpp
	src/ui/Dialog.cpp
	src/ui/FilePathWidget.cpp
	src/ui/IconWidget.cpp
	src/ui/ImagePropertiesDialog.cpp
	src/ui/ImportDialog.cpp
	src/ui/ItemDelegate.cpp
	src/ui/ItemViewWidget.cpp
	src/ui/LineEditWidget.cpp
	src/ui/LocaleDialog.cpp
	src/ui/MainWindow.cpp
	src/ui/MasterPasswordDialog.cpp
	src/ui/Menu.cpp
	src/ui/MenuBarWidget.cpp
	src/ui/NotificationDialog.cpp
	src/ui/OpenAddressDialog.cpp
	src/ui/OpenBookmarkDialog.cpp
	src/ui/OptionDelegate.cpp
	src/ui/OptionWidget.cpp
	src/ui/PreferencesDialog.cpp
	src/ui/PreviewWidget.cpp
	src/ui/ProxyModel.cpp
	src/ui/ReloadTimeDialog.cpp
	src/ui/ReportDialog.cpp
	src/ui/SaveSessionDialog.cpp
	src/ui/SearchEnginePropertiesDialog.cpp
	src/ui/SessionsManagerDialog.cpp
	src/ui/SidebarWidget.cpp
	src/ui/SourceViewerWebWidget.cpp
	src/ui/SourceViewerWidget.cpp
	src/ui/StartupDialog.cpp
	src/ui/StatusBarWidget.cpp
	src/ui/Style.cpp
	src/ui/TabBarWidget.cpp
	src/ui/TabSwitcherWidget.cpp
	src/ui/TextLabelWidget.cpp
	src/ui/ToolBarAreaWidget.cpp
	src/ui/ToolBarDialog.cpp
	src/ui/ToolBarWidget.cpp
	src/ui/ToolButtonWidget.cpp
	src/ui/TransferDialog.cpp
	src/ui/TrayIcon.cpp
	src/ui/UpdateCheckerDialog.cpp
	src/ui/UserAgentsManagerDialog.cpp
	src/ui/WebsiteInformationDialog.cpp
	src/ui/WebsitePreferencesDialog.cpp
	src/ui/WebWidget.cpp
	src/ui/WidgetFactory.cpp
	src/ui/Window.cpp
	src/ui/WorkspaceWidget.cpp
	src/ui/preferences/AcceptLanguageDialog.cpp
	src/ui/preferences/ContentBlockingDialog.cpp
	src/ui/preferences/ContentBlockingIntervalDelegate.cpp
	src/ui/preferences/ContentBlockingProfileDialog.cpp
	src/ui/preferences/CookiesExceptionsDialog.cpp
	src/ui/preferences/JavaScriptPreferencesDialog.cpp
	src/ui/preferences/KeyboardProfileDialog.cpp
	src/ui/preferences/MouseProfileDialog.cpp
	src/ui/preferences/PreferencesAdvancedPageWidget.cpp
	src/ui/preferences/PreferencesContentPageWidget.cpp
	src/ui/preferences/PreferencesGeneralPageWidget.cpp
	src/ui/preferences/PreferencesPrivacyPageWidget.cpp
	src/ui/preferences/PreferencesSearchPageWidget.cpp
	src/modules/backends/passwords/file/FilePasswordsStorageBackend.cpp
	src/modules/importers/html/HtmlBookmarksImporter.cpp
	src/modules/importers/opera/OperaBookmarksImporter.cpp
	src/modules/importers/opera/OperaNotesImporter.cpp
	src/modules/importers/opera/OperaSearchEnginesImporter.cpp
	src/modules/importers/opera/OperaSessionImporter.cpp
	src/modules/widgets/action/ActionWidget.cpp
	src/modules/widgets/action/NavigationActionWidget.cpp
	src/modules/widgets/address/AddressWidget.cpp
	src/modules/widgets/bookmark/BookmarkWidget.cpp
	src/modules/widgets/contentBlockingInformation/ContentBlockingInformationWidget.cpp
	src/modules/widgets/menuButton/MenuButtonWidget.cpp
	src/modules/widgets/panelChooser/PanelChooserWidget.cpp
	src/modules/widgets/progressInformation/ProgressInformationWidget.cpp
	src/modules/widgets/search/SearchWidget.cpp
	src/modules/widgets/statusMessage/StatusMessageWidget.cpp
	src/modules/widgets/zoom/ZoomWidget.cpp
	src/modules/windows/addons/AddonsContentsWidget.cpp
	src/modules/windows/bookmarks/BookmarksContentsWidget.cpp
	src/modules/windows/cache/CacheContentsWidget.cpp
	src/modules/windows/configuration/ConfigurationContentsWidget.cpp
	src/modules/windows/cookies/CookiesContentsWidget.cpp
	src/modules/windows/history/HistoryContentsWidget.cpp
	src/modules/windows/notes/NotesContentsWidget.cpp
	src/modules/windows/passwords/PasswordsContentsWidget.cpp
	src/modules/windows/transfers/TransfersContentsWidget.cpp
	src/modules/windows/web/PasswordBarWidget.cpp
	src/modules/windows/web/PermissionBarWidget.cpp
	src/modules/windows/web/PopupsBarWidget.cpp
	src/modules/windows/web/ProgressBarWidget.cpp
	src/modules/windows/web/SearchBarWidget.cpp
	src/modules/windows/web/SelectPasswordDialog.cpp
	src/modules/windows/web/StartPageModel.cpp
	src/modules/windows/web/StartPagePreferencesDialog.cpp
	src/modules/windows/web/StartPageWidget.cpp
	src/modules/windows/web/TileDelegate.cpp
	src/modules/windows/web/WebContentsWidget.cpp
	3rdparty/mousegestures/MouseGestures.cpp
)

qt5_add_resources(otter_res
	resources/resources.qrc
)

qt5_wrap_ui(otter_ui
	src/ui/AcceptCookieDialog.ui
	src/ui/AuthenticationDialog.ui
	src/ui/BookmarkPropertiesDialog.ui
	src/ui/BookmarksImporterWidget.ui
	src/ui/CertificateDialog.ui
	src/ui/ClearHistoryDialog.ui
	src/ui/ConsoleWidget.ui
	src/ui/ImagePropertiesDialog.ui
	src/ui/ImportDialog.ui
	src/ui/LocaleDialog.ui
	src/ui/MainWindow.ui
	src/ui/MasterPasswordDialog.ui
	src/ui/OpenAddressDialog.ui
	src/ui/OpenBookmarkDialog.ui
	src/ui/PreferencesDialog.ui
	src/ui/ReloadTimeDialog.ui
	src/ui/ReportDialog.ui
	src/ui/SaveSessionDialog.ui
	src/ui/SearchEnginePropertiesDialog.ui
	src/ui/SessionsManagerDialog.ui
	src/ui/SidebarWidget.ui
	src/ui/StartupDialog.ui
	src/ui/ToolBarDialog.ui
	src/ui/TransferDialog.ui
	src/ui/UpdateCheckerDialog.ui
	src/ui/UserAgentsManagerDialog.ui
	src/ui/WebsiteInformationDialog.ui
	src/ui/WebsitePreferencesDialog.ui
	src/ui/preferences/AcceptLanguageDialog.ui
	src/ui/preferences/ContentBlockingDialog.ui
	src/ui/preferences/ContentBlockingProfileDialog.ui
	src/ui/preferences/CookiesExceptionsDialog.ui
	src/ui/preferences/JavaScriptPreferencesDialog.ui
	src/ui/preferences/KeyboardProfileDialog.ui
	src/ui/preferences/MouseProfileDialog.ui
	src/ui/preferences/PreferencesAdvancedPageWidget.ui
	src/ui/preferences/PreferencesContentPageWidget.ui
	src/ui/preferences/PreferencesGeneralPageWidget.ui
	src/ui/preferences/PreferencesPrivacyPageWidget.ui
	src/ui/preferences/PreferencesSearchPageWidget.ui
	src/modules/windows/addons/AddonsContentsWidget.ui
	src/modules/windows/bookmarks/BookmarksContentsWidget.ui
	src/modules/windows/cache/CacheContentsWidget.ui
	src/modules/windows/configuration/ConfigurationContentsWidget.ui
	src/modules/windows/cookies/CookiesContentsWidget.ui
	src/modules/windows/history/HistoryContentsWidget.ui
	src/modules/windows/notes/NotesContentsWidget.ui
	src/modules/windows/passwords/PasswordsContentsWidget.ui
	src/modules/windows/transfers/TransfersContentsWidget.ui
	src/modules/windows/web/PasswordBarWidget.ui
	src/modules/windows/web/PermissionBarWidget.ui
	src/modules/windows/web/PopupsBarWidget.ui
	src/modules/windows/web/SearchBarWidget.ui
	src/modules/windows/web/SelectPasswordDialog.ui
	src/modules/windows/web/StartPagePreferencesDialog.ui
)

if (Qt5WebEngineWidgets_FOUND AND ENABLE_QTWEBENGINE)
	add_definitions(-DOTTER_ENABLE_QTWEBENGINE)

	list(APPEND OTTER_BACKENDS_WEB "QtWebEngine")

	set(otter_src
		${otter_src}
		src/modules/backends/web/qtwebengine/QtWebEnginePage.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineUrlRequestInterceptor.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineTransfer.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebBackend.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebWidget.cpp
	)

	qt5_add_resources(otter_res
		src/modules/backends/web/qtwebengine/QtWebEngineResources.qrc
	)
endif (Qt5WebEngineWidgets_FOUND AND ENABLE_QTWEBENGINE)

if (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)
	add_definitions(-DOTTER_ENABLE_QTWEBKIT)

	if (${Qt5WebKitWidgets_VERSION_MINOR} GREATER 600)
		list(APPEND OTTER_BACKENDS_WEB "QtWebKit")
	else (${Qt5WebKitWidgets_VERSION_MINOR} GREATER 600)
		add_definitions(-DOTTER_ENABLE_QTWEBKIT_LEGACY)

		list(APPEND OTTER_BACKENDS_WEB "QtWebKit (legacy)")
	endif (${Qt5WebKitWidgets_VERSION_MINOR} GREATER 600)

	set(otter_src
		${otter_src}
		src/modules/backends/web/qtwebkit/QtWebKitCookieJar.cpp
		src/modules/backends/web/qtwebkit/QtWebKitFtpListingNetworkReply.cpp
		src/modules/backends/web/qtwebkit/QtWebKitHistoryInterface.cpp
		src/modules/backends/web/qtwebkit/QtWebKitInspector.cpp
		src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.cpp
		src/modules/backends/web/qtwebkit/QtWebKitPage.cpp
		src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.cpp
		src/modules/backends/web/qtwebkit/QtWebKitPluginWidget.cpp
		src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp
		src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp
		src/modules/backends/web/qtwebkit/3rdparty/qtftp/qftp.cpp
		src/modules/backends/web/qtwebkit/3rdparty/qtftp/qurlinfo.cpp
	)

	qt5_add_resources(otter_res
		src/modules/backends/web/qtwebkit/QtWebKitResources.qrc
	)
endif (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)

if (ENABLE_CRASHREPORTS)
	add_definitions(-DOTTER_ENABLE_CRASHREPORTS)
	include_directories(${CMAKE_SOURCE_DIR}/3rdparty/breakpad/src/)

	if (WIN32)
		set(otter_src
			${otter_src}
			3rdparty/breakpad/src/common/windows/guid_string.cc
			3rdparty/breakpad/src/client/windows/crash_generation/crash_generation_client.cc
			3rdparty/breakpad/src/client/windows/handler/exception_handler.cc
		)
	elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(otter_src
			${otter_src}
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
	endif (WIN32)
endif (ENABLE_CRASHREPORTS)

if (HUNSPELL_FOUND)
	add_definitions(-DOTTER_ENABLE_SPELLCHECK)
	add_definitions(-DSONNET_STATIC)
	add_definitions(-DQT_STATICPLUGIN)
	include_directories(${HUNSPELL_INCLUDE_DIR})

	set(otter_src
		${otter_src}
		3rdparty/sonnet/src/core/client.cpp
		3rdparty/sonnet/src/core/loader.cpp
		3rdparty/sonnet/src/core/speller.cpp
		3rdparty/sonnet/src/core/spellerplugin.cpp
		3rdparty/sonnet/src/core/settings.cpp
		3rdparty/sonnet/src/core/textbreaks.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspellclient.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspelldebug.cpp
		3rdparty/sonnet/src/plugins/hunspell/hunspelldict.cpp
	)

	if (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)
		set(otter_src
			${otter_src}
			src/modules/backends/web/qtwebkit/qwebkitplatformplugin.h
			src/modules/backends/web/qtwebkit/QtWebKitPlugin.cpp
			src/modules/backends/web/qtwebkit/QtWebKitSpellChecker.cpp
		)
	endif (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)
endif (HUNSPELL_FOUND)

if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)

	find_package(Qt5WinExtras 5.3.0 QUIET)

	if (MSVC)
		set(CMAKE_PREFIX_PATH $ENV{QTDIR})
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /GL /Zi")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /DEBUG /OPT:REF")

		if (CMAKE_CL_64)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
		else (CMAKE_CL_64)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS,5.01 /ENTRY:\"mainCRTStartup\"")
		endif (CMAKE_CL_64)
	endif (MSVC)

	set(otter_src
		${otter_src}
		otter-browser.rc
		src/modules/platforms/windows/WindowsPlatformIntegration.cpp
	)
elseif (APPLE)
	find_package(Qt5MacExtras 5.3.0 QUIET)

	set(MACOSX_BUNDLE_BUNDLE_NAME "Otter Browser")
	set(MACOSX_BUNDLE_BUNDLE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
	set(MACOSX_BUNDLE_LONG_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
	set(MACOSX_BUNDLE_ICON_FILE otter-browser.icns)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.otter-browser.otter-browser")
	set(MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2013-2016 Otter Browser Team. All rights reserved.")
	set(otter_src
		${otter_src}
		src/modules/platforms/mac/MacPlatformIntegration.mm
		src/modules/platforms/mac/MacPlatformStyle.cpp
		resources/icons/otter-browser.icns
	)

	set_source_files_properties(resources/icons/otter-browser.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif (UNIX)
	set(otter_src
		${otter_src}
		src/modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.cpp
		3rdparty/libmimeapps/ConfigReader.cpp
		3rdparty/libmimeapps/DesktopEntry.cpp
		3rdparty/libmimeapps/Index.cpp
		3rdparty/libmimeapps/Tools.cpp
	)
endif (WIN32)

add_executable(otter-browser WIN32 MACOSX_BUNDLE
	${otter_ui}
	${otter_res}
	${otter_src}
)

if (Qt5WebEngineWidgets_FOUND AND ENABLE_QTWEBENGINE)
	target_link_libraries(otter-browser Qt5::WebEngineCore Qt5::WebEngineWidgets)
endif (Qt5WebEngineWidgets_FOUND AND ENABLE_QTWEBENGINE)

if (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)
	target_link_libraries(otter-browser Qt5::WebKit Qt5::WebKitWidgets)
endif (Qt5WebKitWidgets_FOUND AND ENABLE_QTWEBKIT)

if (HUNSPELL_FOUND)
	target_link_libraries(otter-browser ${HUNSPELL_LIBRARIES})
endif (HUNSPELL_FOUND)

if (WIN32)
	target_link_libraries(otter-browser Qt5::WinExtras ole32 shell32 advapi32 user32)
elseif (APPLE)
	find_library(FRAMEWORK_Cocoa Cocoa)
	find_library(FRAMEWORK_Foundation Foundation)

	target_link_libraries(otter-browser Qt5::MacExtras ${FRAMEWORK_Cocoa} ${FRAMEWORK_Foundation})
elseif (UNIX)
	target_link_libraries(otter-browser Qt5::DBus)

	if (ENABLE_CRASHREPORTS)
		target_link_libraries(otter-browser -lpthread)
	endif (ENABLE_CRASHREPORTS)
endif (WIN32)

target_link_libraries(otter-browser Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::Network Qt5::PrintSupport Qt5::Qml Qt5::Widgets Qt5::XmlPatterns)

set(OTTER_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications CACHE FILEPATH "Install path for .desktop files")

file(GLOB _qm_files resources/translations/*.qm)

foreach(_current_QM_FILE ${_qm_files})
	install(FILES ${_current_QM_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/otter-browser/locale/)
endforeach(_current_QM_FILE)

install(FILES resources/icons/otter-browser-16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps/ RENAME otter-browser.png)
install(FILES otter-browser.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES man/otter-browser.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
install(TARGETS otter-browser DESTINATION bin/)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

list(LENGTH OTTER_BACKENDS_WEB OTTER_BACKENDS_WEB_LENGTH)

if (${OTTER_BACKENDS_WEB_LENGTH} EQUAL 0)
	message(SEND_ERROR "No web backends available!")
else (${OTTER_BACKENDS_WEB_LENGTH} EQUAL 0)
	set(OTTER_BACKENDS_WEB_SUMMARY "The following web backends will be built:\n")

	foreach(_backend ${OTTER_BACKENDS_WEB})
		set(OTTER_BACKENDS_WEB_SUMMARY "${OTTER_BACKENDS_WEB_SUMMARY} * ${_backend}\n")
	endforeach(_backend)

	message(STATUS ${OTTER_BACKENDS_WEB_SUMMARY})
endif (${OTTER_BACKENDS_WEB_LENGTH} EQUAL 0)
