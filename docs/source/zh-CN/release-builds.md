# 发布构建

项目会使用 GitHub Actions 生成 Windows、macOS 和 Linux 发布包。Windows/MSVC 是历史上最成熟的路径；macOS 本机已经可以用 Homebrew Qt 5 和 Boost 完成 Release 构建；Linux 使用同一套 CMake 目标，并依赖 Qt5、Boost、OpenGL 和 GLU 开发库。

## 推荐策略

1. 所有 PR 先跑三平台构建。
2. 只有 `v0.1.0` 这类版本标签触发 release 包上传。
3. 所有发布包都随附 GPLv3 许可文本。
4. 某个平台的 CI 未稳定前，将对应发布包标记为预览构建。

本机可以用于排查问题，但正式二进制应由 CI 生成，以保证平台隔离和可复现。
