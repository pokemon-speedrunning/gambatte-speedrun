# gambatte-speedrun

Fork of https://github.com/sinamas/gambatte with changes related to Pokemon speedrunning. Under GPLv2.

This has been updated to build using Qt5 instead of Qt4. If you have Qt5 dependencies, building should be easy enough. It can be built on windows using [msys2](https://msys2.github.io/) and the qt5-static package.

To build it on Debian, do:

    sudo apt install build-essential git scons zlib1g-dev qt5-default libqt5x11extras5-dev libxrandr-dev libxv-dev libasound2-dev
    git clone https://github.com/Dabomstew/gambatte-speedrun
    cd gambatte-speedrun
    ./build_qt.sh
    cp gambatte_qt/bin/gambatte_qt SOMEWHERE/gsr # or w/e

Otherwise GLHF.
