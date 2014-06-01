#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

!greaterThan(QT_MAJOR_VERSION, 4) | !greaterThan(QT_MINOR_VERSION, 1) {
    error("Qt 5.2.0 or newer is required.")
}

QT += core gui network printsupport script sql webkitwidgets widgets

win32 {
    QT += winextras
}

TARGET = otter-browser
TEMPLATE = app

SOURCES += src/main.cpp \
    src/core/ActionsManager.cpp \
    src/core/AddressCompletionModel.cpp \
    src/core/Application.cpp \
    src/core/BookmarksManager.cpp \
    src/core/Console.cpp \
    src/core/CookieJar.cpp \
    src/core/FileSystemCompleterModel.cpp \
    src/core/HistoryManager.cpp \
    src/core/Importer.cpp \
    src/core/LocalListingNetworkReply.cpp \
    src/core/NetworkManager.cpp \
    src/core/NetworkManagerFactory.cpp \
    src/core/NetworkAutomaticProxy.cpp \
    src/core/NetworkCache.cpp \
    src/core/NetworkProxyFactory.cpp \
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
    src/ui/ClearHistoryDialog.cpp \
    src/ui/ConsoleWidget.cpp \
    src/ui/ContentsDialog.cpp \
    src/ui/ContentsWidget.cpp \
    src/ui/FilePathWidget.cpp \
    src/ui/ItemDelegate.cpp \
    src/ui/MainWindow.cpp \
    src/ui/MdiWidget.cpp \
    src/ui/OptionDelegate.cpp \
    src/ui/OptionWidget.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/PreviewWidget.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/SearchDelegate.cpp \
    src/ui/SearchPropertiesDialog.cpp \
    src/ui/SearchWidget.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/StartupDialog.cpp \
    src/ui/StatusBarWidget.cpp \
    src/ui/TabBarDockWidget.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/TableViewWidget.cpp \
    src/ui/TextLabelWidget.cpp \
    src/ui/WebWidget.cpp \
    src/ui/UserAgentsManagerDialog.cpp \
    src/ui/Window.cpp \
    src/ui/preferences/BlockedContentDialog.cpp \
    src/ui/preferences/KeyboardShortcutDelegate.cpp \
    src/ui/preferences/SearchShortcutDelegate.cpp \
    src/ui/preferences/ShortcutsProfileDialog.cpp \
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
    src/core/Console.h \
    src/core/CookieJar.h \
    src/core/FileSystemCompleterModel.h \
    src/core/HistoryManager.h \
    src/core/Importer.h \
    src/core/LocalListingNetworkReply.h \
    src/core/NetworkAutomaticProxy.h \
    src/core/NetworkCache.h \
    src/core/NetworkManager.h \
    src/core/NetworkManagerFactory.h \
    src/core/NetworkProxyFactory.h \
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
    src/ui/ClearHistoryDialog.h \
    src/ui/ConsoleWidget.h \
    src/ui/ContentsDialog.h \
    src/ui/ContentsWidget.h \
    src/ui/FilePathWidget.h \
    src/ui/ItemDelegate.h \
    src/ui/MainWindow.h \
    src/ui/MdiWidget.h \
    src/ui/OptionDelegate.h \
    src/ui/OptionWidget.h \
    src/ui/PreferencesDialog.h \
    src/ui/PreviewWidget.h \
    src/ui/SaveSessionDialog.h \
    src/ui/SearchDelegate.h \
    src/ui/SearchPropertiesDialog.h \
    src/ui/SearchWidget.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/StartupDialog.h \
    src/ui/StatusBarWidget.h \
    src/ui/TabBarDockWidget.h \
    src/ui/TabBarWidget.h \
    src/ui/TableViewWidget.h \
    src/ui/TextLabelWidget.h \
    src/ui/UserAgentsManagerDialog.h \
    src/ui/WebWidget.h \
    src/ui/Window.h \
    src/ui/preferences/BlockedContentDialog.h \
    src/ui/preferences/KeyboardShortcutDelegate.h \
    src/ui/preferences/SearchShortcutDelegate.h \
    src/ui/preferences/ShortcutsProfileDialog.h \
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
    src/ui/ClearHistoryDialog.ui \
    src/ui/ConsoleWidget.ui \
    src/ui/MainWindow.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/SearchPropertiesDialog.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/StartupDialog.ui \
    src/ui/UserAgentsManagerDialog.ui \
    src/ui/Window.ui \
    src/ui/preferences/BlockedContentDialog.ui \
    src/ui/preferences/ShortcutsProfileDialog.ui \
    src/modules/windows/bookmarks/BookmarksContentsWidget.ui \
    src/modules/windows/cache/CacheContentsWidget.ui \
    src/modules/windows/configuration/ConfigurationContentsWidget.ui \
    src/modules/windows/cookies/CookiesContentsWidget.ui \
    src/modules/windows/history/HistoryContentsWidget.ui \
    src/modules/windows/transfers/TransfersContentsWidget.ui \
    src/modules/windows/web/ImagePropertiesDialog.ui \
    src/modules/windows/web/WebContentsWidget.ui

RESOURCES += resources/resources.qrc

TRANSLATIONS += resources/translations/cs_CZ.ts \
    resources/translations/de_DE.ts \
    resources/translations/en_GB.ts \
    resources/translations/en_US.ts \
    resources/translations/es_ES.ts \
    resources/translations/et.ts \
    resources/translations/fr_FR.ts \
    resources/translations/id_ID.ts \
    resources/translations/it_IT.ts \
    resources/translations/lt.ts \
    resources/translations/nl.ts \
    resources/translations/pl_PL.ts \
    resources/translations/pt_BR.ts \
    resources/translations/pt_PT.ts \
    resources/translations/ro.ts \
    resources/translations/ru_RU.ts \
    resources/translations/sr.ts \
    resources/translations/sr@Ijekavian.ts \
    resources/translations/sr@ijekavianlatin.ts \
    resources/translations/sr@latin.ts \
    resources/translations/tr_TR.ts \
    resources/translations/uk_UA.ts

RC_FILE = otter-browser.rc
