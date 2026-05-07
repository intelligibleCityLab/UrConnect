# 输出字段

UrConnect 根据分析类型和参数自动命名输出字段。Shapefile 字段受 DBF 10 字符限制，CSV/TXT 会保留更完整的字段名。

| 分析 | 规则 | 示例 | CSV/TXT | Shapefile |
| --- | --- | --- | --- | --- |
| Metric Reach | `R_m`, `mMD_m` | `m=500` | `R_500`, `mMD_500` | `R500`, `mMD500` |
| Directional Reach | `R_da` | `d=2`, `a=20` | `R_2d20a` | `R_2d20a` |
| Combined Reach | `R_da_m` | `d=2`, `a=20`, `m=500` | `R_2d20a_500` | `R2d20a500` |
| Junction Reach | `R_jx` | `j=5`, `x=4` | `R_5j4x` | `R5j4x` |
| Directional Distance | `D_a_m`, `DL_a_m` | `a=20`, `m=500` | `D_20a_500`, `DL_20a_500` | `D20a500`, `DL20a500` |
| Junction Distance | `D_x_m` | `x=4`, `m=500` | `D_4x_500` | `D4x500` |
| Weighting | `_W_FieldName` | `Shop` | `R_500_W_Shop` | `R500WShop` |
| Point Distance | `PD_id` | `id=123` | `PD_123` | `PD123` |

说明：

- `mMD` 表示平均米制距离。
- `DL` 表示按线段长度加权的转向距离。
- `Junction Distance` 不适用 `Compute Junctions` 选项。
