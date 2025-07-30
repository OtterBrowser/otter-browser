if (NOT ENABLE_QTWEBKIT)
	return ()
endif ()

find_package(${QT_MAJOR_VERSION}WebKitWidgets ${QT_MINIMUM_VERSION_WEBKIT} REQUIRED)

set_package_properties(${QT_MAJOR_VERSION}WebKit PROPERTIES URL "https://qtwebkit.github.io/" DESCRIPTION "QtWebKit based backend (core)" OPTIONAL)
set_package_properties(${QT_MAJOR_VERSION}WebKitWidgets PROPERTIES URL "https://qtwebkit.github.io/" DESCRIPTION "QtWebKit based backend (widgets)" TYPE OPTIONAL)

if (NOT TARGET ${QT_MAJOR_VERSION}::WebKitWidgets)
	return ()
endif ()

add_definitions(-DOTTER_ENABLE_QTWEBKIT -DOTTER_QTWEBKIT_PLUGINS_AVAILABLE -DQT_STATICPLUGIN)

list(APPEND OTTER_BACKENDS_WEB "QtWebKit")
list(APPEND OTTER_LINK_LIBRARIES ${QT_MAJOR_VERSION}::WebKit ${QT_MAJOR_VERSION}::WebKitWidgets)

set(OTTER_SOURCES
	${OTTER_SOURCES}
	src/modules/backends/web/qtwebkit/qwebkitplatformplugin.h
	src/modules/backends/web/qtwebkit/QtWebKitCookieJar.cpp
	src/modules/backends/web/qtwebkit/QtWebKitFtpListingNetworkReply.cpp
	src/modules/backends/web/qtwebkit/QtWebKitHistoryInterface.cpp
	src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.cpp
	src/modules/backends/web/qtwebkit/QtWebKitNotificationPresenter.cpp
	src/modules/backends/web/qtwebkit/QtWebKitPage.cpp
	src/modules/backends/web/qtwebkit/QtWebKitPlugin.cpp
	src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp
	src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp
	src/modules/backends/web/qtwebkit/3rdparty/qtftp/qftp.cpp
	src/modules/backends/web/qtwebkit/3rdparty/qtftp/qurlinfo.cpp
)

if (TARGET Hunspell::Hunspell AND ENABLE_SPELLCHECK)
	set(OTTER_SOURCES
		${OTTER_SOURCES}
		src/modules/backends/web/qtwebkit/QtWebKitSpellChecker.cpp
	)
endif ()

qt_add_resources(OTTER_RESOURCES
	src/modules/backends/web/qtwebkit/QtWebKitResources.qrc
)
