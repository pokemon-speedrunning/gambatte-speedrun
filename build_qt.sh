#!/bin/sh

echo "cd libgambatte && scons"
(cd libgambatte && scons -c && scons) || exit

echo "cd gambatte_qt && qmake && make"
(cd gambatte_qt && rm -f Makefile && rm -f src/Makefile* && qmake && make clean && make)
