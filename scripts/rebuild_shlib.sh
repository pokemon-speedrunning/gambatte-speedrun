#!/bin/sh

echo "cd libgambatte && scons shlib"
(cd libgambatte && scons -c shlib && scons shlib)

if [ -d	 "../../../Assets/" ]
then
cp ./libgambatte.dll ../../../Assets/libgambatte.dll
cp ./libgambatte.so ../../../Assets/libgambatte.dll.so
fi

if [ -d	 "../../../output/" ]
then
cp ./libgambatte.dll ../../../output/libgambatte.dll
cp ./libgambatte.so ../../../output/libgambatte.dll.so
fi
