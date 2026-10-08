# 安装

UrConnect 通过 GitHub Releases 和源代码分发。桌面程序使用 C++、Qt 5、CMake、Boost 头文件、OpenGL，以及随源代码携带的 Shapelib。

## 平台状态

当前源码版本为 0.2.2。发布包包含重新构建的 Windows 桌面程序和三个平台的独立 CLI。macOS 和 Linux 包仍标为 experimental，更广泛的平台验证继续进行。

## 从发布包安装

请从 GitHub Releases 页面查看已发布的安装包。v0.2.2 的发布流程生成：

- Windows 桌面程序和 CLI：`UrConnect-v0.2.2-windows-x64.zip`
- Windows CLI：`urconnect-cli-windows-x64.exe`
- macOS experimental: `UrConnect-v0.2.2-macos-arm64-experimental.tar.gz`
- Linux experimental: `UrConnect-v0.2.2-linux-x64-experimental.tar.gz`

请完整解压发布包后运行 Windows 的 `UrConnect.exe`，或 macOS/Linux 的 `UrConnect` 应用。Windows ZIP 已包含 Qt 运行库。CLI 不需要 Qt 或图形界面。

## 从源码构建

安装以下依赖：

- CMake 3.15 或更新版本
- 支持 C++11 的编译器
- Qt 5.15 Core、Gui、Widgets、OpenGL 模块
- Boost 头文件
- OpenGL 和 GLU 开发库

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="C:/Qt/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build --config Release
```

生成的可执行文件位于 `build/bin/Release/`。

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

## 构建说明

- 输入网络必须是在 GIS 或其他绘图工具中准备好的线段模型。
- UrConnect 不提供几何编辑工具。
- Shapefile 输出会写回已打开的源文件。
- CSV/TXT 输入会生成对应的 Shapefile 输出，便于在 GIS 中继续使用。
