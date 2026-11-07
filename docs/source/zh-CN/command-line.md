# 命令行界面（CLI）

本手册适用于 **v0.2.5** 的 `urconnect-cli`。它是独立、无图形界面的 UrConnect 分析程序，运行时不需要 Qt、Python 或显示服务器。可视化和点距离工具请参阅桌面版指南；本页列出的命令是当前 CLI 提供的分析功能。

## 下载与运行

从 [CLI v0.2.5 发布页](https://github.com/intelligibleCityLab/UrConnect/releases/tag/cli-v0.2.5) 下载与系统匹配的文件：

| 系统 | 发布文件与程序 | 状态 |
| --- | --- | --- |
| Windows x64 | `urconnect-cli-windows-x64.exe` | 独立 CLI；也可解压 `UrConnect-CLI-0.2.5-windows-x64.zip`，使用其中的 `urconnect-cli.exe` |
| macOS ARM64（Apple Silicon） | `UrConnect-CLI-0.2.5-macos-arm64.tar.gz`；解压 CLI 包后使用其中的 `urconnect-cli` | Experimental；未经 Apple 公证，不是 Intel 版本 |
| Linux x64 | `UrConnect-CLI-0.2.5-linux-x64.tar.gz`；解压包内的 `urconnect-cli` | Experimental；在 Ubuntu 24.04 上测试，需要系统 C/C++ 运行库 |

各系统使用各自的可执行文件，不是同一个文件跨系统运行。CLI 静态链接 Shapelib；Linux 桌面版需要的 Qt/OpenGL 不属于 CLI 的依赖。

在程序所在目录运行：

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

后续示例假设 `urconnect-cli` 已加入 `PATH`；否则请使用上面的程序路径。含空格的路径需要引号。macOS 的 Gatekeeper 可能阻止未公证的下载程序运行，这与分析或数据错误不同；不要关闭系统级安全保护。

## 如何选择输入和输出路径

程序路径指定要运行的 CLI；命令后面的路径指定输入路网。相对路径以终端当前工作目录为基准，不是以程序所在目录为基准。程序和数据不在同一文件夹时，建议使用绝对路径。

目前不能单独指定输出目录。要保留原始数据并把结果放到指定文件夹，应先复制**该数据集的全部同名文件**，再对复制后的 `.shp` 运行。下面为每次运行新建一个文件夹；请替换成自己的路径。

```powershell
# Windows PowerShell：程序路径、原始数据、结果文件夹
$cli = "C:\Tools\UrConnect\urconnect-cli-windows-x64.exe"
$source = "C:\Street Data\network"
$runDir = "C:\Results\reach-run-01"
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Copy-Item -Path "$source.*" -Destination $runDir
& $cli reach "$runDir\network.shp" --radius "400,800"
```

```bash
# macOS / Linux：程序和数据可放在不同目录
cli="/path/to/UrConnect/urconnect-cli"
source_base="/path/to/Street Data/network"
run_dir="/path/to/Results/reach-run-01"
mkdir "$run_dir" && cp "$source_base".* "$run_dir/" && \
  "$cli" reach "$run_dir/network.shp" --radius 400,800
```

先建立上一级 `Results` 文件夹。结果写入 `reach-run-01/network.dbf`；在 GIS 中打开同目录的 `network.shp`，即可查看 `R400`、`R800` 字段。PowerShell 运行带引号的程序路径或保存路径的变量时，需要前面的 `&`。若使用 Windows ZIP 内的程序，把 `$cli` 改为其中的 `urconnect-cli.exe` 路径。

`od --pairs` 的结果 CSV 写在 **pairs CSV 所在目录**。如需集中保存结果，也把 pairs CSV 复制到运行文件夹。各命令的具体输出位置见下表。

## 输入数据与备份

```{warning}
请在完整 Shapefile 数据集的副本上运行。分析会向输入 `.dbf` 添加或更新字段，重复运行可能覆盖同名结果字段或文件。目前没有 `--output` 参数，也不会自动备份。不要让多个命令同时写入同一数据集。
```

- CLI **只接受 Shapefile 路网**，不接受桌面版支持的 CSV/TXT 路网输入。同名 `.shp`、`.shx`、`.dbf` 必须放在一起；复制时也保留 `.prj` 等附属文件。
- 每条道路必须是**两个顶点的线段**。运行前应把所有要素处理成此格式；不要依赖导入器安全地跳过不支持的几何。请先在 GIS 中拆分、吸附和清理拓扑；几何相交不会自动生成交叉口。
- 以米解释半径和距离时，应使用米制投影坐标。CLI 不进行重投影，米制计算直接使用输入坐标单位。
- `--from`、`--to` 和 OD CSV 中的道路 ID 是 **Shapefile 从 0 开始的记录序号**，不是自定义 `ID`/`FID` 属性值。重排数据会改变序号。
- 权重必须是 DBF 中已有的数值字段，名称需准确匹配。输入 DBF 和输出目录必须可写，并关闭占用文件的其他软件。

## 命令一览

语法：`urconnect-cli <command> <input.shp> [options]`。

| 命令 | 分析内容 | 必需参数 |
| --- | --- | --- |
| `reach` | 米制可达量 | `--radius` |
| `md` | 米制半径内的平均米制距离 | `--radius` |
| `dr` | 转向可达量 | `--angle`、`--turns` |
| `ddl` | 米制半径内的转向距离 | `--radius`、`--angle` |
| `mdr` | 米制与转向组合可达量 | `--radius`、`--angle`、`--turns` |
| `jnr` | 交叉口可达量 | `--junction-degree`、`--junction-limit` |
| `jnd` | 米制半径内的交叉口距离 | `--radius`、`--junction-degree` |
| `netreach` | 从起点道路出发的可达子网络 | `--from` 和对应 `--type` 的参数 |
| `stepdepth` | 起点道路到每条道路的距离 | `--from`；`dr` 另需 `--angle`，`jnr` 另需 `--junction-degree` |
| `od` | 道路间最短路径 | `--from`（可选 `--to`）或 `--pairs`；`dr` 另需 `--angle`，`jnr` 另需 `--junction-turns` |

### 参数与扫描

| 参数 | 含义与取值 |
| --- | --- |
| `--radius` | 非负米制距离，可用逗号列表；`n` 表示无限制 |
| `--angle` | 非负转向角度阈值，单位为度（通常 0–180），达到或超过此阈值时记一次转向；`dr`、`ddl`、`mdr` 支持列表，起点类命令只接受一个值 |
| `--turns` | 最大转向次数，非负整数列表或 `n` |
| `--junction-degree` | 一个非负整数，交叉口度数阈值；度数达到或超过阈值的节点计入交叉口距离 |
| `--junction-limit` | 最大计入交叉口数量，非负整数列表或 `n` |
| `--junction-turns` | 一个非负整数，附加交叉口计数规则的度数阈值；**不是**转向次数或交叉口数量上限 |
| `--weight` | 数值属性名称列表，例如 `ShopCount,Population`；用于属性加权，不是自定义最短路径成本函数 |
| `--type` | `mr`（米制，默认）、`dr`（转向）、`jnr`（交叉口）；仅用于 `netreach`、`stepdepth`、`od` |
| `--from`、`--to` | 道路 ID 的逗号列表 |
| `--pairs` | OD CSV 路径，不能与 `--from` 或 `--to` 同时使用 |
| `--threads` | 全路网分析工作线程数：正整数或 `auto`，默认 `1` |

列表中不要加空格：使用 `400,800`，不要写成 `400, 800`。支持 `--key value` 和 `--key=value`，但不能重复传入同一个参数。

所有命令均接受 `--weight`。`--junction-turns` 可用于 `reach`、`md`、`dr`、`mdr`、`netreach`、`od`。`netreach` 仅使用其类型对应的参数：`mr` 需要 `--radius`；`dr` 需要 `--angle` 和 `--turns`；`jnr` 需要 `--junction-degree` 和 `--junction-limit`。每次 `netreach` 使用一个半径、转向次数或交叉口数量上限；子网络导出不是全路网参数扫描。

全路网命令支持参数列表扫描；`mdr` 会在每个角度下组合半径和转向次数：

```bash
urconnect-cli reach network.shp --radius 400,800 --weight ShopCount
urconnect-cli md network.shp --radius 400,800
urconnect-cli dr network.shp --angle 45,90 --turns 0,1,2
urconnect-cli ddl network.shp --radius 400,800 --angle 45
urconnect-cli mdr network.shp --radius 400,800 --angle 45 --turns 1,2
urconnect-cli jnr network.shp --junction-degree 3 --junction-limit 2,3
urconnect-cli jnd network.shp --radius 800 --junction-degree 3
```

`ShopCount` 为示例字段，请替换成已有数值字段。无限制米制可达量使用 `--radius n`。大路网建议先用较小的有限半径试跑。

## 起点与 OD 路径

请把示例 ID 替换为当前路网中的有效记录序号：

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

`stepdepth` 指定多个起点时，记录这些起点中的最小距离。`od` 省略 `--to` 时计算起点到路网的路径，可能产生大量输出；手工起终点列表计算其组合。需要明确逐对匹配时使用 OD CSV。

### 批量 OD CSV

使用 UTF-8 CSV，保留表头，后续两列填写不带引号的整数：

```text
origin,destination
10,500
20,500
```

第一行作为表头；之后每个非空行的前两列依次为起点和终点。不要省略表头，也不要使用自定义属性 ID。

```bash
urconnect-cli od network.shp --pairs pairs.csv --type mr
urconnect-cli od network.shp --pairs pairs.csv --type dr --angle 45
urconnect-cli od network.shp --pairs pairs.csv --type jnr --junction-turns 3
```

批量模式在 CSV 同目录生成 `pairs_MR.csv`、`pairs_DR.csv` 或 `pairs_JnR.csv`，并在输入 DBF 中写入 `PathCount`，**不导出**手工模式的路径 Shapefile。结果列为 `origin,destination,distance,path`；带引号的 `path` 是不含起终点道路的中间道路 ID 列表。`--type` 决定选路准则。三种类型的 CSV `distance` 列都表示选中路径的几何长度，单位与输入坐标一致，不是转向次数或交叉口数量。

## 输出位置

| 工作流 | 输出 |
| --- | --- |
| 全路网分析 | 输入 DBF 的结果字段，例如 `R400`、`R800`、`mMD400`、`R1d45a`、`R1j3x` |
| `netreach` | 路网同目录的 `Reach_*` Shapefile 数据集 |
| `stepdepth` | 输入 DBF 中的 `StepD_mr`、`StepD_dr` 或 `StepD_jnr`；不可达道路为 `-1` |
| 手工 `od` | 路网同目录的 `Path_*` Shapefile；多条路径可生成子目录 |
| `od --pairs` | OD CSV 同目录的结果 CSV，以及输入 DBF 的 `PathCount` |

DBF 字段名最多 10 个字符，较长的参数或权重名称可能缩短并冲突。请使用较短名称，可能冲突的扫描使用不同数据副本。参见[输出字段](output-fields.md)中的桌面版/分析引擎命名规则。CLI 不导出地图图片，也没有输出目录或坐标转换参数。

## 如何选择线程数

**同一个 CLI 程序支持单线程和多线程。** 使用 `--threads 1`、`--threads 8` 或 `--threads auto`；默认是 `1`。此参数适用于七种全路网命令：`reach`、`md`、`dr`、`ddl`、`mdr`、`jnr`、`jnd`；`netreach`、`stepdepth`、`od` 不接受此参数。

```bash
urconnect-cli mdr network.shp --radius 250 --angle 45 --turns 2 --threads 8
urconnect-cli reach network.shp --radius 400,800 --threads auto
```

实际工作线程数不超过可用 CPU 并发数和起点道路数，终端会显示 `Analysis workers: N`。各线程处理各自的起点道路，全部计算完成后再串行写入 DBF；参数扫描产生的结果字段不变。线程越多可能占用越多内存，大路网建议从适中的线程数开始。不要同时运行多个 CLI 进程写入同一数据集。

CLI **v0.2.5** 已支持此参数；桌面版 **v0.2.4** 安装包附带的旧 CLI 不支持，请下载上面的新 CLI 包。桌面程序仍为 v0.2.4；不需要分别下载单线程和多线程程序。路网加载和角度预处理可能独立使用线程。源码构建示例中的 `-j 4` 只控制编译任务数，不控制分析线程。

全路网 `mdr` 保留 v0.2.4 引入的 Grasshopper 流程：先选出转向可达道路子集，再在子集内计算米制可达量。CLI 的 `netreach` 只提供 `mr`、`dr`、`jnr`，尚未提供桌面版的组合 directional-metric interactive reach。

## 只构建 CLI

需要 Git、CMake 3.15+、C 和 C++11 编译器以及 Boost 头文件。默认启用的测试还需要 Python 3；若只构建程序、不运行测试，可加 `-DBUILD_TESTING=OFF`。首次 CMake 配置需要联网获取 spdlog。Shapelib 源码已包含；只构建 CLI 不需要 Qt/OpenGL。

从发布页下载源码压缩包，解压后在源码根目录打开终端；或克隆指定版本：

```bash
git clone --branch cli-v0.2.5 --depth 1 https://github.com/intelligibleCityLab/UrConnect.git
cd UrConnect
```

```powershell
# Windows：先安装 Visual Studio 2022 C++ 工具与 Boost
cmake -S . -B build-cli -G "Visual Studio 17 2022" -A x64 `
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build-cli --config Release --target urconnect-cli
.\build-cli\bin\Release\urconnect-cli.exe --version
```

```bash
# macOS：先安装 Xcode Command Line Tools
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

运行生成路网测试前，先构建全部 CLI 目标：

```bash
cmake --build build-cli --config Release -j 4
ctest --test-dir build-cli -C Release --output-on-failure
```

## 错误与可复现性

- 退出码 `0` 为成功，`1` 为命令/参数错误，`2` 为输入加载错误或捕获的分析异常。也要检查 stderr 中的信息。
- `road ID ... is outside the input network`：检查从 0 开始的记录序号，以及要素是否为两个顶点。
- `weight attribute not found or not numeric`：核对 DBF 字段名称及数值类型。
- 缺少 `.shx`/`.dbf` 或 DBF 不可写：恢复附属文件、写权限并解除文件占用。
- 找不到命令：使用 `./urconnect-cli` 或 `.\urconnect-cli-windows-x64.exe`，或将程序目录加入 `PATH`。
- 程序格式不匹配：检查系统和 CPU 架构；Linux 运行库不兼容时，可在目标系统上从源码构建。

可复现分析应记录 `--version`、完整命令、输入文件校验和、坐标系/单位以及数值权重定义。保留原始输入，并为每次运行使用独立的工作数据和输出。
