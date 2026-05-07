# 安裝

UrConnect 透過 GitHub Releases 和源碼分發。桌面程式使用 C++、Qt 5、CMake、Boost 標頭、OpenGL，以及隨源碼攜帶的 Shapelib。

## 平台狀態

Windows 是 v0.1.0 的主要發布目標。macOS 和 Linux 包作為 experimental 構建提供，跨平台驗證仍在繼續。

## 從發布包安裝

請從 GitHub Releases 頁面下載對應系統的發布包：

- Windows: `UrConnect-v0.1.0-windows-x64.zip`
- macOS experimental: `UrConnect-v0.1.0-macos-arm64-experimental.tar.gz`
- Linux experimental: `UrConnect-v0.1.0-linux-x64-experimental.tar.gz`

解壓後執行 `UrConnect` 可執行檔或應用程式包。

## 從源碼構建

依賴：

- CMake 3.13 或更新版本
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
