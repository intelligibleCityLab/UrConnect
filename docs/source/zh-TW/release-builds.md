# 發布構建

專案會使用 GitHub Actions 生成 Windows、macOS 和 Linux 發布包。Windows/MSVC 是歷史上最成熟的路徑；macOS 本機已經可以用 Homebrew Qt 5 和 Boost 完成 Release 構建；Linux 使用同一套 CMake 目標，並依賴 Qt5、Boost、OpenGL 和 GLU 開發庫。

## 推薦策略

1. 所有 PR 先跑三平台構建。
2. 只有 `v0.1.0` 這類版本標籤觸發 release 包上傳。
3. 所有發布包都隨附 GPLv3 授權文本。
4. 某個平台的 CI 未穩定前，將對應發布包標記為預覽構建。
