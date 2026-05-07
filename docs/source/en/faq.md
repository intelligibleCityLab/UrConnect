# Frequently Asked Questions

## Is UrConnect only for Windows?

No. The CMake project defines Windows, macOS, and Linux targets. Windows is currently the most established build path. macOS has been configured locally with Homebrew Qt 5 and Boost, and compile-time portability issues are being fixed. Linux should be supported through CI once case-sensitive source paths and GLU dependencies are consistently handled.

## Does UrConnect edit street geometry?

No. Prepare and clean the street segment model in GIS or another drawing tool before opening it in UrConnect.

## Which input format should I use?

Use Shapefile when GIS interoperability is the priority. Use CSV/TXT when long field names matter, because Shapefile DBF fields are limited to 10 characters.

## Why are some fields hidden in the attribute panel?

The attribute panel shows numeric fields for analysis and visualization. Text fields are hidden.

## Can I run multiple radii at once?

Several all-source analyses accept comma-separated radii, such as `500,1000,n`. Angle thresholds generally do not accept multiple values in the same run.
