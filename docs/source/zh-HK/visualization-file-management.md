# 視覺化與文件管理

## 視覺化

```{figure} ../_static/images/guide/4.56.png
:alt: 視覺化面板

視覺化面板。
```

Visualization 模組控制背景顏色、數值色譜範圍、圖層可見性、線色和線寬。

Color Range
: 改變當前數值屬性的色帶。

Layers
: 開關 `Reach by Selection` 和 `Path Analysis` 輸出圖層。

Lines Edit
: 應用線色或線寬設定。線寬可以按所選數值屬性篩選。

```{figure} ../_static/images/guide/4.57.png
:alt: 色譜範圍設定

色譜範圍設定。
```

```{figure} ../_static/images/guide/4.58.png
:alt: 線寬設定

線寬設定。
```

使用 `Reset` 可恢復預設視覺化狀態。

## 文件管理

Open
: 開啟 Shapefile、CSV 或 TXT 線段網路資料。

Export Analysis
: 將 `Interactive Reach` 或 `Path Analysis` 生成的圖層匯出為 Shapefile。

Export Screen
: 將當前視圖匯出為 SVG 或 PNG。

Export Attributes
: 將所選數值屬性和線段 ID 匯出為 TXT 或 CSV。

對於 Shapefile，欄位名限制為 10 個字元。CSV 和 TXT 輸出會盡可能使用較長欄位名。
