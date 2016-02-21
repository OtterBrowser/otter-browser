cmake_minimum_required(VERSION 2.8.12)

project(Sonnet)

include_directories(${HUNSPELL_INCLUDE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(REQUIRED_QT_VERSION 5.3.0)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Widgets)
find_package(Hunspell REQUIRED)

set(sonnetcore_SRCS
	src/core/loader.cpp
	src/core/client.cpp
	src/core/spellerplugin.cpp
	src/core/speller.cpp
	src/core/settings.cpp
	src/core/textbreaks.cpp
	src/plugins/hunspell/hunspellclient.cpp
	src/plugins/hunspell/hunspelldict.cpp
	src/plugins/hunspell/hunspelldebug.cpp
)

# Dear packagers, this is just used as an extra search paths for plugins. Don't get your panties in a twist.
add_definitions(-DINSTALLATION_PLUGIN_PATH="${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_PLUGINDIR}")

add_library(KF5SonnetCore ${sonnetcore_SRCS})

target_link_libraries(KF5SonnetCore PUBLIC Qt5::Core ${HUNSPELL_LIBRARIES})

set_target_properties(KF5SonnetCore PROPERTIES VERSION ${SONNET_VERSION_STRING}
                                               SOVERSION ${SONNET_SOVERSION}
                                               EXPORT_NAME SonnetCore
)

# CMAKE_CURRENT_BINARY_DIR: for camelcase headers and lowercase forwarders
target_include_directories(KF5SonnetCore INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/SonnetCore>")
target_include_directories(KF5SonnetCore PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
