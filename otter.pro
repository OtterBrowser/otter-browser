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
    src/core/WindowsManager.cpp

HEADERS += src/ui/MainWindow.h \
    src/ui/CookiesDialog.h \
    src/ui/PreferencesDialog.h \
    src/core/SettingsManager.h \
    src/core/Application.h \
    src/ui/TabBarWidget.h \
    src/ui/Window.h \
    src/core/WindowsManager.h

FORMS += src/ui/MainWindow.ui \
    src/ui/CookiesDialog.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/Window.ui
