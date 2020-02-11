# Gambatte-Speedrun

Fork of https://github.com/sinamas/gambatte with changes related to Pokemon speedrunning. Under GPLv2.

Gambatte-Speedrun has been updated to build using Qt5 instead of Qt4. Distributable binaries can be built on Windows using [msys2](https://msys2.github.io/) and the qt5-static package, or on macOS using [Homebrew](https://brew.sh/) and the qt5 package with its `macdeployqt` tool.

## Building

See [INSTALL.md](INSTALL.md) for how to set up the build environment. After completing the setup, running the following in the project's root directory should build the "PSR" version of Gambatte-Speedrun:
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
To build Gambatte-Speedrun's `libgambatte` (shared library; for use in scripting, TASing, and other programmatic use of the core emulator), you can run the following from the project root, regardless of platform:
```
$ sh scripts/build_shlib.sh
```
## Running Tests

To be able to run the upstream hwtests suite on Gambatte-Speedrun, you must acquire the DMG and CGB bootroms. Name the DMG bootrom `bios.gb`, the CGB bootrom `bios.gbc`, and move both into the `test` directory.

Run the following in a terminal from the project root to assemble and run all hwtests:
```
$ (cd test && sh scripts/assemble_tests.sh)
$ sh scripts/test.sh
```