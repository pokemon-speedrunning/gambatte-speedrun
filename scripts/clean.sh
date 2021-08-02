#!/bin/sh

echo "cd gambatte_qt && make distclean"
(cd gambatte_qt && make distclean)

echo "cd libgambatte && scons -c ."
(cd gambatte_core/libgambatte && scons -c .)

echo "cd test && scons -c"
(cd gambatte_core/test && scons -c)

echo "rm -f *gambatte*/config.log"
rm -f *gambatte*/config.log

echo "rm -rf *gambatte*/.scon*"
rm -rf *gambatte*/.scon*

echo "rm -f test/config.log"
rm -f gambatte_core/test/config.log

echo "rm -rf test/.scon*"
rm -rf gambatte_core/test/.scon*
