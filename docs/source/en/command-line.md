# Command-Line Interface

This guide covers `urconnect-cli` **v0.2.5**, the standalone, headless interface to the UrConnect analysis engine. It does not require Qt, a GUI, Python, or a display server. Use the desktop guides for visualization and point-distance tools; the commands below are the analyses exposed by this CLI version.

## Install And Run

Download the matching asset from [CLI v0.2.5 Releases](https://github.com/intelligibleCityLab/UrConnect/releases/tag/cli-v0.2.5):

| Platform | Asset and executable | Status |
| --- | --- | --- |
| Windows x64 | `urconnect-cli-windows-x64.exe` | Standalone CLI; alternatively extract `UrConnect-CLI-0.2.5-windows-x64.zip` and use `urconnect-cli.exe` |
| macOS ARM64 (Apple Silicon) | `UrConnect-CLI-0.2.5-macos-arm64.tar.gz`; `urconnect-cli` in the extracted CLI package | Experimental; not Apple-notarized; not an Intel binary |
| Linux x64 | `UrConnect-CLI-0.2.5-linux-x64.tar.gz`; `urconnect-cli` in the extracted package | Experimental; tested on Ubuntu 24.04; standard C/C++ runtime libraries required |

These are platform-specific executables, not one binary that runs on every OS. Shapelib is statically linked into the CLI. The Linux desktop GUI has additional Qt/OpenGL dependencies, but the CLI does not.

From the directory containing the executable:

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

Later examples assume `urconnect-cli` is on `PATH`; otherwise use the executable path above. Quote paths containing spaces. On macOS, Gatekeeper may prevent an unnotarized download from running; this is separate from analysis or input-data errors. Do not disable system-wide security protections.

## Choose Input And Output Paths

The executable path selects the program; the next path after the command selects the input network. Relative paths are resolved from the terminal's current working directory, not from the executable directory. Use absolute paths when these folders differ.

There is no separate output-directory option. To keep the original unchanged and put results in a chosen folder, copy **all files belonging to the dataset** there and run the copied `.shp`. These examples use a new folder for each run; replace the example paths with your own.

```powershell
# Windows PowerShell: standalone executable, source data, and output folder
$cli = "C:\Tools\UrConnect\urconnect-cli-windows-x64.exe"
$source = "C:\Street Data\network"
$runDir = "C:\Results\reach-run-01"
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Copy-Item -Path "$source.*" -Destination $runDir
& $cli reach "$runDir\network.shp" --radius "400,800"
```

```bash
# macOS / Linux: the extracted executable can live anywhere
cli="/path/to/UrConnect/urconnect-cli"
source_base="/path/to/Street Data/network"
run_dir="/path/to/Results/reach-run-01"
mkdir "$run_dir" && cp "$source_base".* "$run_dir/" && \
  "$cli" reach "$run_dir/network.shp" --radius 400,800
```

Create the parent `Results` folder first. The resulting `network.dbf` is in `reach-run-01`; open its accompanying `network.shp` in GIS to inspect the `R400` and `R800` fields. PowerShell needs `&` to run a quoted executable path or a variable holding it. When using the Windows ZIP, set `$cli` to its `urconnect-cli.exe` instead.

For `od --pairs`, the result CSV is written beside the **pairs CSV**, so copy that file to the run folder too if you want all results together. The output table below distinguishes each command's destinations.

## Prepare The Input

```{warning}
Work on a copy of the complete Shapefile dataset. Analyses can append or update fields in the input `.dbf`, and repeated runs can overwrite matching result fields/files. There is no `--output` option or automatic backup. Do not run concurrent commands against the same dataset.
```

- The CLI accepts **Shapefile input only**, not the desktop application's CSV/TXT network inputs. Keep matching `.shp`, `.shx`, and `.dbf` files together; retain `.prj` and other sidecars when copying data.
- Each analyzed road must be a **two-vertex line segment**. Prepare every feature in this format before running; do not rely on the importer skipping unsupported geometry safely. Split/snap/clean the network in GIS first; geometric crossings do not automatically create junctions.
- Use projected coordinates in metres for metre-based radii and distances. The CLI does not reproject data; metric calculations use the input coordinate units.
- Road IDs for `--from`, `--to`, and OD CSVs are **zero-based Shapefile record indices**, not values from a custom `ID`/`FID` attribute. Reordering the dataset changes these IDs.
- Weight fields must already exist as numeric DBF attributes. Use their exact names. The input DBF and output directories must be writable; close other applications holding file locks.

## Command Reference

Syntax: `urconnect-cli <command> <input.shp> [options]`.

| Command | Analysis | Required parameters |
| --- | --- | --- |
| `reach` | Metric reach | `--radius` |
| `md` | Mean metric distance within a metric radius | `--radius` |
| `dr` | Directional reach | `--angle`, `--turns` |
| `ddl` | Directional distance within a metric radius | `--radius`, `--angle` |
| `mdr` | Combined metric and directional reach | `--radius`, `--angle`, `--turns` |
| `jnr` | Junction reach | `--junction-degree`, `--junction-limit` |
| `jnd` | Junction distance within a metric radius | `--radius`, `--junction-degree` |
| `netreach` | Reachable subset from source road(s) | `--from` and the parameters for the chosen `--type` |
| `stepdepth` | Distance from source road(s) to each road | `--from`; additionally `--angle` for `dr`, or `--junction-degree` for `jnr` |
| `od` | Shortest paths between roads | `--from` (optionally `--to`), or `--pairs`; additionally `--angle` for `dr`, or `--junction-turns` for `jnr` |

### Parameters And Sweeps

| Option | Meaning and accepted values |
| --- | --- |
| `--radius` | Nonnegative metric distance; comma-separated lists and `n` (unlimited) accepted |
| `--angle` | Nonnegative turn threshold in degrees (normally 0–180); a turn is counted when the angle reaches or exceeds this threshold; angle lists supported by `dr`, `ddl`, and `mdr`; one value for origin-based commands |
| `--turns` | Maximum directional changes; nonnegative integer list or `n` |
| `--junction-degree` | Single nonnegative integer degree threshold for junction reach/distance; nodes of at least this degree are counted |
| `--junction-limit` | Maximum counted junctions; nonnegative integer list or `n` |
| `--junction-turns` | Single nonnegative integer junction-degree threshold for the additional junction-counting rule; **not** a limit on the number of turns/junctions |
| `--weight` | Comma-separated numeric attribute names, e.g. `ShopCount,Population`; attribute weighting, not a custom shortest-path cost function |
| `--type` | `mr` (metric, default), `dr` (directional), or `jnr` (junction); only for `netreach`, `stepdepth`, and `od` |
| `--from`, `--to` | Comma-separated road IDs |
| `--pairs` | OD CSV path; cannot be combined with `--from` or `--to` |
| `--threads` | Whole-network analysis workers: positive integer or `auto`; default `1` |

Write lists without spaces: `400,800`, not `400, 800`. `--key value` and `--key=value` are accepted, but the same option cannot be supplied twice.

All commands accept `--weight`. `--junction-turns` is available for `reach`, `md`, `dr`, `mdr`, `netreach`, and `od`. For `netreach`, use only the parameters matching its type: `mr` needs `--radius`; `dr` needs `--angle` and `--turns`; `jnr` needs `--junction-degree` and `--junction-limit`. Use a single radius/turn/junction limit per `netreach` invocation; its subset export is not a whole-network sweep.

Whole-network commands support parameter sweeps. With `mdr`, radius and turn lists are combined for each angle:

```bash
urconnect-cli reach network.shp --radius 400,800 --weight ShopCount
urconnect-cli md network.shp --radius 400,800
urconnect-cli dr network.shp --angle 45,90 --turns 0,1,2
urconnect-cli ddl network.shp --radius 400,800 --angle 45
urconnect-cli mdr network.shp --radius 400,800 --angle 45 --turns 1,2
urconnect-cli jnr network.shp --junction-degree 3 --junction-limit 2,3
urconnect-cli jnd network.shp --radius 800 --junction-degree 3
```

`ShopCount` is illustrative: replace it with an existing numeric field. Unlimited reach can be requested with `--radius n`. Start with small finite radii on large networks.

## Source Roads And OD Paths

Replace the example road IDs with valid record indices from your network:

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

`stepdepth` with several sources records the minimum distance from those sources. For `od`, omitting `--to` requests paths from the source(s) to the network; this can create many outputs. Manual source/destination lists request their combinations. Use an OD CSV for explicitly paired requests.

### Batch OD CSV

Create a UTF-8 CSV with a header and two unquoted integer columns:

```text
origin,destination
10,500
20,500
```

The first line is treated as a header; each subsequent nonblank row supplies origin and destination in that order. Do not omit the header or use custom attribute IDs.

```bash
urconnect-cli od network.shp --pairs pairs.csv --type mr
urconnect-cli od network.shp --pairs pairs.csv --type dr --angle 45
urconnect-cli od network.shp --pairs pairs.csv --type jnr --junction-turns 3
```

Batch mode writes `pairs_MR.csv`, `pairs_DR.csv`, or `pairs_JnR.csv` beside the pairs file and a `PathCount` field to the input DBF. It does **not** export the manual-mode route Shapefiles. The CSV columns are `origin,destination,distance,path`; the quoted `path` contains intermediate road IDs, excluding the two endpoint roads. `--type` selects the route-selection criterion. For all three types, the CSV `distance` column reports the selected route’s geometric length in input coordinate units, not a count of turns or junctions.

## Outputs

| Workflow | Output |
| --- | --- |
| Whole-network analyses | Result fields appended/updated in the input DBF; e.g. `R400`, `R800`, `mMD400`, `R1d45a`, `R1j3x` |
| `netreach` | `Reach_*` Shapefile dataset beside the network |
| `stepdepth` | `StepD_mr`, `StepD_dr`, or `StepD_jnr` in the input DBF; unreachable roads use `-1` |
| Manual `od` | `Path_*` Shapefile output(s) beside the network, with subdirectories for multiple paths |
| `od --pairs` | Metric-specific CSV beside the pairs file plus input DBF `PathCount` |

DBF field names are limited to 10 characters. Long parameter/weight names may be shortened and can collide; keep names short and use separate dataset copies for ambiguous sweeps. See [Output Fields](output-fields.md) for the GUI/engine naming conventions. The CLI writes no map images and has no output-directory or CRS-conversion option.

## Choose The Thread Count

**One CLI executable supports both single-threaded and multithreaded analysis.** Use `--threads 1`, `--threads 8`, or `--threads auto`; the default is `1`. This option applies to the seven whole-network commands: `reach`, `md`, `dr`, `ddl`, `mdr`, `jnr`, and `jnd`. It is not accepted by `netreach`, `stepdepth`, or `od`.

```bash
urconnect-cli mdr network.shp --radius 250 --angle 45 --turns 2 --threads 8
urconnect-cli reach network.shp --radius 400,800 --threads auto
```

The actual worker count is capped by available CPU concurrency and the number of origin roads, and is printed as `Analysis workers: N`. Each worker processes its own origin roads; DBF output is written serially after computation finishes. Parameter sweeps still produce the same result columns. Using more threads can increase memory use; start with a moderate count on large networks. Never run separate CLI processes against the same dataset simultaneously.

CLI **v0.2.5** includes this option; the CLI bundled with desktop **v0.2.4** does not. Download the current CLI package above. The desktop application remains v0.2.4; no separate single-thread/multithread binaries are needed. Network loading and angle preprocessing may use threads independently. The `-j 4` in source-build examples controls compilation, not analysis.

Whole-network `mdr` retains the Grasshopper procedure introduced in v0.2.4: first select the directional-reachable subset, then compute metric reach within it. CLI `netreach` exposes only `mr`, `dr`, and `jnr`, not the desktop application's combined directional-metric interactive reach.

## Build The CLI Only

Build dependencies: Git, CMake 3.15+, a C and C++11 compiler, and Boost headers. Python 3 is required for the tests enabled by default; use `-DBUILD_TESTING=OFF` if building only the executable without tests. The first CMake configuration needs internet access to fetch spdlog. Shapelib sources are bundled; Qt/OpenGL are unnecessary for a CLI-only build.

Download the source archive from the release page and open a terminal in its extracted root, or clone the release tag:

```bash
git clone --branch cli-v0.2.5 --depth 1 https://github.com/intelligibleCityLab/UrConnect.git
cd UrConnect
```

```powershell
# Windows, with Visual Studio 2022 C++ tools and Boost installed
cmake -S . -B build-cli -G "Visual Studio 17 2022" -A x64 `
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build-cli --config Release --target urconnect-cli
.\build-cli\bin\Release\urconnect-cli.exe --version
```

```bash
# macOS, with Xcode Command Line Tools installed
brew install cmake boost python
cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release \
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build-cli --target urconnect-cli -j 4
./build-cli/bin/Release/urconnect-cli --version
```

```bash
# Linux (Ubuntu)
sudo apt-get install python3 git build-essential cmake libboost-dev
cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release \
  -DURCONNECT_BUILD_GUI=OFF -DBOOST_ROOT=/usr/include
cmake --build build-cli --target urconnect-cli -j 4
./build-cli/bin/Release/urconnect-cli --version
```

To run the generated-network tests, build all CLI targets before invoking CTest:

```bash
cmake --build build-cli --config Release -j 4
ctest --test-dir build-cli -C Release --output-on-failure
```

## Errors And Reproducibility

- Exit status `0` means success; `1` reports command/parameter errors; `2` reports input/loading or caught analysis errors. Check stderr as well as the exit status.
- `road ID ... is outside the input network`: check zero-based record indices and two-vertex geometry.
- `weight attribute not found or not numeric`: check the exact DBF name and field type.
- Missing `.shx`/`.dbf` or unwritable DBF: restore the sidecars, permissions, and file availability.
- `command not found` or PowerShell lookup failure: use `./urconnect-cli` or `.\urconnect-cli-windows-x64.exe`, or add its directory to `PATH`.
- Wrong executable format: download the correct OS/architecture build. For an incompatible Linux runtime, build from source on the target system.

For reproducible runs, record `--version`, the exact command, input checksums, coordinate system/units, and numeric weight definitions. Preserve an untouched input copy and isolate the working data and outputs for each run.
