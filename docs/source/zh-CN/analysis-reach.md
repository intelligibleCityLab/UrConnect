# 可达量分析

可达量分析位于 `Analysis > Reach`，用于全源计算；`Analysis > Interactive Reach` 用于所选源线段计算。

## 米制可达量

路径：`Analysis > Reach > Metric Reach`

Metric Reach 计算指定米制半径内可达线段的总长度。半径为 `500` 时，Shapefile 字段名为 `R500`，CSV/TXT 字段名为 `R_500`。

```{figure} ../_static/images/guide/4.33.png
:alt: Metric Reach 参数

Metric Reach 参数面板。
```

```{figure} ../_static/images/guide/4.34.png
:alt: Metric Reach 结果

Metric Reach 结果。
```

Metric Reach 还会生成平均米制距离字段，例如 `mMD500`，因为同一搜索过程可以同时计算可达长度和平均米制距离。

## 转向可达量

路径：`Analysis > Reach > Directional Reach`

Directional Reach 使用转向次数和角度阈值。`Directional Change = 2` 且 `Angle Threshold = 20` 时，会生成类似 `R2d20a` 的字段。

```{figure} ../_static/images/guide/4.35.png
:alt: Directional Reach 参数

Directional Reach 参数面板。
```

```{figure} ../_static/images/guide/4.36.png
:alt: Directional Reach 结果

Directional Reach 结果。
```

对于规则网格，角度阈值通常设置为 5 到 10 度即可。对于有机路网，15 到 30 度可能更合适。转向次数 2 常用于识别主街与普通街道之间的差异。

## 交叉口可达量

路径：`Analysis > Reach > Junction Reach`

Junction Reach 通过交叉口数量限制搜索范围。`Junctions = 5` 且 `Degree Thresholds = 4` 表示只把四岔路口计为交叉口，并生成类似 `R5j4x` 的字段。

```{figure} ../_static/images/guide/4.37.png
:alt: Junction Reach 参数

Junction Reach 参数面板。
```

```{figure} ../_static/images/guide/4.38.png
:alt: Junction Reach 结果

Junction Reach 结果。
```

## 组合可达量

路径：`Analysis > Reach > Combined Reach`

Combined Reach 同时应用米制半径和转向半径。例如 `500 m`、`2` 次转向、`20` 度表示线段必须在 500 米范围内，并且方向变化不超过两次。

```{figure} ../_static/images/guide/4.39.png
:alt: Combined Reach 参数

Combined Reach 参数面板。
```

```{figure} ../_static/images/guide/4.40.png
:alt: Combined Reach 结果

Combined Reach 结果。
```

## 选项

可达量选项包括交叉口计数和加权计算。

```{figure} ../_static/images/guide/4.41.png
:alt: Weighted Reach 参数

Weighted Reach 字段选择。
```

```{figure} ../_static/images/guide/4.42.png
:alt: Weighted Reach 结果

Weighted Reach 结果。
```

加权计算使用所选数值字段作为被可达的数量。例如，按 `Shopcount` 加权的可达量估计的是可达商铺相关活动量，而不仅是可达街道长度。

## 交互式可达量

Interactive Reach 从一个或多个所选源线段计算可达量。它会在信息面板返回临时文本摘要，并将可达子网络绘制为 `Reach by Selection` 图层。

```{figure} ../_static/images/guide/4.43.png
:alt: Interactive Reach

Interactive Reach 参数和结果。
```
