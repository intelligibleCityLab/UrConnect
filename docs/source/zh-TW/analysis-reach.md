# 可達量分析

可達量分析位於 `Analysis > Reach`，用於全源計算；`Analysis > Interactive Reach` 用於所選源線段計算。

## 米制可達量

路徑：`Analysis > Reach > Metric Reach`

Metric Reach 計算指定米制半徑內可達線段的總長度。半徑為 `500` 時，Shapefile 欄位名為 `R500`，CSV/TXT 欄位名為 `R_500`。

```{figure} ../_static/images/guide/4.33.png
:alt: Metric Reach 參數

Metric Reach 參數面板。
```

```{figure} ../_static/images/guide/4.34.png
:alt: Metric Reach 結果

Metric Reach 結果。
```

Metric Reach 還會生成平均米制距離欄位，例如 `mMD500`，因為同一搜尋過程可以同時計算可達長度和平均米制距離。

## 轉向可達量

路徑：`Analysis > Reach > Directional Reach`

Directional Reach 使用轉向次數和角度閾值。`Directional Change = 2` 且 `Angle Threshold = 20` 時，會生成類似 `R2d20a` 的欄位。

```{figure} ../_static/images/guide/4.35.png
:alt: Directional Reach 參數

Directional Reach 參數面板。
```

```{figure} ../_static/images/guide/4.36.png
:alt: Directional Reach 結果

Directional Reach 結果。
```

對於規則網格，角度閾值通常設定為 5 到 10 度即可。對於有機路網，15 到 30 度可能更合適。轉向次數 2 常用於識別主街與普通街道之間的差異。

## 交叉口可達量

路徑：`Analysis > Reach > Junction Reach`

Junction Reach 透過交叉口數量限制搜尋範圍。`Junctions = 5` 且 `Degree Thresholds = 4` 表示只把四岔路口計為交叉口，並生成類似 `R5j4x` 的欄位。

```{figure} ../_static/images/guide/4.37.png
:alt: Junction Reach 參數

Junction Reach 參數面板。
```

```{figure} ../_static/images/guide/4.38.png
:alt: Junction Reach 結果

Junction Reach 結果。
```

## 組合可達量

路徑：`Analysis > Reach > Combined Reach`

Combined Reach 同時應用米制半徑和轉向半徑。例如 `500 m`、`2` 次轉向、`20` 度表示線段必須在 500 米範圍內，並且方向變化不超過兩次。

```{figure} ../_static/images/guide/4.39.png
:alt: Combined Reach 參數

Combined Reach 參數面板。
```

```{figure} ../_static/images/guide/4.40.png
:alt: Combined Reach 結果

Combined Reach 結果。
```

## 選項

可達量選項包括交叉口計數和加權計算。

```{figure} ../_static/images/guide/4.41.png
:alt: Weighted Reach 參數

Weighted Reach 欄位選擇。
```

```{figure} ../_static/images/guide/4.42.png
:alt: Weighted Reach 結果

Weighted Reach 結果。
```

加權計算使用所選數值欄位作為被可達的數量。例如，按 `Shopcount` 加權的可達量估計的是可達商鋪相關活動量，而不僅是可達街道長度。

## 互動式可達量

Interactive Reach 從一個或多個所選源線段計算可達量。它會在資訊面板返回臨時文字摘要，並將可達子網路繪製為 `Reach by Selection` 圖層。

```{figure} ../_static/images/guide/4.43.png
:alt: Interactive Reach

Interactive Reach 參數和結果。
```
