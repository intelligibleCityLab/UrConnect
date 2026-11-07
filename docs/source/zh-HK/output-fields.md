# 輸出欄位

UrConnect 根據分析類型和參數值命名輸出欄位。Shapefile 欄位名會縮短，以滿足 DBF 10 字元欄位名限制。

| 分析 | 規則 | 示例 | CSV/TXT 欄位 | Shapefile 欄位 |
| --- | --- | --- | --- | --- |
| Metric Reach | `R_m`，以及平均米制距離 `mMD_m` | `m = 500` | `R_500`, `mMD_500` | `R500`, `mMD500` |
| Directional Reach | `R_da` | `d = 2`, `a = 20` | `R_2d20a` | `R_2d20a` |
| Combined Reach | `R_da_m` | `d = 2`, `a = 20`, `m = 500` | `R_2d20a_500` | `R2d20a500` |
| Junction Reach | `R_jx` | `j = 5`, `x = 4` | `R_5j4x` | `R5j4x` |
| Directional Distance | `D_a_m`，以及 `DL_a_m` | `a = 20`, `m = 500` | `D_20a_500`, `DL_20a_500` | `D20a500`, `DL20a500` |
| Junction Distance | `D_x_m` | `x = 4`, `m = 500` | `D_4x_500` | `D4x500` |
| 交叉口選項 | 後綴 `_C_x` | `x = 4` | `R_500_C_4x` | `R500C4x` |
| 加權選項 | 後綴 `_W_FieldName` | `FieldName = Shop` | `R_500_W_Shop` | `R500WShop` |
| Metric Point Distance | `PD_id` | `id = 123` | `PD_123` | `PD123` |
| Directional Point Distance | `PD_a_id` | `id = 123`, `a = 20` | `PD_20a_123` | `PD20a123` |
| Junction Point Distance | `PD_x_id` | `id = 123`, `x = 4` | `PD_4x_123` | `PD4x123` |

說明：

- `mMD` 表示平均米制距離。
- `DL` 表示按線段長度加權的轉向距離。
- `Junction Distance` 不適用 `Compute Junctions` 選項。
- 啟用選項時，Shapefile 欄位可能會省略角度閾值等細節，以符合 DBF 欄位名長度限制。
