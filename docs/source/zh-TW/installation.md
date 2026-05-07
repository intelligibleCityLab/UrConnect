# 安裝

UrConnect 透過 GitHub Releases 和源碼形式分發。桌面程式使用 C++、Qt 5、CMake、Boost 標頭、OpenGL，以及隨源碼攜帶的 Shapelib。

## 平台狀態

Windows 是 v0.1.0 的主要發布目標。macOS 和 Linux 包作為 experimental 版本提供，跨平台驗證仍在繼續。

## 從發布包安裝

發布包可在 GitHub Releases 頁面下載：

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

macOS 示例：

```bash
brew install cmake qt@5 boost
cmake -S . -B build \
  -DQT5_ROOT="$(brew --prefix qt@5)" \
  -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build --config Release -j 4
```
