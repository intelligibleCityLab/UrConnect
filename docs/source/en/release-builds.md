# Release Builds

The repository includes a release workflow for building desktop packages on Windows, macOS, and Linux. Windows/MSVC is the historically established path. macOS now builds locally with Homebrew Qt 5 and Boost, and Linux is configured through the same CMake target with Qt5, Boost, OpenGL, and GLU development packages.

## Release Strategy

1. Build every pull request on the three target platforms.
2. Publish release artifacts only from version tags such as `v0.1.0`.
3. Keep release archives source-compatible with GPLv3 obligations.
4. Mark any platform artifact as preview until its GitHub Actions build is consistently green.

## Local Build Tooling

Local builds are useful for diagnosis, but releases should be produced by GitHub Actions to keep binaries reproducible and separated by platform.

Recommended local tools:

- Windows: Visual Studio 2022, CMake, Qt 5.15, Boost
- macOS: Xcode Command Line Tools, Homebrew, CMake, `qt@5`, Boost
- Linux: GCC or Clang, CMake, Qt 5 development packages, Boost, OpenGL/GLU

## Packaging Outputs

The release workflow should create:

- zipped Windows executable directory
- compressed macOS app bundle
- compressed Linux executable directory

The package contents should include the executable, required Qt runtime libraries, icon resources, and license files.
