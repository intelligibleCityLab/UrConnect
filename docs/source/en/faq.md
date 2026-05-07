# Frequently Asked Questions

## Is UrConnect only for Windows?

No. Windows is the primary v0.1.0 release target, and macOS/Linux packages are available as experimental builds. The core analysis code is shared across platforms; the experimental label reflects packaging and validation status.

## Does UrConnect edit street geometry?

No. Prepare and clean the street segment model in GIS or another drawing tool before opening it in UrConnect.

## Which input format should I use?

Use Shapefile when GIS interoperability is the priority. Use CSV/TXT when long field names matter, because Shapefile DBF fields are limited to 10 characters.

## Why are some fields hidden in the attribute panel?

The attribute panel shows numeric fields for analysis and visualization. Text fields are hidden.

## Can I run multiple radii at once?

Several all-source analyses accept comma-separated radii, such as `500,1000,n`. Angle thresholds generally do not accept multiple values in the same run.
