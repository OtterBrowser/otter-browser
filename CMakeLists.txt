cmake_minimum_required(VERSION 2.8.10.2)

project(otter-browser)

set(MAJOR_VERSION "0")
set(MINOR_VERSION "9")
set(PATCH_VERSION "04")
set(OTTER_VERSION_MAIN "0.9.04")
set(OTTER_VERSION_CONTEXT "-dev")

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
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5webkit5 (>=5.1.0), libqt5script5 (>=5.2.0)")
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gstreamer0.10-plugins-base, gstreamer0.10-plugins-good")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "net")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
	set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
		${CMAKE_INSTALL_PREFIX}/share/applications
		/usr/share/icons
		/usr/share/icons/hicolor
		/usr/share/icons/hicolor/16x16
		/usr/share/icons/hicolor/32x32
		/usr/share/icons/hicolor/48x48
		/usr/share/icons/hicolor/64x64
		/usr/share/icons/hicolor/128x128
		/usr/share/icons/hicolor/256x256
		/usr/share/icons/hicolor/16x16/apps
		/usr/share/icons/hicolor/32x32/apps
		/usr/share/icons/hicolor/48x48/apps
		/usr/share/icons/hicolor/64x64/apps
		/usr/share/icons/hicolor/128x128/apps
		/usr/share/icons/hicolor/256x256/apps
	)

	include(CPack)
endif (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebKit REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

set(otter_src
	src/main.cpp
	src/core/Action.cpp
	src/core/ActionsManager.cpp
	src/core/AddressCompletionModel.cpp
	src/core/Application.cpp
	src/core/BookmarksImporter.cpp
	src/core/BookmarksManager.cpp
	src/core/BookmarksModel.cpp
	src/core/ContentBlockingList.cpp
	src/core/ContentBlockingManager.cpp
	src/core/Console.cpp
	src/core/CookieJar.cpp
	src/core/FileSystemCompleterModel.cpp
	src/core/HistoryManager.cpp
	src/core/Importer.cpp
	src/core/LocalListingNetworkReply.cpp
	src/core/NetworkAutomaticProxy.cpp
	src/core/NetworkCache.cpp
	src/core/NetworkManager.cpp
	src/core/NetworkManagerFactory.cpp
	src/core/NetworkProxyFactory.cpp
	src/core/PlatformIntegration.cpp
	src/core/SearchesManager.cpp
	src/core/SearchSuggester.cpp
	src/core/SessionsManager.cpp
	src/core/SettingsManager.cpp
	src/core/ShortcutsManager.cpp
	src/core/TransfersManager.cpp
	src/core/Utils.cpp
	src/core/WebBackend.cpp
	src/core/WebBackendsManager.cpp
	src/core/WindowsManager.cpp
	src/ui/ActionWidget.cpp
	src/ui/AddressWidget.cpp
	src/ui/AuthenticationDialog.cpp
	src/ui/BookmarkPropertiesDialog.cpp
	src/ui/BookmarksComboBoxWidget.cpp
	src/ui/BookmarksImporterWidget.cpp
	src/ui/ClearHistoryDialog.cpp
	src/ui/ConsoleWidget.cpp
	src/ui/ContentsDialog.cpp
	src/ui/ContentsWidget.cpp
	src/ui/FilePathWidget.cpp
	src/ui/GoBackActionWidget.cpp
	src/ui/GoForwardActionWidget.cpp
	src/ui/ImportDialog.cpp
	src/ui/ItemDelegate.cpp
	src/ui/LocaleDialog.cpp
	src/ui/MainWindow.cpp
	src/ui/MdiWidget.cpp
	src/ui/Menu.cpp
	src/ui/OpenAddressDialog.cpp
	src/ui/OpenBookmarkDialog.cpp
	src/ui/OptionDelegate.cpp
	src/ui/OptionWidget.cpp
	src/ui/PreferencesDialog.cpp
	src/ui/PreviewWidget.cpp
	src/ui/ReloadTimeDialog.cpp
	src/ui/SaveSessionDialog.cpp
	src/ui/SearchDelegate.cpp
	src/ui/SearchPropertiesDialog.cpp
	src/ui/SearchWidget.cpp
	src/ui/SessionsManagerDialog.cpp
	src/ui/SidebarWidget.cpp
	src/ui/StartupDialog.cpp
	src/ui/StatusBarWidget.cpp
	src/ui/TabBarDockWidget.cpp
	src/ui/TabBarWidget.cpp
	src/ui/TableViewWidget.cpp
	src/ui/TextLabelWidget.cpp
	src/ui/TrayIcon.cpp
	src/ui/UserAgentsManagerDialog.cpp
	src/ui/WebsitePreferencesDialog.cpp
	src/ui/WebWidget.cpp
	src/ui/Window.cpp
	src/ui/preferences/AcceptLanguageDialog.cpp
	src/ui/preferences/ContentBlockingDialog.cpp
	src/ui/preferences/JavaScriptPreferencesDialog.cpp
	src/ui/preferences/KeyboardShortcutDelegate.cpp
	src/ui/preferences/SearchKeywordDelegate.cpp
	src/ui/preferences/ShortcutsProfileDialog.cpp
	src/ui/toolbars/ZoomWidget.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebPage.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp
	src/modules/importers/html/HtmlBookmarksImporter.cpp
	src/modules/importers/opera/OperaBookmarksImporter.cpp
	src/modules/windows/bookmarks/BookmarksContentsWidget.cpp
	src/modules/windows/cache/CacheContentsWidget.cpp
	src/modules/windows/configuration/ConfigurationContentsWidget.cpp
	src/modules/windows/cookies/CookiesContentsWidget.cpp
	src/modules/windows/history/HistoryContentsWidget.cpp
	src/modules/windows/transfers/ProgressBarDelegate.cpp
	src/modules/windows/transfers/TransfersContentsWidget.cpp
	src/modules/windows/web/ImagePropertiesDialog.cpp
	src/modules/windows/web/ProgressBarWidget.cpp
	src/modules/windows/web/WebContentsWidget.cpp
)

qt5_add_resources(otter_res
	resources/resources.qrc
)

qt5_wrap_ui(otter_ui
	src/ui/AuthenticationDialog.ui
	src/ui/BookmarkPropertiesDialog.ui
	src/ui/BookmarksImporterWidget.ui
	src/ui/ClearHistoryDialog.ui
	src/ui/ConsoleWidget.ui
	src/ui/ImportDialog.ui
	src/ui/LocaleDialog.ui
	src/ui/MainWindow.ui
	src/ui/OpenAddressDialog.ui
	src/ui/OpenBookmarkDialog.ui
	src/ui/PreferencesDialog.ui
	src/ui/ReloadTimeDialog.ui
	src/ui/SaveSessionDialog.ui
	src/ui/SearchPropertiesDialog.ui
	src/ui/SessionsManagerDialog.ui
	src/ui/SidebarWidget.ui
	src/ui/StartupDialog.ui
	src/ui/UserAgentsManagerDialog.ui
	src/ui/WebsitePreferencesDialog.ui
	src/ui/preferences/AcceptLanguageDialog.ui
	src/ui/preferences/ContentBlockingDialog.ui
	src/ui/preferences/JavaScriptPreferencesDialog.ui
	src/ui/preferences/ShortcutsProfileDialog.ui
	src/modules/windows/bookmarks/BookmarksContentsWidget.ui
	src/modules/windows/cache/CacheContentsWidget.ui
	src/modules/windows/configuration/ConfigurationContentsWidget.ui
	src/modules/windows/cookies/CookiesContentsWidget.ui
	src/modules/windows/history/HistoryContentsWidget.ui
	src/modules/windows/transfers/TransfersContentsWidget.ui
	src/modules/windows/web/ImagePropertiesDialog.ui
	src/modules/windows/web/WebContentsWidget.ui
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(otter_src ${otter_src} src/modules/platforms/windows/WindowsPlatformIntegration.cpp)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

add_executable(otter-browser
	${otter_ui}
	${otter_res}
	${otter_src}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	qt5_use_modules(otter-browser WinExtras)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

qt5_use_modules(otter-browser PrintSupport Script Sql WebKitWidgets)

set(OTTER_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(XDG_APPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/applications CACHE FILEPATH "Install path for .desktop files")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB _qm_files resources/translations/*.qm)

foreach(_current_QM_FILE ${_qm_files})
	install(FILES ${_current_QM_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/otter-browser/locale/)
endforeach(_current_QM_FILE)

install(FILES resources/icons/otter-browser-16.png DESTINATION /usr/share/icons/hicolor/16x16/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-32.png DESTINATION /usr/share/icons/hicolor/32x32/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-48.png DESTINATION /usr/share/icons/hicolor/48x48/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-64.png DESTINATION /usr/share/icons/hicolor/64x64/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-128.png DESTINATION /usr/share/icons/hicolor/128x128/apps/ RENAME otter-browser.png)
install(FILES resources/icons/otter-browser-256.png DESTINATION /usr/share/icons/hicolor/256x256/apps/ RENAME otter-browser.png)
install(FILES otter-browser.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(TARGETS otter-browser DESTINATION bin)
