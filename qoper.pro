#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T21:40:25
#
#-------------------------------------------------

QT += core gui webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qoper
TEMPLATE = app

SOURCES += src/main.cpp\
    src/ui/MainWindow.cpp \
    src/ui/PreferencesDialog.cpp \
    src/ui/CokiesDialog.cpp

HEADERS += src/ui/MainWindow.h \
    src/ui/PreferencesDialog.h \
    src/ui/CokiesDialog.h

FORMS += src/ui/MainWindow.ui \
    src/ui/PreferencesDialog.ui \
    src/ui/CokiesDialog.ui
