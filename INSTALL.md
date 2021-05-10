# Setting up the build environment

These instructions explain how to set up an environment for building Gambatte-Speedrun from source on different operating systems. Once you've set up your build environment, see the main [README.md](README.md) for build instructions and information on running the test suite.

If all you want to do is build Gambatte-Speedrun's `libgambatte` shared library (for use in scripting, "botting", TASing, and other programmatic use of the emulation core), you do not need to perform the Qt-specific steps or the Testrunner-specific steps. You'll still need to follow the "basic steps" for setting up the build environment.

Running the hwtests suite requires installing an additional dependency (`libpng`); this can be skipped if you have no intention of using the testrunner (we primarily use the test suite to confirm no regressions in accuracy were introduced in our changes). Likewise, the Qt-specific steps can be skipped if you have no intention of building the full Qt UI application.

---
*Platform-specific instructions:*
* [Windows](#windows)
* [macOS](#macos)
* [Debian/Ubuntu](#debianubuntu)

---
## Windows

***\*NOTE:*** The instructions below assume you're installing 64-bit MSYS2 and using the 32-bit MinGW toolchain (they work with other configurations, but the commands won't be exactly the same).
* Choice of 32-bit vs 64-bit for MSYS2 doesn't really matter; either is fine
* There's currently no benefit to building a 64-bit version of Gambatte-Speedrun on Windows, so 32-bit is used for the release binaries

***\*NOTE:*** At the end of these steps, to do any build tasks related to Gambatte-Speedrun, open the MSYS2 MinGW 32-bit shell (under the MSYS2 folder in the Start menu; ***\*must be\**** this specific shell in order for building to work).

### Basic steps

*Do the following:*

\- Install [MSYS2](https://www.msys2.org/) by selecting the one-click installer exe for x86_64

\- Run MSYS2 shell and update MSYS2 core components and packages *(copied from [here](https://www.msys2.org/wiki/MSYS2-installation/#iii-updating-packages))*:
```
$ pacman -Syuu

Follow the instructions. Repeat this step until it says there are no packages to update.
See the above link if you have an older installation of MSYS2 and/or pacman.
```
\- Install MSYS2 packages for general build development environment:
```
$ pacman -S base-devel git mingw-w64-i686-zlib mingw-w64-i686-toolchain
```

### Qt-specific steps

\- Acquire and install the qt5-static library environment:
```
$ pacman -S mingw-w64-i686-qt5-static
```
\- Modify `.bash_profile` to add qt5-static binaries to `PATH`:
```
$ echo 'if [[ "${MSYSTEM}" == "MINGW"* ]]; then PATH="/${MSYSTEM}/qt5-static/bin:${PATH}"; fi' >> ~/.bash_profile
```

### Testrunner-specific steps

\- Install the `libpng` package:
```
$ pacman -S mingw-w64-i686-libpng
```

## macOS

### Basic steps

*Open a terminal and do the following:*

\- Install Homebrew if not already installed *(copied from [here](https://brew.sh/))*:
```
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
\- Install the `scons` and `zlib` packages:
```
$ brew install scons zlib
```

### Qt-specific steps

\- Install Xcode Command Line Tools (if not already installed; newer Homebrew versions should install them by default):
```
$ xcode-select --install
```
\- Install Qt5 package through Homebrew *(NOTE: currently tested to work with XCode 11 and Qt 5.14.1, which supports macOS 10.13+; builds for older macOS versions should also be possible, but you would need to dig up the Homebrew formulae for older versions of Qt5)*:
```
$ brew install qt
```
***\*NOTE:*** After building and creating "Gambatte-Speedrun.app", you can run the following command to create a deployable standalone macOS app, which can be used as a release build that other macOS users can run:
```
$ macdeployqt gambatte_qt/bin/Gambatte-Speedrun.app -dmg
```

### Testrunner-specific steps

\- Install the `libpng` package:
```
$ brew install libpng
```

## Debian/Ubuntu

### Basic steps

*Open a terminal and do the following:*

\- Install build dependencies:
```
$ sudo apt install build-essential git scons zlib1g-dev
```

### Qt-specific steps

\- Install Qt5 development packages, plus Gambatte-Speedrun dependencies that aren't bundled with Qt5:
```
$ sudo apt install qt5-default libqt5x11extras5-dev libxrandr-dev libxv-dev libasound2-dev
```

### Testrunner-specific steps

\- Install the `libpng-dev` package:
```
$ sudo apt install libpng-dev
```
