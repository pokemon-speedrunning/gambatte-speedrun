#!/bin/sh

echo "cd libgambatte && scons"
(cd gambatte_core/libgambatte && scons) || exit

echo "cd gambatte_qt && make"
(cd gambatte_qt && rm -f bin/gambatte_speedrun.exe && qmake && make)
