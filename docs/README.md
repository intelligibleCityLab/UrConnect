# UrConnect Documentation

This folder contains the Sphinx source for the UrConnect user documentation. The rendered documentation is published as HTML pages with language-specific navigation.

[简体中文](README.zh-CN.md)

## Rendered Pages

- [English](https://intelligiblecitylab.github.io/UrConnect/en/installation.html)
- [简体中文](https://intelligiblecitylab.github.io/UrConnect/zh-CN/installation.html)
- [繁體中文](https://intelligiblecitylab.github.io/UrConnect/zh-TW/installation.html)

When the repository is private, GitHub Pages may not be available. The docs workflow still builds the HTML pages and uploads them as a GitHub Actions artifact.

## Local Preview

To preview the pages locally:

```bash
python3 -m pip install -r requirements.txt
sphinx-build -b html source _build/html
```
