#!/bin/sh

echo "cd libgambatte && scons shlib"
(cd libgambatte && scons -c shlib && scons shlib)
