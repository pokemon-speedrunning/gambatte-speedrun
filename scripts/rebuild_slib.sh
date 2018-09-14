#!/bin/sh

echo "cd libgambatte && scons slib"
(cd libgambatte && scons -c slib && scons slib)
