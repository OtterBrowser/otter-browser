#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

QT += core gui network printsupport webkitwidgets widgets

TARGET = otter
TEMPLATE = app

SOURCES += src/main.cpp\
    src/core/SettingsManager.cpp \
    src/core/Application.cpp \
    src/core/WindowsManager.cpp \
    src/core/ActionsManager.cpp \
    src/core/NetworkAccessManager.cpp \
    src/core/SessionsManager.cpp \
    src/core/LocalListingNetworkReply.cpp \
    src/core/BookmarksManager.cpp \
    src/core/CookieJar.cpp \
    src/core/TransfersManager.cpp \
    src/core/Utils.cpp \
    src/ui/AddressWidget.cpp \
    src/ui/MainWindow.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/Window.cpp \
    src/ui/StatusBarWidget.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/BookmarkDialog.cpp \
    src/ui/AuthenticationDialog.cpp \
    src/ui/PreviewWidget.cpp \
    src/ui/ImagePropertiesDialog.cpp \
    src/ui/ContentsWidget.cpp \
    src/backends/web/WebBackend.cpp \
    src/backends/web/WebWidget.cpp \
    src/backends/web/WebBackendsManager.cpp \
    src/backends/web/qtwebkit/WebBackendWebKit.cpp \
    src/backends/web/qtwebkit/WebWidgetWebKit.cpp \
    src/backends/web/qtwebkit/WebPageWebKit.cpp \
    src/modules/windows/web/WebContentsWidget.cpp \
    src/modules/windows/web/ProgressBarWidget.cpp \
    src/modules/windows/cookies/CookiesContentsWidget.cpp \
    src/modules/windows/cookies/CookiePropertiesDialog.cpp \
    src/modules/windows/transfers/TransfersContentsWidget.cpp \
    src/modules/windows/transfers/ProgressBarDelegate.cpp

HEADERS += src/core/SettingsManager.h \
    src/core/Application.h \
    src/core/WindowsManager.h \
    src/core/ActionsManager.h \
    src/core/NetworkAccessManager.h \
    src/core/SessionsManager.h \
    src/core/LocalListingNetworkReply.h \
    src/core/BookmarksManager.h \
    src/core/CookieJar.h \
    src/core/TransfersManager.h \
    src/core/Utils.h \
    src/ui/AddressWidget.h \
    src/ui/MainWindow.h \
    src/ui/PreferencesDialog.h \
    src/ui/TabBarWidget.h \
    src/ui/Window.h \
    src/ui/StatusBarWidget.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/SaveSessionDialog.h \
    src/ui/BookmarkDialog.h \
    src/ui/AuthenticationDialog.h \
    src/ui/PreviewWidget.h \
    src/ui/ImagePropertiesDialog.h \
    src/ui/ContentsWidget.h \
    src/backends/web/WebBackend.h \
    src/backends/web/WebWidget.h \
    src/backends/web/WebBackendsManager.h \
    src/backends/web/qtwebkit/WebBackendWebKit.h \
    src/backends/web/qtwebkit/WebWidgetWebKit.h \
    src/backends/web/qtwebkit/WebPageWebKit.h \
    src/modules/windows/web/WebContentsWidget.h \
    src/modules/windows/web/ProgressBarWidget.h \
    src/modules/windows/cookies/CookiesContentsWidget.h \
    src/modules/windows/cookies/CookiePropertiesDialog.h \
    src/modules/windows/transfers/TransfersContentsWidget.h \
    src/modules/windows/transfers/ProgressBarDelegate.h

FORMS += src/ui/MainWindow.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/Window.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/BookmarkDialog.ui \
    src/ui/AuthenticationDialog.ui \
    src/ui/ImagePropertiesDialog.ui \
    src/modules/windows/web/WebContentsWidget.ui \
    src/modules/windows/cookies/CookiesContentsWidget.ui \
    src/modules/windows/cookies/CookiePropertiesDialog.ui \
    src/modules/windows/transfers/TransfersContentsWidget.ui

RESOURCES += \
    resources/resources.qrc
