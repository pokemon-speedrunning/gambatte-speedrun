pushd .
cd libgambatte
call scons
popd
pushd .
cd gambatte_qt
qmake
nmake
popd
