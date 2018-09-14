#!/bin/sh

echo "cd gambatte_qt && qmake && make"
(cd gambatte_qt && rm -f Makefile && rm -f src/Makefile* && qmake && make clean && make)
