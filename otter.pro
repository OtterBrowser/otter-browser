#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

QT += core network gui printsupport widgets webkitwidgets

TARGET = otter
TEMPLATE = app

SOURCES += src/main.cpp\
    src/ui/MainWindow.cpp \
    src/ui/CookiesDialog.cpp \
    src/ui/PreferencesDialog.cpp \
    src/core/SettingsManager.cpp \
    src/core/Application.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/Window.cpp \
    src/core/WindowsManager.cpp \
    src/core/ActionsManager.cpp \
    src/core/NetworkAccessManager.cpp \
    src/ui/StatusBarWidget.cpp \
    src/backends/web/WebBackend.cpp \
    src/backends/web/qtwebkit/WebBackendWebKit.cpp \
    src/backends/web/WebWidget.cpp \
    src/backends/web/qtwebkit/WebWidgetWebKit.cpp \
    src/backends/web/WebBackendsManager.cpp \
    src/backends/web/qtwebkit/WebPageWebKit.cpp \
    src/core/SessionsManager.cpp \
    src/core/LocalListingNetworkReply.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/ProgressBarWidget.cpp \
    src/core/BookmarksManager.cpp \
    src/ui/BookmarkDialog.cpp \
    src/ui/AuthenticationDialog.cpp

HEADERS += src/ui/MainWindow.h \
    src/ui/CookiesDialog.h \
    src/ui/PreferencesDialog.h \
    src/core/SettingsManager.h \
    src/core/Application.h \
    src/ui/TabBarWidget.h \
    src/ui/Window.h \
    src/core/WindowsManager.h \
    src/core/ActionsManager.h \
    src/core/NetworkAccessManager.h \
    src/ui/StatusBarWidget.h \
    src/backends/web/WebBackend.h \
    src/backends/web/qtwebkit/WebBackendWebKit.h \
    src/backends/web/WebWidget.h \
    src/backends/web/qtwebkit/WebWidgetWebKit.h \
    src/backends/web/WebBackendsManager.h \
    src/backends/web/qtwebkit/WebPageWebKit.h \
    src/core/SessionsManager.h \
    src/core/LocalListingNetworkReply.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/SaveSessionDialog.h \
    src/ui/ProgressBarWidget.h \
    src/core/BookmarksManager.h \
    src/ui/BookmarkDialog.h \
    src/ui/AuthenticationDialog.h

FORMS += src/ui/MainWindow.ui \
    src/ui/CookiesDialog.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/Window.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/BookmarkDialog.ui \
    src/ui/AuthenticationDialog.ui

RESOURCES += \
    resources/resources.qrc
