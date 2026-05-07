# Distance Analysis

Distance analysis differs from Reach analysis: Reach measures the amount of reachable network, while Distance measures average or total distance from a source segment to reachable segments.

## Directional Distance

Path: `Analysis > Distance > Directional Distance`

Directional Distance uses a metric search radius and an angle threshold. A radius can be numeric, such as `500`, or `n` for the full network.

```{figure} ../_static/images/guide/4.44.png
:alt: Directional Distance calculation

Directional Distance and Directional Distance per Segment Length.
```

```{figure} ../_static/images/guide/4.45.png
:alt: Directional Distance parameters

Directional Distance parameter panel.
```

```{figure} ../_static/images/guide/4.46.png
:alt: Directional Distance result

Directional Distance result.
```

The outputs include:

- `D`: mean directional distance by segment count
- `DL`: directional distance weighted by segment length

## Junction Distance

Path: `Analysis > Distance > Junction Distance`

Junction Distance replaces directional-change counts with junction counts. A degree threshold of `4` counts 4-way intersections.

```{figure} ../_static/images/guide/4.47.png
:alt: Junction Distance parameters

Junction Distance parameter panel.
```

```{figure} ../_static/images/guide/4.48.png
:alt: Junction Distance result

Junction Distance result.
```

## Distance Options

Weighted Distance uses numeric attributes in the average distance calculation.

```{figure} ../_static/images/guide/4.49.png
:alt: Distance options

Distance option panel.
```

```{figure} ../_static/images/guide/4.50.png
:alt: Distance option result

Weighted Distance result.
```

## Point Distance

Point Distance is similar to Step Depth in depthmap. It computes the distance from one selected segment to every other segment.

```{figure} ../_static/images/guide/4.51.png
:alt: Point Distance parameters

Metric, directional, and junction Point Distance options.
```

```{figure} ../_static/images/guide/4.52.png
:alt: Point Distance result

Point Distance result.
```
