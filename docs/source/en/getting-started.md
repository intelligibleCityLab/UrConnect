# Getting Started

## Get Started In 5 Steps

1. Prepare a cleaned segment-based street network in GIS.
2. Open the network with `File > Open`.
3. Choose an analysis under `Analysis`, such as `Reach`, `Distance`, or `Point Distance`.
4. Set the parameters, then click `Run`.
5. Review the map visualization, exported attributes, and generated fields.

## Data Requirements

UrConnect works with segment models. Each line represents a street segment. The current application does not edit geometry, so topology, snapping, duplicate lines, and attribute joins should be resolved before opening the file.

Supported input formats:

- Shapefile (`.shp`)
- CSV files containing segment endpoint coordinates
- TXT files containing segment endpoint coordinates

For Shapefile input, analysis results are written into the source file. For CSV/TXT input, UrConnect also creates a same-named Shapefile so results can be used in GIS.

### Coordinate System And Endpoint Connections

Use a projected coordinate reference system with metre units. UrConnect calculates lengths directly from the input X/Y coordinates; it does not read the `.prj` file, validate the coordinate reference system, reproject coordinates, or warn when input is unprojected (longitude/latitude). Reproject the data in GIS before importing it; supplying a `.prj` file alone does not convert the coordinates.

Snap endpoints that should connect before import. UrConnect identifies shared endpoints by matching X/Y coordinate strings formatted to 10 significant digits. It has no configurable snapping tolerance, and this formatting is not a fixed distance tolerance. Split lines at intended junctions so that they share endpoints.

## Core Concepts

Reach
: The amount of network reachable from a source segment under a distance rule. The quantity can be segment length, junction count, or selected weighted attributes.

Directional distance
: A topological distance based on turns. Small angular changes can be ignored with an angle threshold.

Junction distance
: A distance based on the number of junctions of a selected degree, such as 3-way or 4-way intersections.

Weighted reach and distance
: Attribute-weighted calculations using numeric fields such as population, POI counts, floor area, or other joined GIS variables.

## Interface Overview

```{figure} ../_static/images/guide/4.32.png
:alt: UrConnect interface

UrConnect desktop interface.
```

The left side contains tools, file operations, analysis modules, path analysis, visualization controls, and run/stop controls. The central map window displays the current network and analysis results. The right side lists numeric attributes from the opened file.
