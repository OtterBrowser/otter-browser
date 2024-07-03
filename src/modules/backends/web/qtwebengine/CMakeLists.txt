if (ENABLE_QT6)
	find_package(Qt6 ${MIN_VERSION_QT6} QUIET COMPONENTS WebEngineWidgets)

	set_package_properties(Qt6WebEngineCore PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (core)" TYPE OPTIONAL)
	set_package_properties(Qt6WebEngineWidgets PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (widgets)" TYPE OPTIONAL)

	if (NOT TARGET Qt6::WebEngineWidgets)
		return ()
	endif ()

	add_definitions(-DOTTER_ENABLE_QTWEBENGINE)

	list(APPEND OTTER_BACKENDS_WEB "QtWebEngine")
	list(APPEND OTTER_LINK_LIBRARIES Qt6::WebEngineCore Qt6::WebEngineWidgets)

	set(OTTER_SOURCES
		${OTTER_SOURCES}
		src/modules/backends/web/qtwebengine/QtWebEnginePage.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineUrlRequestInterceptor.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineTransfer.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebBackend.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebWidget.cpp
	)

	qt6_add_resources(OTTER_RESOURCES
		src/modules/backends/web/qtwebengine/QtWebEngineResources.qrc
	)
else ()
	find_package(Qt5 ${MIN_VERSION_QT5} QUIET COMPONENTS WebEngineWidgets)

	set_package_properties(Qt5WebEngineCore PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (core)" TYPE OPTIONAL)
	set_package_properties(Qt5WebEngineWidgets PROPERTIES URL "https://www.qt.io/" DESCRIPTION "QtWebEngine based backend (widgets)" TYPE OPTIONAL)

	if (NOT TARGET Qt5::WebEngineWidgets)
		return ()
	endif ()

	add_definitions(-DOTTER_ENABLE_QTWEBENGINE)

	list(APPEND OTTER_BACKENDS_WEB "QtWebEngine")
	list(APPEND OTTER_LINK_LIBRARIES Qt5::WebEngineCore Qt5::WebEngineWidgets)

	set(OTTER_SOURCES
		${OTTER_SOURCES}
		src/modules/backends/web/qtwebengine/QtWebEnginePage.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineUrlRequestInterceptor.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineTransfer.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebBackend.cpp
		src/modules/backends/web/qtwebengine/QtWebEngineWebWidget.cpp
	)

	qt5_add_resources(OTTER_RESOURCES
		src/modules/backends/web/qtwebengine/QtWebEngineResources.qrc
	)
endif ()
