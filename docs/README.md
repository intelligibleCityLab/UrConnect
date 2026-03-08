# UrConnect Documentation

This documentation is intentionally lightweight until the repository is ready for public release. It should grow into a user guide, developer guide, examples gallery, and API/build reference.

[简体中文](README.zh-CN.md)

## Architecture Notes

UrConnect is organized as a CMake-based C++ desktop application:

- `depthmapX/` contains the Qt application shell, dialogs, views, OpenGL rendering, resources, and the final executable target.
- `salalib/` contains the main spatial analysis engine, including axial, segment, visibility graph, isovist, agent, parser, and map-conversion modules.
- `genlib/` contains shared geometry, math, data structure, XML, and helper utilities inherited from depthmapX-related code.
- `mgraph440/` keeps legacy graph-analysis structures used for compatibility with older formats and workflows.
- `SNDAApp/` contains UrbanConnect-specific analysis code and bundled Shapelib sources for Shapefile I/O.

## Build Notes

Prefer out-of-tree builds:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="path/to/Qt/5.15.x/msvc2019_64" `
  -DBOOST_ROOT="path/to/boost"
cmake --build build --config Release
```

The top-level CMake project exposes `QT5_ROOT` and `BOOST_ROOT` as cache variables. Do not hard-code machine-specific dependency paths in committed project files.

## Open Source Checklist

Before making the repository public:

- Confirm the public project name and repository description.
- Replace placeholder citation metadata in `CITATION.cff` with the final citation.
- Review all bundled third-party code and notices in `THIRD_PARTY_NOTICES.md`.
- Confirm that no private datasets, generated binaries, build outputs, tokens, or institutional-only files are tracked.
- Add screenshots, tutorial data, and examples only if their redistribution terms are clear.
- Decide whether releases will ship source-only packages or signed binary installers.

## Maintenance

Use GitHub issues for bugs and feature requests. Use pull requests for code changes. Keep documentation changes bilingual where practical, with English as the default.
