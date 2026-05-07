# UrConnect 文档

本目录包含 UrConnect 用户文档的 Sphinx 源文件。渲染后的文档以 HTML 页面发布，并提供分语言导航。

默认英文文档见 [README.md](README.md)。

## 渲染页面

- [English](https://intelligiblecitylab.github.io/UrConnect/en/installation.html)
- [简体中文](https://intelligiblecitylab.github.io/UrConnect/zh-CN/installation.html)
- [繁體中文](https://intelligiblecitylab.github.io/UrConnect/zh-TW/installation.html)

仓库为 private 时，GitHub Pages 可能不可用。docs workflow 仍会构建 HTML 页面，并把结果上传为 GitHub Actions artifact。

## 本地预览

如需本地预览页面：

```bash
python3 -m pip install -r requirements.txt
sphinx-build -b html source _build/html
```
