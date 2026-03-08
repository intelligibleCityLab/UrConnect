# UrConnect

UrConnect is a standalone spatial configuration analysis tool for urban street networks. It builds on depthmapX and extends it with UrbanConnect analysis methods based on reach and directional distance, combining topological and metric distance concepts with street attributes such as length, load, or other GIS fields.

[简体中文说明](README.zh-CN.md)

## What It Does

- Runs desktop spatial network analysis workflows for urban morphology and street-network research.
- Supports Shapefile-based GIS data so analyses can be combined with other urban datasets.
- Retains core depthmapX-style graph, segment, visibility, and map-analysis capabilities.
- Adds UrbanConnect measures designed to be more interpretable for design and planning comparisons.

## Project Layout

```text
.
├── depthmapX/        Qt desktop application, views, dialogs, resources, and UI files
├── salalib/          Core spatial analysis algorithms and graph/map data structures
├── genlib/           Shared geometry, math, parsing, and utility code
├── mgraph440/        Legacy graph-analysis code retained for compatibility
├── SNDAApp/          UrbanConnect analysis code and bundled Shapelib sources
├── docs/             Project documentation
└── .github/          GitHub issue, pull request, and project maintenance templates
```

This repository currently contains the desktop C++ application and its in-tree libraries. The main executable target is built from `depthmapX/`.

## Installation

UrConnect is currently built from source.

### Requirements

- CMake 3.13 or newer
- C++11-compatible compiler
- Qt 5.15.x with Core, Gui, Widgets, and OpenGL modules
- Boost headers
- OpenGL

The project has primarily been prepared for Windows/MSVC builds. Other platforms may require small CMake or dependency adjustments.

### Windows Build

Install Visual Studio, CMake, Qt 5, and Boost. Then configure the dependency roots explicitly:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="D:/program/QT/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="D:/program/boost_1_73_0"
cmake --build build --config Release
```

The executable is generated under `build/bin/Release/`.

## Documentation

See [docs/README.md](docs/README.md) for development notes, architecture details, build guidance, and release-preparation notes.

## Citation

If you use UrConnect in academic work, please cite the project. See [CITATION.cff](CITATION.cff) for machine-readable citation metadata. The citation metadata should be updated with publication details before the repository is made public.

## License

UrConnect is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE).

This is the correct conservative license for the project because UrConnect is derived from depthmapX/sala components that carry GPLv3-or-later notices. Some bundled components have their own compatible notices, including LGPL-covered genlib code and MIT-style or LGPL Shapelib code. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Acknowledgements

UrConnect is based on depthmapX and related sala/genlib components. We thank the original depthmapX, sala, genlib, and Shapelib contributors, as well as the research collaborators from Shenzhen University and Georgia Institute of Technology who developed the UrbanConnect methods.
