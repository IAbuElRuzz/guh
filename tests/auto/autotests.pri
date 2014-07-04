QT += testlib network
CONFIG += testcase c++11

include($$top_srcdir/server/server.pri)

DEFINES += TESTING_ENABLED
INCLUDEPATH += $$top_srcdir/server/ $$top_srcdir/server/jsonrpc $$top_srcdir/libguh $$top_srcdir/tests/auto/
LIBS += -L$$top_builddir/libguh/ -lguh -L$$top_builddir/plugins/deviceplugins/mock/ -lguh_devicepluginmock

SOURCES += ../guhtestbase.cpp \
    ../mocktcpserver.cpp \

HEADERS += ../guhtestbase.h \
    ../mocktcpserver.h
