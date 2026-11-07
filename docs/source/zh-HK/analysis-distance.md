# 距離分析

距離分析不同於可達量分析：可達量衡量可達網路的總量，距離分析衡量從源線段到可達線段的平均或總距離。

## 轉向距離

路徑：`Analysis > Distance > Directional Distance`

Directional Distance 使用米制搜尋半徑和角度閾值。半徑可以是數值，例如 `500`，也可以是 `n`，表示完整網路。

```{figure} ../_static/images/guide/4.44.png
:alt: Directional Distance 計算

Directional Distance 和按線段長度加權的 Directional Distance。
```

```{figure} ../_static/images/guide/4.45.png
:alt: Directional Distance 參數

Directional Distance 參數面板。
```

```{figure} ../_static/images/guide/4.46.png
:alt: Directional Distance 結果

Directional Distance 結果。
```

輸出包括：

- `D`：按線段數量計算的平均轉向距離
- `DL`：按線段長度加權的轉向距離

## 交叉口距離

路徑：`Analysis > Distance > Junction Distance`

Junction Distance 用交叉口數量替代方向變化次數。度數閾值為 `4` 時，四岔路口會被計數。

```{figure} ../_static/images/guide/4.47.png
:alt: Junction Distance 參數

Junction Distance 參數面板。
```

```{figure} ../_static/images/guide/4.48.png
:alt: Junction Distance 結果

Junction Distance 結果。
```

## 距離選項

Weighted Distance 使用數值屬性參與平均距離計算。

```{figure} ../_static/images/guide/4.49.png
:alt: Distance 選項

Distance 選項面板。
```

```{figure} ../_static/images/guide/4.50.png
:alt: Distance 選項結果

Weighted Distance 結果。
```

## 點距離

Point Distance 計算一個所選線段到所有其他線段的距離。

```{figure} ../_static/images/guide/4.51.png
:alt: Point Distance 參數

Metric、directional 和 junction Point Distance 選項。
```

```{figure} ../_static/images/guide/4.52.png
:alt: Point Distance 結果

Point Distance 結果。
```
