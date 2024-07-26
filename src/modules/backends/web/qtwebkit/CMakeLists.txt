find_package(Qt5WebKitWidgets 5.212.0 QUIET)

set_package_properties(Qt5WebKit PROPERTIES URL "https://qtwebkit.github.io/" DESCRIPTION "QtWebKit based backend (core)" TYPE OPTIONAL)
set_package_properties(Qt5WebKitWidgets PROPERTIES URL "https://qtwebkit.github.io/" DESCRIPTION "QtWebKit based backend (widgets)" TYPE OPTIONAL)

if (NOT TARGET Qt5::WebKitWidgets)
	return ()
endif ()

add_definitions(-DOTTER_ENABLE_QTWEBKIT -DOTTER_QTWEBKIT_PLUGINS_AVAILABLE -DQT_STATICPLUGIN)

list(APPEND OTTER_BACKENDS_WEB "QtWebKit")

set(otter_src
	${otter_src}
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
	set(otter_src
		${otter_src}
		src/modules/backends/web/qtwebkit/QtWebKitSpellChecker.cpp
	)
endif ()

qt5_add_resources(otter_res
	src/modules/backends/web/qtwebkit/QtWebKitResources.qrc
)
