#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

!greaterThan(QT_MAJOR_VERSION, 4) | !greaterThan(QT_MINOR_VERSION, 1) {
    error("Qt 5.2.0 or newer is required.")
}

QT += core gui network printsupport script sql webkitwidgets widgets

win32: QT += winextras
win32: LIBS += -lOle32 -lshell32
win32: INCLUDEPATH += .\
unix: INCLUDEPATH += ./

OTTER_VERSION_MAIN = 0.9.04
OTTER_VERSION_CONTEXT = -dev

isEmpty(PREFIX): PREFIX = /usr/local

TARGET = otter-browser
TARGET.path = $$PREFIX/

TEMPLATE = app

configHeader.input = config.h.qmake
configHeader.output = config.h
QMAKE_SUBSTITUTES += configHeader

SOURCES += src/main.cpp \
    src/core/Action.cpp \
    src/core/ActionsManager.cpp \
    src/core/AddressCompletionModel.cpp \
    src/core/Application.cpp \
    src/core/BookmarksImporter.cpp \
    src/core/BookmarksManager.cpp \
    src/core/BookmarksModel.cpp \
    src/core/ContentBlockingList.cpp \
    src/core/ContentBlockingManager.cpp \
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
    src/core/PlatformIntegration.cpp \
    src/core/SearchesManager.cpp \
    src/core/SearchSuggester.cpp \
    src/core/SessionsManager.cpp \
    src/core/SettingsManager.cpp \
    src/core/ShortcutsManager.cpp \
    src/core/TransfersManager.cpp \
    src/core/Utils.cpp \
    src/core/WebBackend.cpp \
    src/core/WebBackendsManager.cpp \
    src/core/WindowsManager.cpp \
    src/ui/ActionWidget.cpp \
    src/ui/AddressWidget.cpp \
    src/ui/AuthenticationDialog.cpp \
    src/ui/BookmarkPropertiesDialog.cpp \
    src/ui/BookmarksComboBoxWidget.cpp \
    src/ui/BookmarksImporterWidget.cpp \
    src/ui/ClearHistoryDialog.cpp \
    src/ui/ConsoleWidget.cpp \
    src/ui/ContentsDialog.cpp \
    src/ui/ContentsWidget.cpp \
    src/ui/FilePathWidget.cpp \
    src/ui/GoBackActionWidget.cpp \
    src/ui/GoForwardActionWidget.cpp \
    src/ui/ImportDialog.cpp \
    src/ui/ItemDelegate.cpp \
    src/ui/LocaleDialog.cpp \
    src/ui/MainWindow.cpp \
    src/ui/MdiWidget.cpp \
    src/ui/Menu.cpp \
    src/ui/OpenAddressDialog.cpp \
    src/ui/OpenBookmarkDialog.cpp \
    src/ui/OptionDelegate.cpp \
    src/ui/OptionWidget.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/PreviewWidget.cpp \
    src/ui/ReloadTimeDialog.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/SearchDelegate.cpp \
    src/ui/SearchPropertiesDialog.cpp \
    src/ui/SearchWidget.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/SidebarWidget.cpp \
    src/ui/StartupDialog.cpp \
    src/ui/StatusBarWidget.cpp \
    src/ui/TabBarDockWidget.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/TableViewWidget.cpp \
    src/ui/TextLabelWidget.cpp \
    src/ui/TrayIcon.cpp \
    src/ui/UserAgentsManagerDialog.cpp \
    src/ui/WebsitePreferencesDialog.cpp \
    src/ui/WebWidget.cpp \
    src/ui/Window.cpp \
    src/ui/preferences/AcceptLanguageDialog.cpp \
    src/ui/preferences/ContentBlockingDialog.cpp \
    src/ui/preferences/JavaScriptPreferencesDialog.cpp \
    src/ui/preferences/KeyboardShortcutDelegate.cpp \
    src/ui/preferences/SearchKeywordDelegate.cpp \
    src/ui/preferences/ShortcutsProfileDialog.cpp \
    src/ui/toolbars/ZoomWidget.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitPluginWidget.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebPage.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp \
    src/modules/importers/html/HtmlBookmarksImporter.cpp \
    src/modules/importers/opera/OperaBookmarksImporter.cpp \
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

win32: SOURCES += src/modules/platforms/windows/WindowsPlatformIntegration.cpp

HEADERS += src/core/Action.h \
    src/core/ActionsManager.h \
    src/core/AddressCompletionModel.h \
    src/core/Application.h \
    src/core/BookmarksImporter.h \
    src/core/BookmarksManager.h \
    src/core/BookmarksModel.h \
    src/core/ContentBlockingList.h \
    src/core/ContentBlockingManager.h \
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
    src/core/PlatformIntegration.h \
    src/core/SearchesManager.h \
    src/core/SearchSuggester.h \
    src/core/SessionsManager.h \
    src/core/SettingsManager.h \
    src/core/ShortcutsManager.h \
    src/core/TransfersManager.h \
    src/core/Utils.h \
    src/core/WebBackend.h \
    src/core/WebBackendsManager.h \
    src/core/WindowsManager.h \
    src/ui/ActionWidget.h \
    src/ui/AddressWidget.h \
    src/ui/AuthenticationDialog.h \
    src/ui/BookmarkPropertiesDialog.h \
    src/ui/BookmarksComboBoxWidget.h \
    src/ui/BookmarksImporterWidget.h \
    src/ui/ClearHistoryDialog.h \
    src/ui/ConsoleWidget.h \
    src/ui/ContentsDialog.h \
    src/ui/ContentsWidget.h \
    src/ui/FilePathWidget.h \
    src/ui/GoBackActionWidget.h \
    src/ui/GoForwardActionWidget.h \
    src/ui/ImportDialog.h \
    src/ui/ItemDelegate.h \
    src/ui/LocaleDialog.h \
    src/ui/MainWindow.h \
    src/ui/MdiWidget.h \
    src/ui/Menu.h \
    src/ui/OpenAddressDialog.h \
    src/ui/OpenBookmarkDialog.h \
    src/ui/OptionDelegate.h \
    src/ui/OptionWidget.h \
    src/ui/PreferencesDialog.h \
    src/ui/PreviewWidget.h \
    src/ui/ReloadTimeDialog.h \
    src/ui/SaveSessionDialog.h \
    src/ui/SearchDelegate.h \
    src/ui/SearchPropertiesDialog.h \
    src/ui/SearchWidget.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/SidebarWidget.h \
    src/ui/StartupDialog.h \
    src/ui/StatusBarWidget.h \
    src/ui/TabBarDockWidget.h \
    src/ui/TabBarWidget.h \
    src/ui/TableViewWidget.h \
    src/ui/TextLabelWidget.h \
    src/ui/TrayIcon.h \
    src/ui/UserAgentsManagerDialog.h \
    src/ui/WebsitePreferencesDialog.h \
    src/ui/WebWidget.h \
    src/ui/Window.h \
    src/ui/preferences/AcceptLanguageDialog.h \
    src/ui/preferences/ContentBlockingDialog.h \
    src/ui/preferences/JavaScriptPreferencesDialog.h \
    src/ui/preferences/KeyboardShortcutDelegate.h \
    src/ui/preferences/SearchKeywordDelegate.h \
    src/ui/preferences/ShortcutsProfileDialog.h \
    src/ui/toolbars/ZoomWidget.h \
    src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.h \
    src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.h \
    src/modules/backends/web/qtwebkit/QtWebKitPluginWidget.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebPage.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.h \
    src/modules/importers/html/HtmlBookmarksImporter.h \
    src/modules/importers/opera/OperaBookmarksImporter.h \
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

win32: HEADERS += src/modules/platforms/windows/WindowsPlatformIntegration.h

FORMS += src/ui/AuthenticationDialog.ui \
    src/ui/BookmarkPropertiesDialog.ui \
    src/ui/BookmarksImporterWidget.ui \
    src/ui/ClearHistoryDialog.ui \
    src/ui/ConsoleWidget.ui \
    src/ui/ImportDialog.ui \
    src/ui/LocaleDialog.ui \
    src/ui/MainWindow.ui \
    src/ui/OpenAddressDialog.ui \
    src/ui/OpenBookmarkDialog.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/ReloadTimeDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/SearchPropertiesDialog.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/SidebarWidget.ui \
    src/ui/StartupDialog.ui \
    src/ui/UserAgentsManagerDialog.ui \
    src/ui/WebsitePreferencesDialog.ui \
    src/ui/preferences/AcceptLanguageDialog.ui \
    src/ui/preferences/ContentBlockingDialog.ui \
    src/ui/preferences/JavaScriptPreferencesDialog.ui \
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

TRANSLATIONS += resources/translations/otter-browser_cs_CZ.ts \
    resources/translations/otter-browser_de_DE.ts \
    resources/translations/otter-browser_el.ts \
    resources/translations/otter-browser_en_GB.ts \
    resources/translations/otter-browser_en_US.ts \
    resources/translations/otter-browser_es_ES.ts \
    resources/translations/otter-browser_es_MX.ts \
    resources/translations/otter-browser_et.ts \
    resources/translations/otter-browser_fi.ts \
    resources/translations/otter-browser_fr_FR.ts \
    resources/translations/otter-browser_hu.ts \
    resources/translations/otter-browser_id_ID.ts \
    resources/translations/otter-browser_it_IT.ts \
    resources/translations/otter-browser_ja_JP.ts \
    resources/translations/otter-browser_ka_GE.ts \
    resources/translations/otter-browser_lt.ts \
    resources/translations/otter-browser_nb_NO.ts \
    resources/translations/otter-browser_nl.ts \
    resources/translations/otter-browser_pl_PL.ts \
    resources/translations/otter-browser_pt_BR.ts \
    resources/translations/otter-browser_pt_PT.ts \
    resources/translations/otter-browser_ro.ts \
    resources/translations/otter-browser_ru_RU.ts \
    resources/translations/otter-browser_sk.ts \
    resources/translations/otter-browser_sl_SI.ts \
    resources/translations/otter-browser_sr.ts \
    resources/translations/otter-browser_sr@Ijekavian.ts \
    resources/translations/otter-browser_sr@ijekavianlatin.ts \
    resources/translations/otter-browser_sr@latin.ts \
    resources/translations/otter-browser_tr_TR.ts \
    resources/translations/otter-browser_uk_UA.ts \
    resources/translations/otter-browser_zh_CN.ts \
    resources/translations/otter-browser_zh_TW.ts

RC_FILE = otter-browser.rc
