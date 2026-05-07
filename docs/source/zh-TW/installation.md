# 安裝

UrConnect 目前以源碼和計劃發布的二進位包形式分發。桌面程式使用 C++、Qt 5、CMake、Boost 標頭、OpenGL，以及隨源碼攜帶的 Shapelib。

## 平台狀態

專案的 CMake 已包含 Windows、macOS、Linux 三個平台分支。Windows/MSVC 是歷史上最成熟的構建路徑；macOS 已經在本機用 Homebrew Qt 5 和 Boost 通過 Release 構建；Linux 在三平台 CI 全部通過前仍建議視為預覽構建。

## 從發布包安裝

發布包可在 GitHub Releases 頁面下載：

- Windows: `UrConnect-windows-x64.zip`
- macOS: `UrConnect-macos-arm64.tar.gz` 或 `UrConnect-macos-x64.tar.gz`
- Linux: `UrConnect-linux-x64.tar.gz`

解壓後執行 `depthmapX` 可執行檔或應用程式包。可執行目標名繼承自 depthmapX，專案名稱和文件名稱為 UrConnect。

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
