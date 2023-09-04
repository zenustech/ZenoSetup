TEMPLATE = app

TARGET = uninstall

QT+= widgets

include($$PWD/../metainfo.pri)
QMAKE_TARGET_DESCRIPTION = ZENO-uninstaller

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

CONFIG(debug,debug|release) {
    DESTDIR = $$PWD/../../bin/debug
}

CONFIG(release,debug|release) {
    DESTDIR = $$PWD/../../bin/release
}


HEADERS += uninstall.h

SOURCES += uninstall.cpp \
           main.cpp

RESOURCES += uninstall.qrc


LIBS += $$DESTDIR/winsetup.lib
