#!/bin/sh
#./testrunner hwtests/*.gb* hwtests/*/*.gb* hwtests/*/*/*.gb* hwtests/*/*/*/*.gb*
find hwtests -name '*.gb*' -print0 | xargs -0 ./testrunner
