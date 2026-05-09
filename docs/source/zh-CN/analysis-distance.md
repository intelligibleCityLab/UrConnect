# 距离分析

距离分析不同于可达量分析：可达量衡量可达网络的总量，距离分析衡量从源线段到可达线段的平均或总距离。

## 转向距离

路径：`Analysis > Distance > Directional Distance`

Directional Distance 使用米制搜索半径和角度阈值。半径可以是数值，例如 `500`，也可以是 `n`，表示完整网络。

```{figure} ../_static/images/guide/4.44.png
:alt: Directional Distance 计算

Directional Distance 和按线段长度加权的 Directional Distance。
```

```{figure} ../_static/images/guide/4.45.png
:alt: Directional Distance 参数

Directional Distance 参数面板。
```

```{figure} ../_static/images/guide/4.46.png
:alt: Directional Distance 结果

Directional Distance 结果。
```

输出包括：

- `D`：按线段数量计算的平均转向距离
- `DL`：按线段长度加权的转向距离

## 交叉口距离

路径：`Analysis > Distance > Junction Distance`

Junction Distance 用交叉口数量替代方向变化次数。度数阈值为 `4` 时，四岔路口会被计数。

```{figure} ../_static/images/guide/4.47.png
:alt: Junction Distance 参数

Junction Distance 参数面板。
```

```{figure} ../_static/images/guide/4.48.png
:alt: Junction Distance 结果

Junction Distance 结果。
```

## 距离选项

Weighted Distance 使用数值属性参与平均距离计算。

```{figure} ../_static/images/guide/4.49.png
:alt: Distance 选项

Distance 选项面板。
```

```{figure} ../_static/images/guide/4.50.png
:alt: Distance 选项结果

Weighted Distance 结果。
```

## 点距离

Point Distance 计算一个所选线段到所有其他线段的距离。

```{figure} ../_static/images/guide/4.51.png
:alt: Point Distance 参数

Metric、directional 和 junction Point Distance 选项。
```

```{figure} ../_static/images/guide/4.52.png
:alt: Point Distance 结果

Point Distance 结果。
```
