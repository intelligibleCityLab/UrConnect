# 使用指南

## 介面

左側為工具、文件、分析、路徑分析、視覺化和執行控制；中間為地圖視窗；右側為數值屬性欄。

## 可達量分析

`Analysis > Reach` 提供全源計算，`Analysis > Interactive Reach` 針對所選線段計算。

- `Metric Reach` 計算指定米制半徑內可達線段總長度。
- `Directional Reach` 使用轉向次數和角度閾值。
- `Junction Reach` 使用交叉口數量和交叉口度數。
- `Combined Reach` 同時限制米制半徑和轉向半徑。
- `Options` 可計算交叉口數量或使用數值欄位加權。

```{figure} ../_static/images/guide/4.40.png
:alt: 組合可達量結果

組合可達量結果示例。
```

## 距離分析

距離分析衡量從源線段到可達網路部分的平均或總距離。

- `Directional Distance` 輸出平均轉向距離 `D` 和按線長加權的 `DL`。
- `Junction Distance` 以交叉口數量替代轉向次數。
- `Point Distance` 計算所選線段到其他所有線段的距離。

## 路徑分析

`Path Analysis` 用於模擬起點線段到終點線段之間的最短路徑。可選擇米制最短、轉向最少或交叉口最少三種路徑模式。少量 OD 可手工輸入線段 ID，大量 OD 建議使用兩欄 CSV/TXT 矩陣。

```{figure} ../_static/images/guide/4.55.png
:alt: 路徑模擬結果

路徑模擬結果示例。
```
