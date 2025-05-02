if (NOT ENABLE_QTWEBENGINE)
	return ()
endif ()

find_package(${QT_MAJOR_VERSION} ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)

set_package_properties(${QT_MAJOR_VERSION}WebEngineCore PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (core)" TYPE OPTIONAL)
set_package_properties(${QT_MAJOR_VERSION}WebEngineWidgets PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (widgets)" TYPE OPTIONAL)

if (NOT TARGET ${QT_MAJOR_VERSION}::WebEngineWidgets)
	return ()
endif ()

add_definitions(-DOTTER_ENABLE_QTWEBENGINE)

list(APPEND OTTER_BACKENDS_WEB "QtWebEngine")
list(APPEND OTTER_LINK_LIBRARIES ${QT_MAJOR_VERSION}::WebEngineCore ${QT_MAJOR_VERSION}::WebEngineWidgets)

set(OTTER_SOURCES
	${OTTER_SOURCES}
	src/modules/backends/web/qtwebengine/QtWebEnginePage.cpp
	src/modules/backends/web/qtwebengine/QtWebEngineUrlRequestInterceptor.cpp
	src/modules/backends/web/qtwebengine/QtWebEngineTransfer.cpp
	src/modules/backends/web/qtwebengine/QtWebEngineWebBackend.cpp
	src/modules/backends/web/qtwebengine/QtWebEngineWebWidget.cpp
)

qt_add_resources(OTTER_RESOURCES
	src/modules/backends/web/qtwebengine/QtWebEngineResources.qrc
)

if (WIN32)
	if (NOT ENABLE_QT6)
		find_package(${QT_MAJOR_VERSION} ${QT_MINIMUM_VERSION}} QUIET COMPONENTS WinExtras)
	endif ()
elseif (APPLE)
	if (NOT ENABLE_QT6)
		find_package(${QT_MAJOR_VERSION} ${QT_MINIMUM_VERSION} QUIET COMPONENTS MacExtras)
	endif ()
elseif (UNIX)
	find_package(${QT_MAJOR_VERSION} ${QT_MINIMUM_VERSION} QUIET COMPONENTS DBus)
endif ()
