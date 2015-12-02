#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

OTTER_VERSION_MAIN = 0.9.09
OTTER_VERSION_WEEKLY = ""
OTTER_VERSION_CONTEXT = -dev

message("otter.pro is deprecated, use CMake instead.")

!greaterThan(QT_MAJOR_VERSION, 4) | !greaterThan(QT_MINOR_VERSION, 1) {
    error("Qt 5.2.0 or newer is required.")
}

QT += core gui multimedia network printsupport script sql webkitwidgets widgets xmlpatterns

configHeader.input = config.h.qmake
configHeader.output = config.h
QMAKE_SUBSTITUTES += configHeader

win32: {
    QT += winextras

    LIBS += -lOle32 -lshell32 -ladvapi32 -luser32

    INCLUDEPATH += .\

    SOURCES += src/modules/platforms/windows/WindowsPlatformIntegration.cpp

    HEADERS += src/modules/platforms/windows/WindowsPlatformIntegration.h
}

macx: {
    LIBS += -framework Cocoa -framework Foundation

    OBJECTIVE_SOURCES += src/modules/platforms/mac/MacPlatformIntegration.mm

    HEADERS += src/modules/platforms/mac/MacPlatformIntegration.h
}

unix: {
    QT += dbus

    INCLUDEPATH += ./

   !macx {
        SOURCES += src/modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.cpp \
        3rdparty/libmimeapps/ConfigReader.cpp \
        3rdparty/libmimeapps/DesktopEntry.cpp \
        3rdparty/libmimeapps/Index.cpp \
        3rdparty/libmimeapps/Tools.cpp

        HEADERS += src/modules/platforms/freedesktoporg/FreeDesktopOrgPlatformIntegration.h \
        3rdparty/libmimeapps/ConfigReader.h \
        3rdparty/libmimeapps/DesktopEntry.h \
        3rdparty/libmimeapps/Index.h \
        3rdparty/libmimeapps/Tools.h
    }
}

isEmpty(PREFIX): PREFIX = /usr/local

TARGET = otter-browser
TARGET.path = $$PREFIX/

TEMPLATE = app

SOURCES += src/main.cpp \
    src/core/ActionsManager.cpp \
    src/core/Addon.cpp \
    src/core/AddonsManager.cpp \
    src/core/AddressCompletionModel.cpp \
    src/core/Application.cpp \
    src/core/BookmarksImporter.cpp \
    src/core/BookmarksManager.cpp \
    src/core/BookmarksModel.cpp \
    src/core/ContentBlockingManager.cpp \
    src/core/ContentBlockingProfile.cpp \
    src/core/Console.cpp \
    src/core/CookieJar.cpp \
    src/core/CookieJarProxy.cpp \
    src/core/FileSystemCompleterModel.cpp \
    src/core/GesturesManager.cpp \
    src/core/HandlersManager.cpp \
    src/core/HistoryManager.cpp \
    src/core/HistoryModel.cpp \
    src/core/Importer.cpp \
    src/core/InputInterpreter.cpp \
    src/core/LocalListingNetworkReply.cpp \
    src/core/LongTermTimer.cpp \
    src/core/NetworkManager.cpp \
    src/core/NetworkManagerFactory.cpp \
    src/core/NetworkAutomaticProxy.cpp \
    src/core/NetworkCache.cpp \
    src/core/NetworkProxyFactory.cpp \
    src/core/NotesManager.cpp \
    src/core/NotificationsManager.cpp \
    src/core/PasswordsManager.cpp \
    src/core/PlatformIntegration.cpp \
    src/core/SearchEnginesManager.cpp \
    src/core/SearchSuggester.cpp \
    src/core/SessionModel.cpp \
    src/core/SessionsManager.cpp \
    src/core/Settings.cpp \
    src/core/SettingsManager.cpp \
    src/core/ToolBarsManager.cpp \
    src/core/Transfer.cpp \
    src/core/TransfersManager.cpp \
    src/core/UpdateChecker.cpp \
    src/core/Updater.cpp \
    src/core/Utils.cpp \
    src/core/WebBackend.cpp \
    src/core/WindowsManager.cpp \
    src/ui/AcceptCookieDialog.cpp \
    src/ui/ActionComboBoxWidget.cpp \
    src/ui/AddressDelegate.cpp \
    src/ui/ApplicationComboBoxWidget.cpp \
    src/ui/AuthenticationDialog.cpp \
    src/ui/BookmarkPropertiesDialog.cpp \
    src/ui/BookmarksBarDialog.cpp \
    src/ui/BookmarksComboBoxWidget.cpp \
    src/ui/BookmarksImporterWidget.cpp \
    src/ui/CertificateDialog.cpp \
    src/ui/ClearHistoryDialog.cpp \
    src/ui/ConsoleWidget.cpp \
    src/ui/ContentsDialog.cpp \
    src/ui/ContentsWidget.cpp \
    src/ui/Dialog.cpp \
    src/ui/FilePathWidget.cpp \
    src/ui/ImagePropertiesDialog.cpp \
    src/ui/ImportDialog.cpp \
    src/ui/ItemDelegate.cpp \
    src/ui/ItemViewWidget.cpp \
    src/ui/LineEditWidget.cpp \
    src/ui/LocaleDialog.cpp \
    src/ui/MainWindow.cpp \
    src/ui/Menu.cpp \
    src/ui/MenuBarWidget.cpp \
    src/ui/NotificationDialog.cpp \
    src/ui/OpenAddressDialog.cpp \
    src/ui/OpenBookmarkDialog.cpp \
    src/ui/OptionDelegate.cpp \
    src/ui/OptionWidget.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/PreviewWidget.cpp \
    src/ui/ReloadTimeDialog.cpp \
    src/ui/ReportDialog.cpp \
    src/ui/SaveSessionDialog.cpp \
    src/ui/SearchDelegate.cpp \
    src/ui/SearchEnginePropertiesDialog.cpp \
    src/ui/SessionsManagerDialog.cpp \
    src/ui/SidebarWidget.cpp \
    src/ui/SourceViewerWebWidget.cpp \
    src/ui/SourceViewerWidget.cpp \
    src/ui/StartupDialog.cpp \
    src/ui/StatusBarWidget.cpp \
    src/ui/TabBarStyle.cpp \
    src/ui/TabBarWidget.cpp \
    src/ui/TabSwitcherWidget.cpp \
    src/ui/TextLabelWidget.cpp \
    src/ui/ToolBarAreaWidget.cpp \
    src/ui/ToolBarDialog.cpp \
    src/ui/ToolBarWidget.cpp \
    src/ui/ToolButtonWidget.cpp \
    src/ui/TransferDialog.cpp \
    src/ui/TrayIcon.cpp \
    src/ui/UpdateCheckerDialog.cpp \
    src/ui/UserAgentsManagerDialog.cpp \
    src/ui/WebsiteInformationDialog.cpp \
    src/ui/WebsitePreferencesDialog.cpp \
    src/ui/WebWidget.cpp \
    src/ui/Window.cpp \
    src/ui/WorkspaceWidget.cpp \
    src/ui/preferences/AcceptLanguageDialog.cpp \
    src/ui/preferences/ActionDelegate.cpp \
    src/ui/preferences/ContentBlockingDialog.cpp \
    src/ui/preferences/ContentBlockingIntervalDelegate.cpp \
    src/ui/preferences/JavaScriptPreferencesDialog.cpp \
    src/ui/preferences/KeyboardProfileDialog.cpp \
    src/ui/preferences/KeyboardShortcutDelegate.cpp \
    src/ui/preferences/MouseProfileDialog.cpp \
    src/ui/preferences/PreferencesAdvancedPageWidget.cpp \
    src/ui/preferences/PreferencesContentPageWidget.cpp \
    src/ui/preferences/PreferencesGeneralPageWidget.cpp \
    src/ui/preferences/PreferencesPrivacyPageWidget.cpp \
    src/ui/preferences/PreferencesSearchPageWidget.cpp \
    src/ui/preferences/SearchKeywordDelegate.cpp \
    src/ui/toolbars/ActionWidget.cpp \
    src/ui/toolbars/AddressWidget.cpp \
    src/ui/toolbars/BookmarkWidget.cpp \
    src/ui/toolbars/GoBackActionWidget.cpp \
    src/ui/toolbars/GoForwardActionWidget.cpp \
    src/ui/toolbars/MenuButtonWidget.cpp \
    src/ui/toolbars/PanelChooserWidget.cpp \
    src/ui/toolbars/SearchWidget.cpp \
    src/ui/toolbars/StatusMessageWidget.cpp \
    src/ui/toolbars/ZoomWidget.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitFtpListingNetworkReply.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitHistoryInterface.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitInspector.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitPage.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitPluginWidget.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.cpp \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.cpp \
    src/modules/backends/web/qtwebkit/3rdparty/qtftp/qftp.cpp \
    src/modules/backends/web/qtwebkit/3rdparty/qtftp/qurlinfo.cpp \
    src/modules/importers/html/HtmlBookmarksImporter.cpp \
    src/modules/importers/opera/OperaBookmarksImporter.cpp \
    src/modules/importers/opera/OperaNotesImporter.cpp \
    src/modules/windows/bookmarks/BookmarksContentsWidget.cpp \
    src/modules/windows/cache/CacheContentsWidget.cpp \
    src/modules/windows/configuration/ConfigurationContentsWidget.cpp \
    src/modules/windows/cookies/CookiesContentsWidget.cpp \
    src/modules/windows/history/HistoryContentsWidget.cpp \
    src/modules/windows/notes/NotesContentsWidget.cpp \
    src/modules/windows/transfers/ProgressBarDelegate.cpp \
    src/modules/windows/transfers/TransfersContentsWidget.cpp \
    src/modules/windows/web/PasswordBarWidget.cpp \
    src/modules/windows/web/PermissionBarWidget.cpp \
    src/modules/windows/web/PopupsBarWidget.cpp \
    src/modules/windows/web/ProgressBarWidget.cpp \
    src/modules/windows/web/SearchBarWidget.cpp \
    src/modules/windows/web/StartPageModel.cpp \
    src/modules/windows/web/StartPagePreferencesDialog.cpp \
    src/modules/windows/web/StartPageWidget.cpp \
    src/modules/windows/web/TileDelegate.cpp \
    src/modules/windows/web/WebContentsWidget.cpp \
    3rdparty/mousegestures/MouseGestures.cpp

HEADERS += src/core/ActionsManager.h \
    src/core/Addon.h \
    src/core/AddonsManager.h \
    src/core/AddressCompletionModel.h \
    src/core/Application.h \
    src/core/BookmarksImporter.h \
    src/core/BookmarksManager.h \
    src/core/BookmarksModel.h \
    src/core/ContentBlockingManager.h \
    src/core/ContentBlockingProfile.h \
    src/core/Console.h \
    src/core/CookieJar.h \
    src/core/CookieJarProxy.h \
    src/core/FileSystemCompleterModel.h \
    src/core/GesturesManager.h \
    src/core/HandlersManager.h \
    src/core/HistoryManager.h \
    src/core/HistoryModel.h \
    src/core/Importer.h \
    src/core/InputInterpreter.h \
    src/core/LocalListingNetworkReply.h \
    src/core/LongTermTimer.h \
    src/core/NetworkAutomaticProxy.h \
    src/core/NetworkCache.h \
    src/core/NetworkManager.h \
    src/core/NetworkManagerFactory.h \
    src/core/NetworkProxyFactory.h \
    src/core/NotesManager.h \
    src/core/NotificationsManager.h \
    src/core/PasswordsManager.h \
    src/core/PlatformIntegration.h \
    src/core/SearchEnginesManager.h \
    src/core/SearchSuggester.h \
    src/core/SessionModel.h \
    src/core/SessionsManager.h \
    src/core/Settings.h \
    src/core/SettingsManager.h \
    src/core/ToolBarsManager.h \
    src/core/Transfer.h \
    src/core/TransfersManager.h \
    src/core/UpdateChecker.h \
    src/core/Updater.h \
    src/core/Utils.h \
    src/core/WebBackend.h \
    src/core/WindowsManager.h \
    src/ui/AcceptCookieDialog.h \
    src/ui/ActionComboBoxWidget.h \
    src/ui/AddressDelegate.h \
    src/ui/ApplicationComboBoxWidget.h \
    src/ui/AuthenticationDialog.h \
    src/ui/BookmarkPropertiesDialog.h \
    src/ui/BookmarksBarDialog.h \
    src/ui/BookmarksComboBoxWidget.h \
    src/ui/BookmarksImporterWidget.h \
    src/ui/CertificateDialog.h \
    src/ui/ClearHistoryDialog.h \
    src/ui/ConsoleWidget.h \
    src/ui/ContentsDialog.h \
    src/ui/ContentsWidget.h \
    src/ui/Dialog.h \
    src/ui/FilePathWidget.h \
    src/ui/ImagePropertiesDialog.h \
    src/ui/ImportDialog.h \
    src/ui/ItemDelegate.h \
    src/ui/ItemViewWidget.h \
    src/ui/LineEditWidget.h \
    src/ui/LocaleDialog.h \
    src/ui/MainWindow.h \
    src/ui/Menu.h \
    src/ui/MenuBarWidget.h \
    src/ui/NotificationDialog.h \
    src/ui/OpenAddressDialog.h \
    src/ui/OpenBookmarkDialog.h \
    src/ui/OptionDelegate.h \
    src/ui/OptionWidget.h \
    src/ui/PreferencesDialog.h \
    src/ui/PreviewWidget.h \
    src/ui/ReloadTimeDialog.h \
    src/ui/ReportDialog.h \
    src/ui/SaveSessionDialog.h \
    src/ui/SearchDelegate.h \
    src/ui/SearchEnginePropertiesDialog.h \
    src/ui/SessionsManagerDialog.h \
    src/ui/SidebarWidget.h \
    src/ui/SourceViewerWebWidget.h \
    src/ui/SourceViewerWidget.h \
    src/ui/StartupDialog.h \
    src/ui/StatusBarWidget.h \
    src/ui/TabBarStyle.h \
    src/ui/TabBarWidget.h \
    src/ui/TabSwitcherWidget.h \
    src/ui/TextLabelWidget.h \
    src/ui/ToolBarAreaWidget.h \
    src/ui/ToolBarDialog.h \
    src/ui/ToolBarWidget.h \
    src/ui/ToolButtonWidget.h \
    src/ui/TransferDialog.h \
    src/ui/TrayIcon.h \
    src/ui/UpdateCheckerDialog.h \
    src/ui/UserAgentsManagerDialog.h \
    src/ui/WebsiteInformationDialog.h \
    src/ui/WebsitePreferencesDialog.h \
    src/ui/WebWidget.h \
    src/ui/Window.h \
    src/ui/WorkspaceWidget.h \
    src/ui/preferences/AcceptLanguageDialog.h \
    src/ui/preferences/ActionDelegate.h \
    src/ui/preferences/ContentBlockingDialog.h \
    src/ui/preferences/ContentBlockingIntervalDelegate.h \
    src/ui/preferences/JavaScriptPreferencesDialog.h \
    src/ui/preferences/KeyboardProfileDialog.h \
    src/ui/preferences/KeyboardShortcutDelegate.h \
    src/ui/preferences/MouseProfileDialog.h \
    src/ui/preferences/PreferencesAdvancedPageWidget.h \
    src/ui/preferences/PreferencesContentPageWidget.h \
    src/ui/preferences/PreferencesGeneralPageWidget.h \
    src/ui/preferences/PreferencesPrivacyPageWidget.h \
    src/ui/preferences/PreferencesSearchPageWidget.h \
    src/ui/preferences/SearchKeywordDelegate.h \
    src/ui/toolbars/ActionWidget.h \
    src/ui/toolbars/AddressWidget.h \
    src/ui/toolbars/BookmarkWidget.h \
    src/ui/toolbars/GoBackActionWidget.h \
    src/ui/toolbars/GoForwardActionWidget.h \
    src/ui/toolbars/MenuButtonWidget.h \
    src/ui/toolbars/PanelChooserWidget.h \
    src/ui/toolbars/SearchWidget.h \
    src/ui/toolbars/StatusMessageWidget.h \
    src/ui/toolbars/ZoomWidget.h \
    src/modules/backends/web/qtwebkit/QtWebKitFtpListingNetworkReply.h \
    src/modules/backends/web/qtwebkit/QtWebKitHistoryInterface.h \
    src/modules/backends/web/qtwebkit/QtWebKitInspector.h \
    src/modules/backends/web/qtwebkit/QtWebKitNetworkManager.h \
    src/modules/backends/web/qtwebkit/QtWebKitPage.h \
    src/modules/backends/web/qtwebkit/QtWebKitPluginFactory.h \
    src/modules/backends/web/qtwebkit/QtWebKitPluginWidget.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebBackend.h \
    src/modules/backends/web/qtwebkit/QtWebKitWebWidget.h \
    src/modules/backends/web/qtwebkit/3rdparty/qtftp/qftp.h \
    src/modules/backends/web/qtwebkit/3rdparty/qtftp/qurlinfo.h \
    src/modules/importers/html/HtmlBookmarksImporter.h \
    src/modules/importers/opera/OperaBookmarksImporter.h \
    src/modules/importers/opera/OperaNotesImporter.h \
    src/modules/windows/bookmarks/BookmarksContentsWidget.h \
    src/modules/windows/cache/CacheContentsWidget.h \
    src/modules/windows/configuration/ConfigurationContentsWidget.h \
    src/modules/windows/cookies/CookiesContentsWidget.h \
    src/modules/windows/history/HistoryContentsWidget.h \
    src/modules/windows/notes/NotesContentsWidget.h \
    src/modules/windows/transfers/ProgressBarDelegate.h \
    src/modules/windows/transfers/TransfersContentsWidget.h \
    src/modules/windows/web/PasswordBarWidget.h \
    src/modules/windows/web/PermissionBarWidget.h \
    src/modules/windows/web/PopupsBarWidget.h \
    src/modules/windows/web/ProgressBarWidget.h \
    src/modules/windows/web/SearchBarWidget.h \
    src/modules/windows/web/StartPageModel.h \
    src/modules/windows/web/StartPagePreferencesDialog.h \
    src/modules/windows/web/StartPageWidget.h \
    src/modules/windows/web/TileDelegate.h \
    src/modules/windows/web/WebContentsWidget.h \
    3rdparty/mousegestures/MouseGestures.h

FORMS += src/ui/AcceptCookieDialog.ui \
    src/ui/AuthenticationDialog.ui \
    src/ui/BookmarkPropertiesDialog.ui \
    src/ui/BookmarksBarDialog.ui \
    src/ui/BookmarksImporterWidget.ui \
    src/ui/CertificateDialog.ui \
    src/ui/ClearHistoryDialog.ui \
    src/ui/ConsoleWidget.ui \
    src/ui/ImagePropertiesDialog.ui \
    src/ui/ImportDialog.ui \
    src/ui/LocaleDialog.ui \
    src/ui/MainWindow.ui \
    src/ui/OpenAddressDialog.ui \
    src/ui/OpenBookmarkDialog.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/ReloadTimeDialog.ui \
    src/ui/ReportDialog.ui \
    src/ui/SaveSessionDialog.ui \
    src/ui/SearchEnginePropertiesDialog.ui \
    src/ui/SessionsManagerDialog.ui \
    src/ui/SidebarWidget.ui \
    src/ui/StartupDialog.ui \
    src/ui/ToolBarDialog.ui \
    src/ui/TransferDialog.ui \
    src/ui/UpdateCheckerDialog.ui \
    src/ui/UserAgentsManagerDialog.ui \
    src/ui/WebsiteInformationDialog.ui \
    src/ui/WebsitePreferencesDialog.ui \
    src/ui/preferences/AcceptLanguageDialog.ui \
    src/ui/preferences/ContentBlockingDialog.ui \
    src/ui/preferences/JavaScriptPreferencesDialog.ui \
    src/ui/preferences/KeyboardProfileDialog.ui \
    src/ui/preferences/MouseProfileDialog.ui \
    src/ui/preferences/PreferencesAdvancedPageWidget.ui \
    src/ui/preferences/PreferencesContentPageWidget.ui \
    src/ui/preferences/PreferencesGeneralPageWidget.ui \
    src/ui/preferences/PreferencesPrivacyPageWidget.ui \
    src/ui/preferences/PreferencesSearchPageWidget.ui \
    src/modules/windows/bookmarks/BookmarksContentsWidget.ui \
    src/modules/windows/cache/CacheContentsWidget.ui \
    src/modules/windows/configuration/ConfigurationContentsWidget.ui \
    src/modules/windows/cookies/CookiesContentsWidget.ui \
    src/modules/windows/history/HistoryContentsWidget.ui \
    src/modules/windows/notes/NotesContentsWidget.ui \
    src/modules/windows/transfers/TransfersContentsWidget.ui \
    src/modules/windows/web/PasswordBarWidget.ui \
    src/modules/windows/web/PermissionBarWidget.ui \
    src/modules/windows/web/PopupsBarWidget.ui \
    src/modules/windows/web/SearchBarWidget.ui \
    src/modules/windows/web/StartPagePreferencesDialog.ui

RESOURCES += resources/resources.qrc \
    src/modules/backends/web/qtwebkit/QtWebKitResources.qrc

TRANSLATIONS += resources/translations/otter-browser_bg.ts \
    resources/translations/otter-browser_cs.ts \
    resources/translations/otter-browser_da.ts \
    resources/translations/otter-browser_de.ts \
    resources/translations/otter-browser_el.ts \
    resources/translations/otter-browser_en_CA.ts \
    resources/translations/otter-browser_en_GB.ts \
    resources/translations/otter-browser_en_US.ts \
    resources/translations/otter-browser_es.ts \
    resources/translations/otter-browser_es_MX.ts \
    resources/translations/otter-browser_et.ts \
    resources/translations/otter-browser_fi.ts \
    resources/translations/otter-browser_fr.ts \
    resources/translations/otter-browser_hr.ts \
    resources/translations/otter-browser_hu.ts \
    resources/translations/otter-browser_id.ts \
    resources/translations/otter-browser_it.ts \
    resources/translations/otter-browser_ja.ts \
    resources/translations/otter-browser_jbo.ts \
    resources/translations/otter-browser_ka.ts \
    resources/translations/otter-browser_lt.ts \
    resources/translations/otter-browser_nb.ts \
    resources/translations/otter-browser_nl.ts \
    resources/translations/otter-browser_pl.ts \
    resources/translations/otter-browser_pt.ts \
    resources/translations/otter-browser_pt_BR.ts \
    resources/translations/otter-browser_ro.ts \
    resources/translations/otter-browser_ru.ts \
    resources/translations/otter-browser_sk.ts \
    resources/translations/otter-browser_sl.ts \
    resources/translations/otter-browser_sr.ts \
    resources/translations/otter-browser_sr@ijekavian.ts \
    resources/translations/otter-browser_sr@ijekavianlatin.ts \
    resources/translations/otter-browser_sr@latin.ts \
    resources/translations/otter-browser_sv.ts \
    resources/translations/otter-browser_tr.ts \
    resources/translations/otter-browser_uk.ts \
    resources/translations/otter-browser_zh_CN.ts \
    resources/translations/otter-browser_zh_TW.ts

RC_FILE = otter-browser.rc
