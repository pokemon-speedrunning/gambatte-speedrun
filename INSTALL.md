# Setting up the build environment

These instructions explain how to set up an environment for building Gambatte-Speedrun from source on different operating systems. Once you've set up your build environment, see the main [README.md](README.md) for build instructions and information on running the test suite.

If all you want to do is build Gambatte-Speedrun's `libgambatte` shared library (for use in scripting, TASing, and other programmatic use of the emulation core), you do not need to perform the Qt-specific steps or the Testrunner-specific steps. The steps listed before the sections with specific steps are still required.

Running the hwtests suite requires installing an additional dependency (`libpng`); this can be skipped if you have no intention of using the testrunner (we primarily use the test suite to confirm no regressions in accuracy were introduced in our changes). Likewise, the Qt-specific steps can be skipped if you have no intention of building the full Qt UI application.

## Windows

***\*NOTE:*** The instructions below assume you're installing 64-bit MSYS2 and using the 32-bit MinGW toolchain (they do work with other configurations, but the commands won't be exactly the same).
* Choice of 32-bit vs 64-bit for MSYS2 doesn't really matter; either is fine
* There's currently no benefit to building a 64-bit version of Gambatte-Speedrun on Windows, so 32-bit is used for the release binaries

At the end of these steps, to do any build tasks related to Gambatte-Speedrun, open the MSYS2 MinGW 32-bit shell (under the MSYS2 folder in the Start menu; ***\*must be\**** this specific shell in order for building to work).

\
Do the following: 

\- Install [MSYS2](https://www.msys2.org/) by selecting the one-click installer exe for x86_64

\- Initial setup of MSYS2 (copied from [here](https://github.com/msys2/msys2/wiki/MSYS2-installation#iii-updating-packages))
```
Run MSYS2 shell (Command Prompt). C:\msys64\msys2_shell.cmd

Update msys2 core components and packages (if you have not done so yet):

$ pacman -Syuu

Follow the instructions. Repeat this step until it says there are no packages to update.

See the above link if you have an older installation of MSYS2 and/or pacman.
```
\- Prepare MSYS2 for Qt related build development environment (copied from [here](https://wiki.qt.io/MSYS2#Prepare_MSYS2_for_Qt_related_build_development_environment))
```
Start MSYS2-shell. Run/execute below commands to load MinGW-w64 SEH (64bit/x86_64) posix and Dwarf-2
(32bit/i686) posix toolchains & related other tools, dependencies & components from MSYS2 REPO
(MINGW-packages, MSYS2-packages):

$ pacman -S base-devel git mercurial cvs wget p7zip
$ pacman -S perl ruby python2 mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain

Note: the i686 (32bit) toolchain loads into /c/msys64/mingw32/bin (C:\msys64\mingw32\bin) directory
location, and, x86_64(64bit) toolchain loads into /c/msys64/mingw64/bin (C:\msys64\mingw64\bin)
directory. Perl, Ruby, Python, OpenSSL etc loads into /c/msys64/usr/bin (C:\msys64\usr\bin)
directory.
```
\- Install the `zlib` package:
```
$ pacman -S mingw-w64-i686-zlib
```

### Qt-specific steps

\- Acquire and install the Qt 5.6 static library environment *(NOTE: newer versions bundle a lot more into the environment and create much larger builds*)
```
$ curl -O http://repo.msys2.org/mingw/i686/mingw-w64-i686-qt5-static-5.6.2-4-any.pkg.tar.xz
$ pacman -U mingw-w64-i686-qt5-static-5.6.2-4-any.pkg.tar.xz
```
\- Install Gambatte-Speedrun dependencies that aren't bundled with Qt 5.6
```
$ pacman -S mingw-w64-i686-jasper mingw-w64-i686-libwebp mingw-w64-i686-dbus
```
\- Modify .bash_profile (in /c/msys64/home/\<USERNAME\>) add this line to the end of the file:
```
PATH="/mingw32/qt5-static/bin:${PATH}"
```

### Testrunner-specific steps

\- Install the `libpng` package:
```
$ pacman -S mingw-w64-i686-libpng
```

## macOS

Open the macOS Terminal and do the following:

\- Install Homebrew if not already installed (copied from [here](https://brew.sh/)):
```
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
\- Install the `scons` and `zlib` packages:
```
$ brew install scons zlib
```

### Qt-specific steps

\- Install Xcode Command Line Tools if not already installed:
```
$ xcode-select --install
```
\- Install Qt5 package through Homebrew *(NOTE: currently tested to work with XCode 11 and Qt 5.14.1, which supports macOS 10.13+; builds for older macOS versions should also be possible, but you would need to dig up the Homebrew formulae for older versions of Qt5)*
```
$ brew install qt
```
***\*NOTE:*** After building and creating "Gambatte Qt.app", you can run the following command to create a deployable standalone macOS app, which can be used as a release build that other macOS users can run:
```
$ macdeployqt gambatte_qt/bin/Gambatte\ Qt.app -dmg
```

### Testrunner-specific steps

\- Install the `libpng` package:
```
$ brew install libpng
```

## Debian

Open a terminal and do the following:

\- Install build dependencies:
```
$ sudo apt install build-essential git scons zlib1g-dev 
```

### Qt-specific steps

\- Install qt5 development packages:
```
$ sudo apt install qt5-default libqt5x11extras5-dev libxrandr-dev libxv-dev libasound2-dev
```

### Testrunner-specific steps

\- Install the `libpng-dev` package:
```
$ sudo apt install libpng-dev
```
