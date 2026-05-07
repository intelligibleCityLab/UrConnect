# Example Applications

UrConnect can combine geometric-topological network structure with street attributes and GIS variables. This makes the resulting metrics easier to compare across cases and useful for urban design, planning, and morphology research.

## Directional Reach Across Cities

```{figure} ../_static/images/guide/4.59.png
:alt: Hangzhou and Shanghai directional reach comparison

Two-directional-change reach comparison for Hangzhou and Shanghai.
```

The guide example compares Hangzhou's main urban area and Shanghai's inner-ring area. Although their street densities and average segment lengths are similar, global two-turn reach differs strongly, suggesting different city-scale structural roles.

## Weighted Reach With Population And POI Data

```{figure} ../_static/images/guide/4.60.png
:alt: Shanghai weighted reach

Street-length, population, and POI reach in Shanghai.
```

Weighted reach can show how functional centrality differs from street-network centrality. In the Shanghai example, POI reach highlights the Bund area more strongly than street-length reach alone.

## POI Clustering

```{figure} ../_static/images/guide/4.61.png
:alt: Hangzhou POI clustering

Street clustering by 13 POI reach categories in Hangzhou.
```

```{figure} ../_static/images/guide/4.62.png
:alt: Hangzhou POI cluster statistics

Cluster statistics for 13 POI reach categories.
```

After computing reachable POI counts by category, downstream clustering can identify functionally similar and dissimilar street environments.
