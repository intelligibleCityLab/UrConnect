# 常见问题

## UrConnect 只能在 Windows 上运行吗？

不是。工程已有 Windows、macOS、Linux 构建分支。Windows 最成熟；macOS 已在本机用 Qt 5 和 Boost 进入编译；Linux 需要在 CI 中继续验证。

## 软件能编辑街道线网吗？

不能。请先在 GIS 或绘图软件中完成线段模型绘制和拓扑清理。

## 应该使用 Shapefile 还是 CSV/TXT？

需要 GIS 兼容时使用 Shapefile；需要保留较长字段名时使用 CSV/TXT，因为 Shapefile 字段名最长只有 10 个字符。

## 为什么属性栏不显示文本字段？

属性栏主要用于分析和可视化，因此只显示数值型字段。
