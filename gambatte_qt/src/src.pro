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
QMAKE_CXXFLAGS += -fomit-frame-pointer -fno-exceptions -fno-rtti -fpermissive
TARGET = gambatte_qt

macx:TARGET = "Gambatte Qt"
DESTDIR = ../bin
INCLUDEPATH += ../../libgambatte/include
DEPENDPATH  += ../../libgambatte/include
QT += widgets gui-private
LIBS += -L../../libgambatte -lgambatte -lz
win32 {
	LIBS += -lole32
	#RC_FILE = gambatteicon.rc
}
unix:!macx {
	QT += x11extras
}
exists(../../.git) {
	MY_GIT_REVNO = $$system(git rev-list HEAD --count)
	!isEmpty(MY_GIT_REVNO):DEFINES += GAMBATTE_QT_VERSION_STR='\\"r$$MY_GIT_REVNO\\"'
}

# debug symbols
#QMAKE_CXXFLAGS_RELEASE += -g
#QMAKE_CFLAGS_RELEASE += -g
#QMAKE_LFLAGS_RELEASE =

# dmg support
#DEFINES += DMG_SUPPORT

# gbc rng option
#DEFINES += CGB_RNG_OPTION
