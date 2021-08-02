COMMONPATH = ../../common
include(framework/framework.pro)

SOURCES += main.cpp \
    cheatdialog.cpp \
    fpsselector.cpp \
    gambattesource.cpp \
    gambattemenuhandler.cpp \
    miscdialog.cpp \
    palettedialog.cpp \
    pathselector.cpp
HEADERS += *.h
SOURCES += $$COMMONPATH/videolink/rgb32conv.cpp \
    $$COMMONPATH/videolink/vfilterinfo.cpp \
    $$COMMONPATH/videolink/vfilters/catrom2x.cpp \
    $$COMMONPATH/videolink/vfilters/catrom3x.cpp \
    $$COMMONPATH/videolink/vfilters/kreed2xsai.cpp \
    $$COMMONPATH/videolink/vfilters/maxsthq2x.cpp \
    $$COMMONPATH/videolink/vfilters/maxsthq3x.cpp
HEADERS += $$COMMONPATH/videolink/*.h \
           $$COMMONPATH/videolink/vfilters/*.h
TEMPLATE = app
CONFIG += warn_on \
    release
QMAKE_CFLAGS   += -fomit-frame-pointer
QMAKE_CXXFLAGS += -fomit-frame-pointer -fno-exceptions -fno-rtti

DESTDIR = ../bin
INCLUDEPATH += ../../gambatte_core/libgambatte/include
DEPENDPATH  += ../../gambatte_core/libgambatte/include
QT += widgets gui-private
LIBS += ../../gambatte_core/libgambatte/libgambatte.a -lz
win32 {
	LIBS += -lole32
	#RC_FILE = gambatteicon.rc
}
unix:!macx {
	QT += x11extras
}

TARGET = "gambatte_speedrun"
macx:TARGET = "Gambatte-Speedrun"
VERSION_STR = interim  # default to interim in case there's an issue getting revision count

exists(../../.git) {
	MY_GIT_REVNO = $$system(git rev-list HEAD --count)
	!isEmpty(MY_GIT_REVNO) {
		VERSION_STR = r$$MY_GIT_REVNO
	}
}

DEFINES += GSR_VERSION_STR='\\"$$VERSION_STR\\"'

# debug symbols
#QMAKE_CXXFLAGS_RELEASE += -g
#QMAKE_CFLAGS_RELEASE += -g
#QMAKE_LFLAGS_RELEASE =

# additional platforms
exists(platforms.pri) {
	include(platforms.pri)
}
