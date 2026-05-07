<p align="center">
  <img src="docs/source/_static/images/guide/4.32.png" alt="UrConnect desktop interface" width="900">
</p>

<h1 align="center">UrConnect</h1>

<p align="center">
  Spatial configuration analysis for urban street networks, based on depthmapX and extended with reach, directional distance, weighted accessibility, and path-analysis workflows.
</p>

<p align="center">
  <a href="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml"><img src="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml/badge.svg" alt="Docs build"></a>
  <a href="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/release.yml"><img src="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/release.yml/badge.svg" alt="Release builds"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-0f766e" alt="GPL-3.0-or-later license"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-11-00599C" alt="C++11">
  <img src="https://img.shields.io/badge/Qt-5.15-41CD52" alt="Qt 5.15">
  <img src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-334155" alt="Windows macOS Linux">
</p>

<p align="center">
  <a href="docs/source/en/installation.md">Installation</a> |
  <a href="docs/source/en/getting-started.md">Getting Started</a> |
  <a href="docs/source/en/analysis-reach.md">User Guide</a> |
  <a href="docs/source/en/release-builds.md">Release Builds</a> |
  <a href="README.zh-CN.md">简体中文</a> |
  <a href="README.zh-TW.md">繁體中文</a>
</p>

## Overview

UrConnect is a standalone desktop tool for segment-based urban street-network analysis. It combines topological and metric distance concepts with street attributes such as length, population, POI counts, floor area, or other GIS variables.

The software supports workflows for:

- metric, directional, junction, and combined reach analysis
- interactive reach from selected source segments
- directional and junction distance analysis
- point distance analysis
- shortest-path simulation with manual OD input or OD matrix files
- visualization, screen export, attribute export, and Shapefile outputs

## Documentation

The documentation follows a ReadTheDocs/Sphinx style similar to OSMnx, with a left navigation, page table of contents, searchable pages, and three language sections:

- [English documentation](docs/source/en/installation.md)
- [简体中文文档](docs/source/zh-CN/installation.md)
- [繁體中文文件](docs/source/zh-TW/installation.md)

To build the documentation locally:

```bash
python3 -m pip install -r docs/requirements.txt
sphinx-build -b html docs/source docs/_build/html
```

## Installation

Release binaries are planned for Windows, macOS, and Linux. Windows/MSVC is the historically established path; macOS now builds locally with Homebrew Qt 5 and Boost; Linux should remain a preview target until the CI matrix is green.

Build from source:

```bash
cmake -S . -B build -DQT5_ROOT=/path/to/Qt/5.15 -DBOOST_ROOT=/path/to/boost
cmake --build build --config Release
```

See the [Installation guide](docs/source/en/installation.md) for platform-specific commands.

## Repository Layout

```text
.
├── depthmapX/        Qt desktop application, views, dialogs, resources, and UI files
├── salalib/          Core spatial analysis algorithms and graph/map data structures
├── genlib/           Shared geometry, math, parsing, and utility code
├── mgraph440/        Legacy graph-analysis code retained for compatibility
├── SNDAApp/          UrbanConnect analysis code and bundled Shapelib sources
├── docs/             Sphinx documentation in English, Simplified Chinese, Traditional Chinese
└── .github/          Issue templates, docs workflow, and release build workflow
```

## Citation

If you use UrConnect in academic work, please cite the project. See [CITATION.cff](CITATION.cff). The citation metadata should be updated with publication details before the repository is made public.

## License

UrConnect is licensed under the GNU General Public License v3.0 or later. This is the correct conservative license because UrConnect is derived from depthmapX/sala components carrying GPLv3-or-later notices. Bundled compatible components are summarized in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Acknowledgements

UrConnect builds on depthmapX, sala, genlib, and Shapelib. We thank the original contributors and the research collaborators from Shenzhen University and Georgia Institute of Technology who developed the UrbanConnect methods.
