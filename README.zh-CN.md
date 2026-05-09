<p align="center">
  <img src="docs/source/_static/images/guide/4.60.png" alt="UrConnect 上海加权可达量分析" width="900">
</p>

<h1 align="center">UrConnect</h1>

<p align="center">
  面向线段化城市街道网络的创新空间组构分析工具，提出 UrConnect 的可达量、转向距离、加权可达性和路径分析算法。
</p>

<p align="center">
  <a href="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml"><img src="https://github.com/intelligibleCityLab/UrConnect/actions/workflows/docs.yml/badge.svg" alt="Docs build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0--or--later-0f766e" alt="GPL-3.0-or-later license"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-11-00599C" alt="C++11">
  <img src="https://img.shields.io/badge/Qt-5.15-41CD52" alt="Qt 5.15">
</p>

<p align="center">
  <a href="README.md">English</a> |
  <a href="README.zh-TW.md">繁體中文</a> |
  <a href="https://intelligiblecitylab.github.io/UrConnect/zh-CN/installation.html">安装</a> |
  <a href="https://intelligiblecitylab.github.io/UrConnect/zh-CN/getting-started.html">快速开始</a> |
  <a href="https://intelligiblecitylab.github.io/UrConnect/zh-CN/user-guide.html">用户指南</a>
</p>

## 简介

UrConnect 是一个面向线段化街道网络的桌面分析工具。它将拓扑距离、米制距离和街道属性数据结合起来，可用于城市形态、街道网络、POI、人口、建筑面积等多源数据分析。

主要能力包括：

- 米制、转向、交叉口和组合可达量分析
- 针对所选线段的交互式可达量分析
- 转向距离、交叉口距离和点距离分析
- 手工 OD 或 OD 矩阵的最短路径模拟
- 色谱可视化、屏幕导出、属性导出和 Shapefile 结果写入

## 文档

文档构建为 Sphinx HTML 页面，包含英文、简体中文和繁体中文三个入口：

- [英文文档](https://intelligiblecitylab.github.io/UrConnect/en/installation.html)
- [简体中文文档](https://intelligiblecitylab.github.io/UrConnect/zh-CN/installation.html)
- [繁体中文文档](https://intelligiblecitylab.github.io/UrConnect/zh-TW/installation.html)

## 安装与构建

请从 [GitHub Releases](https://github.com/intelligibleCityLab/UrConnect/releases) 下载对应系统的安装包。Windows 是 v0.1.0 的主要发布目标；macOS 和 Linux 包作为 experimental 版本提供，跨平台验证仍在继续。

源码构建示例：

```bash
cmake -S . -B build -DQT5_ROOT=/path/to/Qt/5.15 -DBOOST_ROOT=/path/to/boost
cmake --build build --config Release
```

更多平台命令见 [安装文档](https://intelligiblecitylab.github.io/UrConnect/zh-CN/installation.html)。

## 许可证

UrConnect 使用 GNU General Public License v3.0 or later。第三方组件说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 致谢

UrConnect 使用了 depthmapX、sala、genlib 和 Shapelib 的开源组件，用于桌面程序、地图、数据和 Shapefile 基础设施。软件的核心分析贡献是 UrConnect 可达量、距离、加权和路径分析算法体系，该方法由深圳大学和 Georgia Institute of Technology 的研究合作者共同发展。
