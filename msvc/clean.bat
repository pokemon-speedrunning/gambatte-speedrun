pushd .
cd gambatte_qt && nmake distclean
popd
pushd .
cd libgambatte
call scons -c
popd
pushd .
cd test
call scons -c
popd
del libgambatte\config.log
rd /s /q libgambatte\.scon*
del test\config.log
rd /s /q test\.scon*
