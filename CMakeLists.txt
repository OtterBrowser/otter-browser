cmake_minimum_required(VERSION 2.8.11)

project(otter-browser)

set(MAJOR_VERSION "0")
set(MINOR_VERSION "1")
set(PATCH_VERSION "01")

if (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
	include(InstallRequiredSystemLibraries)

	set(CPACK_SET_DESTDIR "on")
	set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
	set(CPACK_GENERATOR "DEB")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Web browser controlled by the user, not vice-versa")
	set(CPACK_PACKAGE_VENDOR "Vendor")
	set(CPACK_PACKAGE_CONTACT "Michal Dutkiewicz")
	set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
	set(CPACK_PACKAGE_NAME "otter-browser")
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
	set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5 (>=5:5.2.0)")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "net")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_COMPONENTS_ALL Libraries ApplicationData)

	include(CPack)
endif (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Sensors REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebKit REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)

set(otter_src
	src/main.cpp
	src/core/ActionsManager.cpp
	src/core/Application.cpp
	src/core/BookmarksManager.cpp
	src/core/CookieJar.cpp
	src/core/FileSystemCompleterModel.cpp
	src/core/HistoryManager.cpp
	src/core/LocalListingNetworkReply.cpp
	src/core/NetworkAccessManager.cpp
	src/core/NetworkCache.cpp
	src/core/SearchesManager.cpp
	src/core/SearchSuggester.cpp
	src/core/SessionsManager.cpp
	src/core/SettingsManager.cpp
	src/core/TransfersManager.cpp
	src/core/Utils.cpp
	src/core/WebBackend.cpp
	src/core/WebBackendsManager.cpp
	src/core/WindowsManager.cpp
	src/ui/AddressWidget.cpp
	src/ui/AuthenticationDialog.cpp
	src/ui/BookmarkPropertiesDialog.cpp
	src/ui/ClearHistoryDialog.cpp
	src/ui/ContentsDialog.cpp
	src/ui/ContentsWidget.cpp
	src/ui/ItemDelegate.cpp
	src/ui/MainWindow.cpp
	src/ui/OptionDelegate.cpp
	src/ui/OptionWidget.cpp
	src/ui/PreferencesDialog.cpp
	src/ui/PreviewWidget.cpp
	src/ui/SaveSessionDialog.cpp
	src/ui/SearchDelegate.cpp
	src/ui/SearchPropertiesDialog.cpp
	src/ui/SearchWidget.cpp
	src/ui/SessionsManagerDialog.cpp
	src/ui/StatusBarWidget.cpp
	src/ui/TabBarWidget.cpp
	src/ui/TextLabelWidget.cpp
	src/ui/WebWidget.cpp
	src/ui/Window.cpp
	src/ui/preferences/BlockedContentDialog.cpp
	src/ui/preferences/ShortcutDelegate.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebPage.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp
	src/modules/windows/bookmarks/BookmarksContentsWidget.cpp
	src/modules/windows/cache/CacheContentsWidget.cpp
	src/modules/windows/configuration/ConfigurationContentsWidget.cpp
	src/modules/windows/cookies/CookiePropertiesDialog.cpp
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
	src/ui/ClearHistoryDialog.ui
	src/ui/MainWindow.ui
	src/ui/PreferencesDialog.ui
	src/ui/SaveSessionDialog.ui
	src/ui/SearchPropertiesDialog.ui
	src/ui/SessionsManagerDialog.ui
	src/ui/Window.ui
	src/ui/preferences/BlockedContentDialog.ui
	src/modules/windows/bookmarks/BookmarksContentsWidget.ui
	src/modules/windows/cache/CacheContentsWidget.ui
	src/modules/windows/configuration/ConfigurationContentsWidget.ui
	src/modules/windows/cookies/CookiePropertiesDialog.ui
	src/modules/windows/cookies/CookiesContentsWidget.ui
	src/modules/windows/history/HistoryContentsWidget.ui
	src/modules/windows/transfers/TransfersContentsWidget.ui
	src/modules/windows/web/ImagePropertiesDialog.ui
	src/modules/windows/web/WebContentsWidget.ui
)

add_executable(otter-browser
	${otter_ui}
	${otter_res}
	${otter_src}
)

target_link_libraries(otter-browser
	Qt5::Core
	Qt5::Network
	Qt5::PrintSupport
	Qt5::Widgets
	Qt5::WebKitWidgets
)

install(TARGETS otter-browser DESTINATION bin)
