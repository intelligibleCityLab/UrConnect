# 用户指南

## 界面

左侧为工具、文件、分析、路径分析、可视化和运行控制；中间为地图窗口；右侧为数值属性栏。

## 可达量分析

`Analysis > Reach` 提供全源计算，`Analysis > Interactive Reach` 针对所选线段计算。

- `Metric Reach` 计算指定米制半径内可达线段总长度。
- `Directional Reach` 使用转向次数和角度阈值。
- `Junction Reach` 使用交叉口数量和交叉口度数。
- `Combined Reach` 同时限制米制半径和转向半径。
- `Options` 可计算交叉口数量或使用数值字段加权。

```{figure} ../_static/images/guide/4.40.png
:alt: 组合可达量结果

组合可达量结果示例。
```

## 距离分析

距离分析衡量从源线段到可达网络部分的平均或总距离。

- `Directional Distance` 输出平均转向距离 `D` 和按线长加权的 `DL`。
- `Junction Distance` 以交叉口数量替代转向次数。
- `Point Distance` 计算所选线段到其他所有线段的距离。

```{figure} ../_static/images/guide/4.46.png
:alt: 转向距离结果

转向距离结果示例。
```

## 路径分析

`Path Analysis` 用于模拟起点线段到终点线段之间的最短路径。可选择米制最短、转向最少或交叉口最少三种路径模式。少量 OD 可手工输入线段 ID，大量 OD 建议使用两列 CSV/TXT 矩阵。

```{figure} ../_static/images/guide/4.55.png
:alt: 路径模拟结果

路径模拟结果示例。
```

## 可视化与文件

`Visualization` 控制背景、色谱、图层、线色和线宽。`File` 面板可打开数据、导出分析图层、导出屏幕和导出属性。
