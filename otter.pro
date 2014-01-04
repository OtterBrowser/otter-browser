#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

QT += core gui network printsupport sensors sql webkitwidgets widgets

TARGET = otter-browser
TEMPLATE = app

SOURCES += src/main.cpp \
    src/core/ActionsManager.cpp \
    src/core/AddressCompletionModel.cpp \
    src/core/Application.cpp \
    src/core/BookmarksManager.cpp \
    src/core/CookieJar.cpp \
    src/core/FileSystemCompleterModel.cpp \
    src/core/HistoryManager.cpp \
    src/core/LocalListingNetworkReply.cpp \
    src/core/NetworkAccessManager.cpp \
    src/core/NetworkCache.cpp \
    src/core/SearchesManager.cpp \
    src/core/SearchSuggester.cpp \
    src/core/SessionsManager.cpp \
    src/core/SettingsManager.cpp \
    src/core/TransfersManager.cpp \
    src/core/Utils.cpp \
    src/core/WebBackend.cpp \
    src/core/WebBackendsManager.cpp \
    src/core/WindowsManager.cpp \
    src/ui/AddressWidget.cpp \
    src/ui/AuthenticationDialog.cpp \
    src/ui/BookmarkPropertiesDialog.cpp \
    src/ui/BookmarksImportDialog.cpp \
    src/ui/ClearHistoryDialog.cpp \
    src/ui/ContentsDialog.cpp \
    src/ui/ContentsWidget.cpp \
    src/ui/ItemDelegate.cpp \
    src/ui/MainWindow.cpp \
    src/ui/OptionDelegate.cpp \
    src/ui/OptionWidget.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/PreviewWidget.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/SearchDelegate.cpp \
    src/ui/SearchPropertiesDialog.cpp \
    src/ui/SearchWidget.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/StatusBarWidget.cpp \
    src/ui/TabBarDockWidget.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/TextLabelWidget.cpp \
    src/ui/WebWidget.cpp \
    src/ui/Window.cpp \
    src/ui/preferences/BlockedContentDialog.cpp \
    src/ui/preferences/ShortcutDelegate.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebPage.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp \
    src/modules/windows/bookmarks/BookmarksContentsWidget.cpp \
    src/modules/windows/cache/CacheContentsWidget.cpp \
    src/modules/windows/configuration/ConfigurationContentsWidget.cpp \
    src/modules/windows/cookies/CookiesContentsWidget.cpp \
    src/modules/windows/history/HistoryContentsWidget.cpp \
    src/modules/windows/transfers/ProgressBarDelegate.cpp \
    src/modules/windows/transfers/TransfersContentsWidget.cpp \
    src/modules/windows/web/ImagePropertiesDialog.cpp \
    src/modules/windows/web/ProgressBarWidget.cpp \
    src/modules/windows/web/WebContentsWidget.cpp

HEADERS += src/core/ActionsManager.h \
    src/core/AddressCompletionModel.h \
    src/core/Application.h \
    src/core/BookmarksManager.h \
    src/core/CookieJar.h \
    src/core/FileSystemCompleterModel.h \
    src/core/HistoryManager.h \
    src/core/LocalListingNetworkReply.h \
    src/core/NetworkAccessManager.h \
    src/core/NetworkCache.h \
    src/core/SearchesManager.h \
    src/core/SearchSuggester.h \
    src/core/SessionsManager.h \
    src/core/SettingsManager.h \
    src/core/TransfersManager.h \
    src/core/Utils.h \
    src/core/WebBackend.h \
    src/core/WebBackendsManager.h \
    src/core/WindowsManager.h \
    src/ui/AddressWidget.h \
    src/ui/AuthenticationDialog.h \
    src/ui/BookmarkPropertiesDialog.h \
    src/ui/BookmarksImportDialog.h \
    src/ui/ClearHistoryDialog.h \
    src/ui/ContentsDialog.h \
    src/ui/ContentsWidget.h \
    src/ui/ItemDelegate.h \
    src/ui/MainWindow.h \
    src/ui/OptionDelegate.h \
    src/ui/OptionWidget.h \
    src/ui/PreferencesDialog.h \
    src/ui/PreviewWidget.h \
    src/ui/SaveSessionDialog.h \
    src/ui/SearchDelegate.h \
    src/ui/SearchPropertiesDialog.h \
    src/ui/SearchWidget.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/StatusBarWidget.h \
    src/ui/TabBarDockWidget.h \
    src/ui/TabBarWidget.h \
    src/ui/TextLabelWidget.h \
    src/ui/WebWidget.h \
    src/ui/Window.h \
    src/ui/preferences/BlockedContentDialog.h \
    src/ui/preferences/ShortcutDelegate.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebPage.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.h \
    src/modules/windows/bookmarks/BookmarksContentsWidget.h \
    src/modules/windows/cache/CacheContentsWidget.h \
    src/modules/windows/configuration/ConfigurationContentsWidget.h \
    src/modules/windows/cookies/CookiesContentsWidget.h \
    src/modules/windows/history/HistoryContentsWidget.h \
    src/modules/windows/transfers/ProgressBarDelegate.h \
    src/modules/windows/transfers/TransfersContentsWidget.h \
    src/modules/windows/web/ImagePropertiesDialog.h \
    src/modules/windows/web/ProgressBarWidget.h \
    src/modules/windows/web/WebContentsWidget.h

FORMS += src/ui/AuthenticationDialog.ui \
    src/ui/BookmarkPropertiesDialog.ui \
    src/ui/BookmarksImportDialog.ui \
    src/ui/ClearHistoryDialog.ui \
    src/ui/MainWindow.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/SearchPropertiesDialog.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/Window.ui \
    src/ui/preferences/BlockedContentDialog.ui \
    src/modules/windows/bookmarks/BookmarksContentsWidget.ui \
    src/modules/windows/cache/CacheContentsWidget.ui \
    src/modules/windows/configuration/ConfigurationContentsWidget.ui \
    src/modules/windows/cookies/CookiesContentsWidget.ui \
    src/modules/windows/history/HistoryContentsWidget.ui \
    src/modules/windows/transfers/TransfersContentsWidget.ui \
    src/modules/windows/web/ImagePropertiesDialog.ui \
    src/modules/windows/web/WebContentsWidget.ui

RESOURCES += resources/resources.qrc
