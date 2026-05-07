# 安装

UrConnect 当前以源码和计划发布的二进制包形式分发。桌面程序使用 C++、Qt 5、CMake、Boost 头文件、OpenGL，以及随源码携带的 Shapelib。

## 平台状态

项目的 CMake 已包含 Windows、macOS、Linux 三个平台分支。Windows/MSVC 是历史上最成熟的构建路径；macOS 已经在本机用 Homebrew Qt 5 和 Boost 通过 Release 构建；Linux 在三平台 CI 全部通过前仍建议视为预览构建。

## 从发布包安装

发布包可在 GitHub Releases 页面下载：

- Windows: `UrConnect-windows-x64.zip`
- macOS: `UrConnect-macos-arm64.tar.gz` 或 `UrConnect-macos-x64.tar.gz`
- Linux: `UrConnect-linux-x64.tar.gz`

解压后运行 `depthmapX` 可执行文件或应用包。可执行目标名继承自 depthmapX，项目名称和文档名称为 UrConnect。

## 从源码构建

依赖：

- CMake 3.13 或更新版本
- 支持 C++11 的编译器
- Qt 5.15 Core、Gui、Widgets、OpenGL 模块
- Boost 头文件
- OpenGL 和 GLU 开发库

Windows 示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="C:/Qt/5.15.2/msvc2019_64" `
  -DBOOST_ROOT="C:/local/boost_1_83_0"
cmake --build build --config Release
```

macOS 示例：

```bash
brew install cmake qt@5 boost
cmake -S . -B build \
  -DQT5_ROOT="$(brew --prefix qt@5)" \
  -DBOOST_ROOT="$(brew --prefix boost)"
cmake --build build --config Release -j 4
```

Linux 示例：

```bash
sudo apt-get install -y build-essential cmake qtbase5-dev libqt5opengl5-dev \
  libboost-all-dev libgl1-mesa-dev libglu1-mesa-dev
cmake -S . -B build -DBOOST_ROOT=/usr/include
cmake --build build --config Release -j 4
```
