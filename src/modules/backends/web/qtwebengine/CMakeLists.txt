find_package(Qt5 5.15.0 QUIET COMPONENTS WebEngineWidgets)

set_package_properties(Qt5WebEngineCore PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (core)" TYPE OPTIONAL)
set_package_properties(Qt5WebEngineWidgets PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (widgets)" TYPE OPTIONAL)

if (NOT TARGET Qt5::WebEngineWidgets)
	return ()
endif ()

add_definitions(-DOTTER_ENABLE_QTWEBENGINE)

list(APPEND OTTER_BACKENDS_WEB "QtWebEngine")
list(APPEND OTTER_LINK_LIBRARIES Qt5::WebEngineCore Qt5::WebEngineWidgets)

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
