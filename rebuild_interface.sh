(cd libgambatte && scons) || exit

(cd gambatte_qt && rm -f bin/gambatte_qt.exe && make)