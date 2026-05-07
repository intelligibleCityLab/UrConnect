# 路径分析

Path Analysis 模拟起点线段和终点线段之间的最短路径。它独立于 `Analysis` 模块，支持手工输入起终点，也支持 OD 表输入。

## 路径类型

```{figure} ../_static/images/guide/4.53.png
:alt: 路径分析模式

米制、转向和交叉口路径模式。
```

Metric
: 按米制长度计算最短路径。不需要额外参数。

Directional
: 按方向变化计算最短路径。需要设置 `Angle Threshold`。

Junction
: 按交叉口数量计算最短路径。需要设置 `Junction Degree`。

## 可选路径属性

```{figure} ../_static/images/guide/4.54.png
:alt: 路径分析选项

路径选项面板。
```

选项可以计算：

- 累积方向变化
- 累积交叉口数量
- 所选数值字段的累积加权属性

请保持选项参数与所选路径模式一致，避免解释含混。

## 起点和终点输入

手工输入
: 在 `From ID` 和 `TO ID` 中输入线段 ID，或在地图中选择线段并将其 ID 转入输入框。

OD 表输入
: 使用两列 CSV 或 TXT 文件。第一列为起点线段 ID，第二列为终点线段 ID。不要求列名。

## 结果

```{figure} ../_static/images/guide/4.55.png
:alt: 路径结果

手工路径分析结果。
```

手工输入会返回临时摘要数据，并绘制 `Path Analysis` 图层。OD 表输入不会绘制每一条路径，而是将 `Path count` 写入网络，并创建包含每组 OD 距离和路径构成的 CSV。
