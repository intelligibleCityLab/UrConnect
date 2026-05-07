# 安装

UrConnect 通过 GitHub Releases 和源码形式分发。桌面程序使用 C++、Qt 5、CMake、Boost 头文件、OpenGL，以及随源码携带的 Shapelib。

## 平台状态

Windows 是 v0.1.0 的主要发布目标。macOS 和 Linux 包作为 experimental 版本提供，跨平台验证仍在继续。

## 从发布包安装

发布包可在 GitHub Releases 页面下载：

- Windows: `UrConnect-v0.1.0-windows-x64.zip`
- macOS experimental: `UrConnect-v0.1.0-macos-arm64-experimental.tar.gz`
- Linux experimental: `UrConnect-v0.1.0-linux-x64-experimental.tar.gz`

解压后运行 `UrConnect` 可执行文件或应用包。

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
