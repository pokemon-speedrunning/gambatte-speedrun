# Gambatte-Speedrun

Fork of https://github.com/sinamas/gambatte with changes related to Pokemon speedrunning. Under GPLv2.

Gambatte-Speedrun has been updated to build using Qt5 instead of Qt4. Distributable binaries can be built on Windows using [msys2](https://msys2.github.io/) and the qt5-static package, or on macOS using [Homebrew](https://brew.sh/) and the qt5 package with its `macdeployqt` tool.

## Building

See [INSTALL.md](INSTALL.md) for how to set up the build environment. The amount of setup you need to do depends on exactly what you are planning to build and use.

### Shared Library

If you only wish to build Gambatte-Speedrun's `libgambatte` shared library (for use in scripting, TASing, and other programmatic use of the emulation core), and have the basic build environment set up, you can run the following from the project's root directory, regardless of platform:
```
$ sh scripts/build_shlib.sh
```

### Gambatte-Speedrun *(i.e. the full-blown emulator)*

After completing all the Qt-specific build steps in [INSTALL.md](INSTALL.md), running the following in the project's root directory should build the "PSR" version of Gambatte-Speedrun:
```
$ sh scripts/build_qt.sh
```
To instead build the "non-PSR" version, with additional selectable platforms (GB, GBC, GBA, SGB2), create `gambatte_qt/src/platforms.pri` with the following (before running `build_qt.sh`):
```
# platform support
# GBP is hardcoded
DEFINES += SHOW_PLATFORM_GB
DEFINES += SHOW_PLATFORM_GBC
DEFINES += SHOW_PLATFORM_GBA
DEFINES += SHOW_PLATFORM_SGB
```

### Testrunner

To be able to run the upstream hwtests suite on Gambatte-Speedrun, you must acquire the DMG and CGB bootroms. Name the DMG bootrom `bios.gb`, the CGB bootrom `bios.gbc`, and move both into the `test` directory.

Run the following in a terminal from the project's root directory to assemble and run all hwtests:
```
$ (cd test && sh scripts/assemble_tests.sh)
$ sh scripts/test.sh
```
Note that the first line (with `assemble_tests.sh`) only needs to be run one time, or until the contents of the hwtests directory change.