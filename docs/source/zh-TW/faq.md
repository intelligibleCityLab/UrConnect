# 常見問題

## UrConnect 只能在 Windows 上執行嗎？

不是。工程已有 Windows、macOS、Linux 構建分支。Windows 最成熟；macOS 已在本機用 Qt 5 和 Boost 進入編譯；Linux 需要在 CI 中繼續驗證。

## 軟體能編輯街道線網嗎？

不能。請先在 GIS 或繪圖軟體中完成線段模型繪製和拓撲清理。

## 應該使用 Shapefile 還是 CSV/TXT？

需要 GIS 相容時使用 Shapefile；需要保留較長欄位名時使用 CSV/TXT，因為 Shapefile 欄位名最長只有 10 個字元。
