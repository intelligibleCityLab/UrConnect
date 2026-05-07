# 路徑分析

Path Analysis 模擬起點線段和終點線段之間的最短路徑。它獨立於 `Analysis` 模組，支援手工輸入起終點，也支援 OD 表輸入。

## 路徑類型

```{figure} ../_static/images/guide/4.53.png
:alt: 路徑分析模式

米制、轉向和交叉口路徑模式。
```

Metric
: 按米制長度計算最短路徑。不需要額外參數。

Directional
: 按方向變化計算最短路徑。需要設定 `Angle Threshold`。

Junction
: 按交叉口數量計算最短路徑。需要設定 `Junction Degree`。

## 可選路徑屬性

```{figure} ../_static/images/guide/4.54.png
:alt: 路徑分析選項

路徑選項面板。
```

選項可以計算：

- 累積方向變化
- 累積交叉口數量
- 所選數值欄位的累積加權屬性

請保持選項參數與所選路徑模式一致，避免解釋含混。

## 起點和終點輸入

手工輸入
: 在 `From ID` 和 `TO ID` 中輸入線段 ID，或在地圖中選擇線段並將其 ID 轉入輸入框。

OD 表輸入
: 使用兩欄 CSV 或 TXT 文件。第一欄為起點線段 ID，第二欄為終點線段 ID。不要求欄名。

## 結果

```{figure} ../_static/images/guide/4.55.png
:alt: 路徑結果

手工路徑分析結果。
```

手工輸入會返回臨時摘要資料，並繪製 `Path Analysis` 圖層。OD 表輸入不會繪製每一條路徑，而是將 `Path count` 寫入網路，並建立包含每組 OD 距離和路徑構成的 CSV。
