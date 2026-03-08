# UrConnect

UrConnect 是一个面向城市街道网络的独立空间组构分析工具。项目基于 depthmapX 开发，并扩展了 UrbanConnect 分析方法：以可达量（reach）和转向距离（directional distance）为核心，将拓扑距离、米制距离以及街道长度、负载等 GIS 属性共同纳入分析。

默认英文说明见 [README.md](README.md)。

## 功能概览

- 支持城市形态和街道网络研究中的桌面端空间网络分析流程。
- 支持 Shapefile 等 GIS 数据格式，便于与其他城市数据叠合分析。
- 保留 depthmapX 风格的图分析、线段分析、可视分析和地图分析能力。
- 增加 UrbanConnect 指标，使结果更便于设计、规划和案例比较。

## 项目结构

```text
.
├── depthmapX/        Qt 桌面应用、视图、对话框、资源和 UI 文件
├── salalib/          核心空间分析算法以及图/地图数据结构
├── genlib/           几何、数学、解析和通用工具代码
├── mgraph440/        为兼容保留的旧版图分析代码
├── SNDAApp/          UrbanConnect 分析代码以及随项目携带的 Shapelib 源码
├── docs/             项目文档
└── .github/          GitHub issue、PR 和项目维护模板
```

当前仓库包含 C++ 桌面应用及其随项目构建的内部库。主程序目标位于 `depthmapX/`。

## 构建

UrConnect 目前需要从源码构建。

### 依赖

- CMake 3.13 或更新版本
- 支持 C++11 的编译器
- Qt 5.15.x，并包含 Core、Gui、Widgets、OpenGL 模块
- Boost 头文件
- OpenGL

目前项目主要面向 Windows/MSVC 构建；其他平台可能需要少量 CMake 或依赖配置调整。

### Windows 构建示例

安装 Visual Studio、CMake、Qt 5 和 Boost 后，显式指定依赖路径：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="D:/program/QT/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="D:/program/boost_1_73_0"
cmake --build build --config Release
```

可执行文件会生成在 `build/bin/Release/` 下。

## 文档

更多架构、构建、开发和发布准备说明见 [docs/README.zh-CN.md](docs/README.zh-CN.md)。

## 引用

如果在学术研究中使用 UrConnect，请引用本项目。机器可读的引用信息见 [CITATION.cff](CITATION.cff)。仓库公开前应补充正式论文、作者、DOI 等信息。

## 许可证

UrConnect 使用 GNU General Public License v3.0 开源，详见 [LICENSE](LICENSE)。

这是目前最稳妥且正确的许可证选择，因为 UrConnect 派生自带有 GPLv3-or-later 许可声明的 depthmapX/sala 组件。项目中还包含若干兼容许可的第三方组件，例如 LGPL 的 genlib 代码，以及 MIT-style 或 LGPL 双许可的 Shapelib 代码。详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 致谢

UrConnect 基于 depthmapX 及其相关 sala/genlib 组件开发。感谢 depthmapX、sala、genlib、Shapelib 的原始贡献者，以及来自深圳大学和佐治亚理工学院等机构、参与 UrbanConnect 方法开发的研究合作者。
