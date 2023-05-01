TARGET = PeerPlayer
DESTDIR = bin/

HEADERS = inc/*.hpp
SOURCES = src/*.cpp
OBJECTS_DIR = obj/
INCLUDEPATH = inc/

RESOURCES += res/res.qrc

QT += widgets
QT += network
QT += webengine
QT += webenginewidgets
QT += concurrent

DEFINES += PROGRAM=\\\"PeerPlayer\\\"
# TODO TEST
DEFINES += QT_MESSAGELOGCONTEXT

# missing return statement should be fatal
#QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS += -Werror=return-type

#CONFIG += lrelease embed_translations
#CONFIG += c++11
#CONFIG += c++14
#CONFIG += c++17

#unix: QMAKE_LFLAGS_RELEASE += -static-libstdc++ -static-libgcc -fvisibility=hidden -w
#unix: QMAKE_LFLAGS_DEBUG += -static-libstdc++ -static-libgcc 
#unix: QMAKE_LFLAGS_RELEASE += -static-libstdc++ -static-libgcc 
#CONFIG += static
#CONFIG += staticlib

# vlc-devel
#INCLUDEPATH += libvlcpp/
LIBS += -lvlc

# https://github.com/videolan/vlc/blob/master/doc/libvlc/QtPlayer/QtVLC.pro
#PKGCONFIG = libvlc

