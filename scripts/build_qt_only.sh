#!/bin/sh

echo "cd gambatte_qt && make"
(cd gambatte_qt && rm -f bin/gambatte_speedrun.exe && qmake && make)
