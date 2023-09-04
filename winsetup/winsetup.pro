TEMPLATE = lib
QT -= core gui
CONFIG += staticlib

CONFIG(debug,debug|release) {
    DESTDIR = $$PWD/../../bin/debug
}

CONFIG(release,debug|release) {
    DESTDIR = $$PWD/../../bin/release
}

HEADERS += winsetup.h\
    zip.h
SOURCES += winsetup.c\
    zip.c
