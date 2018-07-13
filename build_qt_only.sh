#!/bin/sh

(cd gambatte_qt && rm -f Makefile && rm -f src/Makefile* && qmake && make clean && make)
