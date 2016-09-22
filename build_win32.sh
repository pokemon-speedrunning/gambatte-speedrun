#!/bin/sh

export PATH=~/mxe/usr/bin:$PATH

echo "cd libgambatte && scons"
cd libgambatte
scons -c
scons

cd ../gambatte_qt
rm -f Makefile
rm -f src/Makefile*
~/mxe/usr/i686-w64-mingw32.static/qt/bin/qmake
make clean
make
