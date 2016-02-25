#!/bin/sh

export PATH=~/mxe/usr/bin:$PATH

echo "cd libgambatte && scons"
(cd libgambatte && scons) || exit

echo "cd gambatte_qt && ~/mxe/usr/i686-w64-mingw32.static/qt/bin/qmake && make"
(cd gambatte_qt && ~/mxe/usr/i686-w64-mingw32.static/qt/bin/qmake && make)
