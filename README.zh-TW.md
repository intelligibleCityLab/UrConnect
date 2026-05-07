<p align="center">
  <img src="docs/source/_static/images/guide/4.32.png" alt="UrConnect 軟體介面" width="900">
</p>

<h1 align="center">UrConnect</h1>

<p align="center">
  面向城市街道網路的空間組構分析工具，基於 depthmapX，並擴展可達量、轉向距離、加權可達性和路徑分析工作流。
</p>

<p align="center">
  <a href="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml"><img src="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml/badge.svg" alt="Docs build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-0f766e" alt="GPL-3.0-or-later license"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-11-00599C" alt="C++11">
  <img src="https://img.shields.io/badge/Qt-5.15-41CD52" alt="Qt 5.15">
</p>

<p align="center">
  <a href="README.md">English</a> |
  <a href="README.zh-CN.md">简体中文</a> |
  <a href="docs/source/zh-TW/installation.md">安裝</a> |
  <a href="docs/source/zh-TW/getting-started.md">快速開始</a> |
  <a href="docs/source/zh-TW/user-guide.md">使用指南</a>
</p>

## 簡介

UrConnect 是一個面向線段化街道網路的桌面分析工具。它將拓撲距離、米制距離和街道屬性資料結合起來，可用於城市形態、街道網路、POI、人口、建築面積等多源資料分析。

主要能力包括：

- 米制、轉向、交叉口和組合可達量分析
- 針對所選線段的互動式可達量分析
- 轉向距離、交叉口距離和點距離分析
- 手工 OD 或 OD 矩陣的最短路徑模擬
- 色譜視覺化、螢幕匯出、屬性匯出和 Shapefile 結果寫入

## 文件

文件採用類似 OSMnx 的 ReadTheDocs/Sphinx 風格，包含英文、簡體中文和繁體中文三個入口：

- [英文文件](docs/source/en/installation.md)
- [簡體中文文檔](docs/source/zh-CN/installation.md)
- [繁體中文文件](docs/source/zh-TW/installation.md)

## 安裝與構建

專案計劃透過 GitHub Releases 提供 Windows、macOS 和 Linux 編譯包。Windows/MSVC 是歷史上最成熟路徑；macOS 已經在本機用 Homebrew Qt 5 和 Boost 通過 Release 構建；Linux 在 CI 完全穩定前作為預覽目標。

```bash
cmake -S . -B build -DQT5_ROOT=/path/to/Qt/5.15 -DBOOST_ROOT=/path/to/boost
cmake --build build --config Release
```

更多平台命令見 [安裝文件](docs/source/zh-TW/installation.md)。

## 授權

UrConnect 使用 GNU General Public License v3.0 or later。由於專案派生自帶 GPLv3-or-later 聲明的 depthmapX/sala 元件，這是目前最穩妥的授權選擇。第三方元件說明見 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
