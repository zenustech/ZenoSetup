TEMPLATE = app

QT += core gui widgets network

TARGET = ZENO-Setup

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/../metainfo.pri)
QMAKE_TARGET_DESCRIPTION = ZENO-Setup

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

CONFIG(debug,debug|release) {
    DESTDIR = $$PWD/../../bin/debug
}

CONFIG(release,debug|release) {
    DESTDIR = $$PWD/../../bin/release
}


SOURCES += \
    frmmain.cpp \
    main.cpp \
    unzip.cpp \
    winapi.cpp \
    zip.cpp \
    zsinstance.cpp \
    zsnetthread.cpp

HEADERS += \
    Dpi.h \
    frmmain.h \
    unzip.h \
    winapi.h \
    zip.h \
    zsinstance.h \
    zsnetthread.h

RESOURCES += \
    install.qrc

INCLUDEPATH += $$PWD/../../thr/include

LIBS += user32.lib $$DESTDIR/winsetup.lib ws2_32.lib winmm.lib wldap32.lib Advapi32.lib Crypt32.lib

CONFIG(debug,debug|release) {
    LIBS += -L$$PWD/../../thr/libs/x64/static/debug \
        -llibcrypto \
        -llibcurl \
        -llibssl
}

CONFIG(release,debug|release) {
LIBS += -L$$PWD/../../thr/libs/x64/static/release \
    -llibcrypto \
    -llibcurl \
    -llibssl
}
