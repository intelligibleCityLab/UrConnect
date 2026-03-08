# UrConnect 文档

在仓库正式公开前，本文档先保持轻量。后续可以扩展为用户指南、开发指南、示例库以及 API/构建参考。

默认英文文档见 [README.md](README.md)。

## 架构说明

UrConnect 是一个基于 CMake 的 C++ 桌面应用：

- `depthmapX/` 包含 Qt 应用外壳、对话框、视图、OpenGL 渲染、资源文件以及最终可执行目标。
- `salalib/` 包含主要空间分析引擎，包括轴线、线段、可视图、等视域、agent、解析器和地图转换模块。
- `genlib/` 包含继承自 depthmapX 相关代码的几何、数学、数据结构、XML 和通用辅助工具。
- `mgraph440/` 保留旧版图分析结构，用于兼容旧格式和旧流程。
- `SNDAApp/` 包含 UrbanConnect 特定分析代码，以及用于 Shapefile I/O 的 Shapelib 源码。

## 构建说明

建议使用源码外构建：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DQT5_ROOT="path/to/Qt/5.15.x/msvc2019_64" `
  -DBOOST_ROOT="path/to/boost"
cmake --build build --config Release
```

顶层 CMake 项目将 `QT5_ROOT` 和 `BOOST_ROOT` 暴露为缓存变量。不要在提交的项目文件里写死个人机器路径。

## 开源前检查清单

仓库公开前建议确认：

- 确认公开项目名称和仓库描述。
- 将 `CITATION.cff` 中的占位引用信息替换为正式引用。
- 复核 `THIRD_PARTY_NOTICES.md` 中的第三方代码和许可说明。
- 确认没有私有数据集、生成二进制、构建产物、token 或仅限内部使用的文件被跟踪。
- 只有在再分发条款清晰时，才加入截图、教程数据和示例数据。
- 明确 release 是只发布源码包，还是发布签名二进制安装包。

## 维护方式

bug 和功能请求使用 GitHub issues，代码变更使用 pull requests。文档尽量保持中英双语，默认英文。
