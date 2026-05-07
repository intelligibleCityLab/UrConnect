# 可视化与文件管理

## 可视化

```{figure} ../_static/images/guide/4.56.png
:alt: 可视化面板

可视化面板。
```

Visualization 模块控制背景颜色、数值色谱范围、图层可见性、线色和线宽。

Color Range
: 改变当前数值属性的色带。

Layers
: 开关 `Reach by Selection` 和 `Path Analysis` 输出图层。

Lines Edit
: 应用线色或线宽设置。线宽可以按所选数值属性筛选。

```{figure} ../_static/images/guide/4.57.png
:alt: 色谱范围设置

色谱范围设置。
```

```{figure} ../_static/images/guide/4.58.png
:alt: 线宽设置

线宽设置。
```

使用 `Reset` 可恢复默认可视化状态。

## 文件管理

Open
: 打开 Shapefile、CSV 或 TXT 线段网络数据。

Export Analysis
: 将 `Interactive Reach` 或 `Path Analysis` 生成的图层导出为 Shapefile。

Export Screen
: 将当前视图导出为 SVG 或 PNG。

Export Attributes
: 将所选数值属性和线段 ID 导出为 TXT 或 CSV。

对于 Shapefile，字段名限制为 10 个字符。CSV 和 TXT 输出会尽可能使用较长字段名。
