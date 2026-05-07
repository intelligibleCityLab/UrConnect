# Reach Analysis

Reach analysis is available under `Analysis > Reach` for all-source calculations and `Analysis > Interactive Reach` for selected-source calculations.

## Metric Reach

Path: `Analysis > Reach > Metric Reach`

Metric Reach computes the total length of segments reachable within a metric radius. A radius of `500` creates a Shapefile field named `R500` and a CSV/TXT field named `R_500`.

```{figure} ../_static/images/guide/4.33.png
:alt: Metric Reach parameters

Metric Reach parameter panel.
```

```{figure} ../_static/images/guide/4.34.png
:alt: Metric Reach result

Metric Reach result.
```

Metric Reach also generates mean metric distance, such as `mMD500`, because the same search process can compute both reachable length and average metric distance.

## Directional Reach

Path: `Analysis > Reach > Directional Reach`

Directional Reach uses turn counts and an angle threshold. `Directional Change = 2` and `Angle Threshold = 20` creates a field such as `R2d20a`.

```{figure} ../_static/images/guide/4.35.png
:alt: Directional Reach parameters

Directional Reach parameter panel.
```

```{figure} ../_static/images/guide/4.36.png
:alt: Directional Reach result

Directional Reach result.
```

For regular grids, an angle threshold around 5 to 10 degrees is often sufficient. For organic networks, 15 to 30 degrees may be more appropriate. A turn count of 2 is often useful for identifying the difference between main streets and ordinary streets.

## Junction Reach

Path: `Analysis > Reach > Junction Reach`

Junction Reach limits the search by the number of junctions. `Junctions = 5` and `Degree Thresholds = 4` means that only 4-way intersections are counted as junctions, producing a field such as `R5j4x`.

```{figure} ../_static/images/guide/4.37.png
:alt: Junction Reach parameters

Junction Reach parameter panel.
```

```{figure} ../_static/images/guide/4.38.png
:alt: Junction Reach result

Junction Reach result.
```

## Combined Reach

Path: `Analysis > Reach > Combined Reach`

Combined Reach applies both a metric radius and a directional radius. For example, `500 m`, `2` turns, and `20` degrees describes segments reachable within 500 meters and no more than two directional changes.

```{figure} ../_static/images/guide/4.39.png
:alt: Combined Reach parameters

Combined Reach parameter panel.
```

```{figure} ../_static/images/guide/4.40.png
:alt: Combined Reach result

Combined Reach result.
```

## Options

Reach options include junction counting and weighted calculations.

```{figure} ../_static/images/guide/4.41.png
:alt: Weighted Reach parameters

Weighted Reach field selection.
```

```{figure} ../_static/images/guide/4.42.png
:alt: Weighted Reach result

Weighted Reach result.
```

Weighted calculations use selected numeric fields as the quantity being reached. For example, weighted reach by `Shopcount` estimates the amount of reachable shop-related activity rather than only reachable street length.

## Interactive Reach

Interactive Reach computes reach from one or several selected source segments. It returns temporary text summaries in the information panel and draws the reachable subnetwork as a `Reach by Selection` layer.

```{figure} ../_static/images/guide/4.43.png
:alt: Interactive Reach

Interactive Reach parameters and result.
```
