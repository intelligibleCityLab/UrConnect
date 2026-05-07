# Output Fields

UrConnect names output fields from the analysis type and parameter values. Shapefile field names are shortened to respect the 10-character DBF field limit.

```{list-table} Field naming rules
:header-rows: 1
:class: field-table

* - Analysis
  - Rule
  - Example
  - CSV/TXT field
  - Shapefile field
* - Metric Reach
  - `R_m`, plus `mMD_m` for mean metric distance
  - `m = 500`
  - `R_500`, `mMD_500`
  - `R500`, `mMD500`
* - Directional Reach
  - `R_da`
  - `d = 2`, `a = 20`
  - `R_2d20a`
  - `R_2d20a`
* - Combined Reach
  - `R_da_m`
  - `d = 2`, `a = 20`, `m = 500`
  - `R_2d20a_500`
  - `R2d20a500`
* - Junction Reach
  - `R_jx`
  - `j = 5`, `x = 4`
  - `R_5j4x`
  - `R5j4x`
* - Directional Distance
  - `D_a_m`, plus `DL_a_m`
  - `a = 20`, `m = 500`
  - `D_20a_500`, `DL_20a_500`
  - `D20a500`, `DL20a500`
* - Junction Distance
  - `D_x_m`
  - `x = 4`, `m = 500`
  - `D_4x_500`
  - `D4x500`
* - Junction option
  - suffix `_C_x`
  - `x = 4`
  - `R_500_C_4x`
  - `R500C4x`
* - Weighting option
  - suffix `_W_FieldName`
  - `FieldName = Shop`
  - `R_500_W_Shop`
  - `R500WShop`
* - Metric Point Distance
  - `PD_id`
  - `id = 123`
  - `PD_123`
  - `PD123`
* - Directional Point Distance
  - `PD_a_id`
  - `id = 123`, `a = 20`
  - `PD_20a_123`
  - `PD20a123`
* - Junction Point Distance
  - `PD_x_id`
  - `id = 123`, `x = 4`
  - `PD_4x_123`
  - `PD4x123`
```

Notes:

- `mMD` means mean metric distance.
- `DL` means directional distance per segment length.
- Junction Distance does not use the `Compute Junctions` option.
- When options are enabled, Shapefile fields may omit angle-threshold detail to fit the DBF field limit.
