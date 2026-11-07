# 安裝

UrConnect 透過 GitHub Releases 和源碼分發。桌面程式使用 C++、Qt 5、CMake、Boost 標頭、OpenGL，以及隨源碼攜帶的 Shapelib。

## 平台狀態

目前原始碼版本為 0.2.4。發布包包含重新建置的 Windows 桌面程式和三個平台的獨立 CLI。macOS 和 Linux 包仍標為 experimental，更廣泛的平台驗證繼續進行。

## 從發布包安裝

請從 GitHub Releases 頁面查看已發布的安裝包。v0.2.4 的發布流程產生：

- Windows 桌面程式和 CLI：`UrConnect-v0.2.4-windows-x64.zip`
- Windows CLI：`urconnect-cli-windows-x64.exe`
- macOS experimental: `UrConnect-v0.2.4-macos-arm64-experimental.tar.gz`
- Linux experimental: `UrConnect-v0.2.4-linux-x64-experimental.tar.gz`

請完整解壓發布包後執行 Windows 的 `UrConnect.exe`，或 macOS/Linux 的 `UrConnect` 應用程式。Windows ZIP 已包含 Qt 執行環境。CLI 不需要 Qt 或圖形介面。

## 命令列安裝與使用

CLI 獨立安裝、輸入及輸出路徑選擇、參數傳遞，請參閱[命令列使用指南](command-line.md)，內含 Windows PowerShell 與 macOS/Linux 範例。下載 [CLI v0.2.5](https://github.com/intelligibleCityLab/UrConnect/releases/tag/cli-v0.2.5)，即可在同一程式中用 `--threads N|auto` 選擇執行緒數目。目前沒有 `--output` 參數。上面的桌面版 v0.2.4 安裝包保持不變，附帶的是舊 CLI。

## 從源碼構建

依賴：

- CMake 3.15 或更新版本
- 自動測試需要 Python 3；若只建置程式，可加 `-DBUILD_TESTING=OFF`
- 支援 C++11 的編譯器
- Qt 5.15 Core、Gui、Widgets、OpenGL 模組
- Boost 標頭
- OpenGL 和 GLU 開發庫

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="C:/Qt/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build --config Release
```

生成的可執行檔位於 `build/bin/Release/`。

### macOS

```bash
brew install cmake qt@5 boost
cmake -S . -B build \
  -DQT5_ROOT="$(brew --prefix qt@5)" \
  -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build --config Release -j 4
```

### Linux

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qtbase5-dev libqt5opengl5-dev \
  libboost-all-dev libgl1-mesa-dev libglu1-mesa-dev
cmake -S . -B build -DBOOST_ROOT=/usr/include
cmake --build build --config Release -j 4
```

## 構建說明

- 輸入網路必須是在 GIS 或其他繪圖工具中準備好的線段模型。
- UrConnect 不提供幾何編輯工具。
- Shapefile 輸出會寫回已開啟的源文件。
- CSV/TXT 輸入會生成對應的 Shapefile 輸出，便於在 GIS 中繼續使用。
