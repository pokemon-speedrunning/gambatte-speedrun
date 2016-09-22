#!/bin/sh

export PATH=~/mxe/usr/bin:$PATH

cd gambatte_qt
~/mxe/usr/i686-w64-mingw32.static/qt/bin/qmake
make
