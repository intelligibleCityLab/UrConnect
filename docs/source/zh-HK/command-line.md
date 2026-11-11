# 命令列介面（CLI）

本手冊適用於 **v0.2.5** 的 `urconnect-cli`。它是獨立、無圖形介面的 UrConnect 分析程式，執行時不需要 Qt、Python 或顯示伺服器。視覺化與點距離工具請參閱桌面版指南；本頁列出的命令是目前 CLI 提供的分析功能。

## 下載與執行

從 [CLI v0.2.5 發布頁](https://github.com/intelligibleCityLab/UrConnect/releases/tag/cli-v0.2.5) 下載與系統相符的檔案：

| 系統 | 發布檔案與程式 | 狀態 |
| --- | --- | --- |
| Windows x64 | `urconnect-cli-windows-x64.exe` | 獨立 CLI；亦可解壓 `UrConnect-CLI-0.2.5-windows-x64.zip`，使用其中的 `urconnect-cli.exe` |
| macOS ARM64（Apple Silicon） | `UrConnect-CLI-0.2.5-macos-arm64.tar.gz`；解壓 CLI 包後使用其中的 `urconnect-cli` | Experimental；未經 Apple 公證，不是 Intel 版本 |
| Linux x64 | `UrConnect-CLI-0.2.5-linux-x64.tar.gz`；解壓包內的 `urconnect-cli` | Experimental；在 Ubuntu 24.04 上測試，需要系統 C/C++ 執行階段函式庫 |

各系統使用各自的執行檔，不是同一個檔案跨系統執行。CLI 靜態連結 Shapelib；Linux 桌面版需要的 Qt/OpenGL 不屬於 CLI 的依賴。

在程式所在目錄執行：

```powershell
# Windows PowerShell
.\urconnect-cli-windows-x64.exe --version
.\urconnect-cli-windows-x64.exe reach "C:\data\network.shp" --radius 400,800
```

```bash
# macOS / Linux
./urconnect-cli --version
./urconnect-cli reach "/path/to/network.shp" --radius 400,800
./urconnect-cli --help
```

後續範例假設 `urconnect-cli` 已加入 `PATH`；否則請使用上面的程式路徑。含空格的路徑需要引號。macOS 的 Gatekeeper 可能阻止未公證的下載程式執行，這與分析或資料錯誤不同；不要關閉系統級安全保護。

## 如何選擇輸入及輸出路徑

程式路徑指定要執行的 CLI；命令後面的路徑指定輸入路網。相對路徑以終端機目前的工作目錄為準，不是以程式所在目錄為準。程式與資料不在同一資料夾時，建議使用絕對路徑。

目前不能單獨指定輸出目錄。要保留原始資料並把結果放到指定資料夾，應先複製**該資料集的全部同名檔案**，再對複製後的 `.shp` 執行。以下為每次執行建立一個新資料夾；請替換成自己的路徑。

```powershell
# Windows PowerShell：程式路徑、原始資料、結果資料夾
$cli = "C:\Tools\UrConnect\urconnect-cli-windows-x64.exe"
$source = "C:\Street Data\network"
$runDir = "C:\Results\reach-run-01"
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Copy-Item -Path "$source.*" -Destination $runDir
& $cli reach "$runDir\network.shp" --radius "400,800"
```

```bash
# macOS / Linux：程式與資料可放在不同目錄
cli="/path/to/UrConnect/urconnect-cli"
source_base="/path/to/Street Data/network"
run_dir="/path/to/Results/reach-run-01"
mkdir "$run_dir" && cp "$source_base".* "$run_dir/" && \
  "$cli" reach "$run_dir/network.shp" --radius 400,800
```

先建立上一層 `Results` 資料夾。結果寫入 `reach-run-01/network.dbf`；在 GIS 中開啟同目錄的 `network.shp`，即可查看 `R400`、`R800` 欄位。PowerShell 執行帶引號的程式路徑或儲存路徑的變數時，需要前面的 `&`。若使用 Windows ZIP 內的程式，把 `$cli` 改為其中的 `urconnect-cli.exe` 路徑。

`od --pairs` 的結果 CSV 寫在 **pairs CSV 所在目錄**。如需集中儲存結果，亦把 pairs CSV 複製到執行資料夾。各命令的具體輸出位置見下表。

## 輸入資料與備份

```{warning}
請在完整 Shapefile 資料集的副本上執行。分析會向輸入 `.dbf` 新增或更新欄位，重複執行可能覆寫同名結果欄位或檔案。目前沒有 `--output` 參數，也不會自動備份。不要讓多個命令同時寫入同一資料集。
```

- CLI **只接受 Shapefile 路網**，不接受桌面版支援的 CSV/TXT 路網輸入。同名 `.shp`、`.shx`、`.dbf` 必須放在一起；複製時也保留 `.prj` 等附屬檔案。
- 每條道路必須是**兩個頂點的線段**。執行前應把所有要素處理成此格式；不要依賴匯入器安全地略過不支援的幾何。請先在 GIS 中拆分、吸附與清理拓撲；幾何相交不會自動產生交叉口。
- 以米解釋半徑與距離時，應使用米制投影座標。CLI 直接使用輸入座標單位，不讀取 `.prj`、不驗證座標系、不進行重新投影，也不會對未投影輸入發出提示。
- 匯入前請在 GIS 中完成端點吸附。共享端點透過格式化為 10 位有效數字的 X/Y 座標字串匹配識別，沒有可設定的吸附容差。詳見[座標系與端點連接要求](getting-started.md)。
- `--from`、`--to` 與 OD CSV 中的道路 ID 是 **Shapefile 從 0 開始的記錄序號**，不是自訂 `ID`/`FID` 屬性值。重排資料會改變序號。
- 權重必須是 DBF 中已有的數值欄位，名稱需準確相符。輸入 DBF 與輸出目錄必須可寫，並關閉占用檔案的其他軟件。

## 命令一覽

語法：`urconnect-cli <command> <input.shp> [options]`。

| 命令 | 分析內容 | 必需參數 |
| --- | --- | --- |
| `reach` | 米制可達量 | `--radius` |
| `md` | 米制半徑內的平均米制距離 | `--radius` |
| `dr` | 轉向可達量 | `--angle`、`--turns` |
| `ddl` | 米制半徑內的轉向距離 | `--radius`、`--angle` |
| `mdr` | 米制與轉向組合可達量 | `--radius`、`--angle`、`--turns` |
| `jnr` | 交叉口可達量 | `--junction-degree`、`--junction-limit` |
| `jnd` | 米制半徑內的交叉口距離 | `--radius`、`--junction-degree` |
| `netreach` | 從起點道路出發的可達子路網 | `--from` 與對應 `--type` 的參數 |
| `stepdepth` | 起點道路到每條道路的距離 | `--from`；`dr` 另需 `--angle`，`jnr` 另需 `--junction-degree` |
| `od` | 道路間最短路徑 | `--from`（可選 `--to`）或 `--pairs`；`dr` 另需 `--angle`，`jnr` 另需 `--junction-turns` |

### 參數與掃描

| 參數 | 含義與取值 |
| --- | --- |
| `--radius` | 非負米制距離，可用逗號列表；`n` 表示無限制 |
| `--angle` | 非負轉向角度閾值，單位為度（通常 0–180），達到或超過此閾值時計一次轉向；`dr`、`ddl`、`mdr` 支援列表，起點類命令只接受一個值 |
| `--turns` | 最大轉向次數，非負整數列表或 `n` |
| `--junction-degree` | 一個非負整數，交叉口度數閾值；度數達到或超過閾值的節點計入交叉口距離 |
| `--junction-limit` | 最大計入交叉口數量，非負整數列表或 `n` |
| `--junction-turns` | 一個非負整數，附加交叉口計數規則的度數閾值；**不是**轉向次數或交叉口數量上限 |
| `--weight` | 數值屬性名稱列表，例如 `ShopCount,Population`；用於屬性加權，不是自訂最短路徑成本函數 |
| `--type` | `mr`（米制，預設）、`dr`（轉向）、`jnr`（交叉口）；僅用於 `netreach`、`stepdepth`、`od` |
| `--from`、`--to` | 道路 ID 的逗號列表 |
| `--pairs` | OD CSV 路徑，不能與 `--from` 或 `--to` 同時使用 |
| `--threads` | 全路網分析工作執行緒數目：正整數或 `auto`，預設 `1` |

列表中不要加空格：使用 `400,800`，不要寫成 `400, 800`。支援 `--key value` 與 `--key=value`，但不能重複傳入同一個參數。

所有命令均接受 `--weight`。`--junction-turns` 可用於 `reach`、`md`、`dr`、`mdr`、`netreach`、`od`。`netreach` 僅使用其類型對應的參數：`mr` 需要 `--radius`；`dr` 需要 `--angle` 與 `--turns`；`jnr` 需要 `--junction-degree` 與 `--junction-limit`。每次 `netreach` 使用一個半徑、轉向次數或交叉口數量上限；子路網匯出不是全路網參數掃描。

全路網命令支援參數列表掃描；`mdr` 會在每個角度下組合半徑與轉向次數：

```bash
urconnect-cli reach network.shp --radius 400,800 --weight ShopCount
urconnect-cli md network.shp --radius 400,800
urconnect-cli dr network.shp --angle 45,90 --turns 0,1,2
urconnect-cli ddl network.shp --radius 400,800 --angle 45
urconnect-cli mdr network.shp --radius 400,800 --angle 45 --turns 1,2
urconnect-cli jnr network.shp --junction-degree 3 --junction-limit 2,3
urconnect-cli jnd network.shp --radius 800 --junction-degree 3
```

`ShopCount` 為範例欄位，請替換成已有數值欄位。無限制米制可達量使用 `--radius n`。大路網建議先用較小的有限半徑試跑。

## 起點與 OD 路徑

請把範例 ID 替換為目前路網中的有效記錄序號：

```bash
urconnect-cli netreach network.shp --from 10 --type mr --radius 400
urconnect-cli netreach network.shp --from 10 --type dr --angle 45 --turns 2
urconnect-cli netreach network.shp --from 10 --type jnr --junction-degree 3 --junction-limit 2
urconnect-cli stepdepth network.shp --from 10,20 --type mr
urconnect-cli stepdepth network.shp --from 10 --type dr --angle 45
urconnect-cli stepdepth network.shp --from 10 --type jnr --junction-degree 3
urconnect-cli od network.shp --from 10 --to 500 --type mr
urconnect-cli od network.shp --from 10 --to 500 --type dr --angle 45
urconnect-cli od network.shp --from 10 --to 500 --type jnr --junction-turns 3
```

`stepdepth` 指定多個起點時，記錄這些起點中的最小距離。`od` 省略 `--to` 時計算起點到路網的路徑，可能產生大量輸出；手工起終點列表計算其組合。需要明確逐對配對時使用 OD CSV。

### 批次 OD CSV

使用 UTF-8 CSV，保留表頭，後續兩欄填寫不帶引號的整數：

```text
origin,destination
10,500
20,500
```

第一行作為表頭；之後每個非空行的前兩欄依次為起點與終點。不要省略表頭，也不要使用自訂屬性 ID。

```bash
urconnect-cli od network.shp --pairs pairs.csv --type mr
urconnect-cli od network.shp --pairs pairs.csv --type dr --angle 45
urconnect-cli od network.shp --pairs pairs.csv --type jnr --junction-turns 3
```

批次模式在 CSV 同目錄產生 `pairs_MR.csv`、`pairs_DR.csv` 或 `pairs_JnR.csv`，並在輸入 DBF 中寫入 `PathCount`，**不匯出**手工模式的路徑 Shapefile。結果欄為 `origin,destination,distance,path`；帶引號的 `path` 是不含起終點道路的中間道路 ID 列表。`--type` 決定選路準則。三種類型的 CSV `distance` 欄都表示選中路徑的幾何長度，單位與輸入座標一致，不是轉向次數或交叉口數量。

## 輸出位置

| 工作流程 | 輸出 |
| --- | --- |
| 全路網分析 | 輸入 DBF 的結果欄位，例如 `R400`、`R800`、`mMD400`、`R1d45a`、`R1j3x` |
| `netreach` | 路網同目錄的 `Reach_*` Shapefile 資料集 |
| `stepdepth` | 輸入 DBF 中的 `StepD_mr`、`StepD_dr` 或 `StepD_jnr`；不可達道路為 `-1` |
| 手工 `od` | 路網同目錄的 `Path_*` Shapefile；多條路徑可產生子目錄 |
| `od --pairs` | OD CSV 同目錄的結果 CSV，以及輸入 DBF 的 `PathCount` |

DBF 欄位名稱最多 10 個字元，較長的參數或權重名稱可能縮短並衝突。請使用較短名稱，可能衝突的掃描使用不同資料副本。參見[輸出欄位](output-fields.md)中的桌面版/分析引擎命名規則。CLI 不匯出地圖圖片，也沒有輸出目錄或座標轉換參數。

## 如何選擇執行緒數目

**同一個 CLI 程式支援單執行緒及多執行緒。** 使用 `--threads 1`、`--threads 8` 或 `--threads auto`；預設是 `1`。此參數適用於七種全路網命令：`reach`、`md`、`dr`、`ddl`、`mdr`、`jnr`、`jnd`；`netreach`、`stepdepth`、`od` 不接受此參數。

```bash
urconnect-cli mdr network.shp --radius 250 --angle 45 --turns 2 --threads 8
urconnect-cli reach network.shp --radius 400,800 --threads auto
```

實際工作執行緒數目不超過可用 CPU 並行數目及起點道路數目，終端機會顯示 `Analysis workers: N`。各執行緒處理各自的起點道路，全部計算完成後再依序寫入 DBF；參數掃描產生的結果欄位不變。執行緒越多可能佔用越多記憶體，大路網建議從適中的數目開始。不要同時執行多個 CLI 程序寫入同一資料集。

CLI **v0.2.5** 已支援此參數；桌面版 **v0.2.4** 安裝包附帶的舊 CLI 不支援，請下載上面的新 CLI 包。桌面程式仍為 v0.2.4；不需要分別下載單執行緒及多執行緒程式。路網載入與角度預處理可能獨立使用執行緒。原始碼建置範例中的 `-j 4` 只控制編譯工作數目，不控制分析執行緒。

全路網 `mdr` 保留 v0.2.4 引入的 Grasshopper 流程：先選出轉向可達道路子集，再在子集內計算米制可達量。CLI 的 `netreach` 只提供 `mr`、`dr`、`jnr`，尚未提供桌面版的組合 directional-metric interactive reach。

## 只構建 CLI

需要 Git、CMake 3.15+、C 與 C++11 編譯器以及 Boost 標頭。預設啟用的測試亦需要 Python 3；若只建置程式、不執行測試，可加 `-DBUILD_TESTING=OFF`。首次 CMake 設定需要連線取得 spdlog。Shapelib 原始碼已包含；只構建 CLI 不需要 Qt/OpenGL。

從發布頁下載原始碼壓縮檔，解壓後在原始碼根目錄開啟終端機；或複製指定版本的儲存庫：

```bash
git clone --branch cli-v0.2.5 --depth 1 https://github.com/intelligibleCityLab/UrConnect.git
cd UrConnect
```

```powershell
# Windows：先安裝 Visual Studio 2022 C++ 工具與 Boost
cmake -S . -B build-cli -G "Visual Studio 17 2022" -A x64 `
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build-cli --config Release --target urconnect-cli
.\build-cli\bin\Release\urconnect-cli.exe --version
```

```bash
# macOS：先安裝 Xcode Command Line Tools
brew install cmake boost python
cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release \
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build-cli --target urconnect-cli -j 4
./build-cli/bin/Release/urconnect-cli --version
```

```bash
# Linux（Ubuntu）
sudo apt-get install python3 git build-essential cmake libboost-dev
cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release \
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT=/usr/include
cmake --build build-cli --target urconnect-cli -j 4
./build-cli/bin/Release/urconnect-cli --version
```

執行產生路網測試前，先構建全部 CLI 目標：

```bash
cmake --build build-cli --config Release -j 4
ctest --test-dir build-cli -C Release --output-on-failure
```

## 錯誤與可重現性

- 結束碼 `0` 為成功，`1` 為命令/參數錯誤，`2` 為輸入載入錯誤或捕獲的分析例外。也要檢查 stderr 中的訊息。
- `road ID ... is outside the input network`：檢查從 0 開始的記錄序號，以及要素是否為兩個頂點。
- `weight attribute not found or not numeric`：核對 DBF 欄位名稱與數值類型。
- 缺少 `.shx`/`.dbf` 或 DBF 不可寫：恢復附屬檔案、寫入權限並解除檔案占用。
- 找不到命令：使用 `./urconnect-cli` 或 `.\urconnect-cli-windows-x64.exe`，或將程式目錄加入 `PATH`。
- 程式格式不相符：檢查系統與 CPU 架構；Linux 執行階段函式庫不相容時，可在目標系統上從原始碼構建。

可重現分析應記錄 `--version`、完整命令、輸入檔案校驗和、座標系/單位以及數值權重定義。保留原始輸入，並為每次執行使用獨立的工作資料與輸出。
