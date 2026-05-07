# Installation

UrConnect is currently distributed as source code and as planned release binaries. The desktop application is written in C++ with Qt 5, CMake, Boost headers, OpenGL, and bundled Shapelib sources.

## Supported Platforms

The CMake project contains platform branches for Windows, macOS, and Linux. Windows/MSVC is the most established build path. macOS and Linux are intended targets, but they should be treated as release candidates until the continuous integration builds are green on all three platforms.

## Install From A Release

When release artifacts are available, download the package for your operating system from the GitHub Releases page:

- Windows: `UrConnect-windows-x64.zip`
- macOS: `UrConnect-macos-arm64.tar.gz` or `UrConnect-macos-x64.tar.gz`
- Linux: `UrConnect-linux-x64.tar.gz`

Unpack the archive and run the `depthmapX` executable or application bundle. The executable name is inherited from the original depthmapX target while the project and documentation use the UrConnect name.

## Build From Source

Install:

- CMake 3.13 or newer
- A C++11 compiler
- Qt 5.15 with Core, Gui, Widgets, and OpenGL modules
- Boost headers
- OpenGL and GLU development libraries

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="C:/Qt/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build --config Release
```

The executable is generated under `build/bin/Release/`.

### macOS

With Homebrew:

```bash
brew install cmake qt@5 boost
cmake -S . -B build \
  -DQT5_ROOT="$(brew --prefix qt@5)" \
  -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build --config Release -j 4
```

### Linux

On Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qtbase5-dev libqt5opengl5-dev \
  libboost-all-dev libgl1-mesa-dev libglu1-mesa-dev
cmake -S . -B build -DBOOST_ROOT=/usr/include
cmake --build build --config Release -j 4
```

## Build Notes

- The input network must be a segment model prepared in GIS or another drawing tool.
- UrConnect does not provide geometry editing tools.
- Shapefile outputs are written back to the opened source file.
- CSV/TXT inputs are backed by a generated Shapefile output for GIS compatibility.
