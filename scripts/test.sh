#!/bin/sh

echo "cd libgambatte && scons"
(cd libgambatte && scons) || exit

echo "cd test && scons"
(cd test && scons) || exit

echo "cd test && sh scripts/run_tests.sh"
(cd test && sh scripts/run_tests.sh)
