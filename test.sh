(cd libgambatte && scons) || exit

(cd test && scons) || exit

(cd test && ./run_tests.sh)
