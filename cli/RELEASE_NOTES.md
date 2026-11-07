One CLI executable now supports a selectable analysis thread count: `--threads 1`, `--threads 8`, or `--threads auto` (default: `1`). No separate single-thread or multithread executable is needed.

The option applies to all seven whole-network commands: `reach`, `md`, `dr`, `ddl`, `mdr`, `jnr`, and `jnd`. Workers process disjoint origin roads; result files are written serially after computation. The actual worker count is capped by CPU concurrency and the number of origin roads. Origin-based `netreach`, `stepdepth`, and `od` do not accept this option.

```sh
urconnect-cli mdr network.shp --radius 250 --angle 45 --turns 2 --threads 8
```

The shared calculation engine is unchanged from desktop v0.2.4, including the corrected Grasshopper-compatible directional-metric reach order. Automated tests compare default, 1, 2, 12 and auto worker runs across all seven commands, parameter sweeps, weighting and competing routes, in addition to existing reach and DDL regressions.

These are CLI-only packages for Windows x64, macOS Apple Silicon and Linux x64. Desktop UrConnect remains v0.2.4; its existing packages are unchanged. The older CLI bundled in those desktop packages does not support `--threads`; use this CLI download. macOS and Linux packages remain experimental, and the macOS binary is ad-hoc signed, not notarized. Linux is built and tested on Ubuntu 24.04.

Each archive includes `CLI-USAGE.txt`, Apt and Hangzhou example datasets, validation reports, and license notices. The standalone Windows EXE is also available. No Qt or Python installation is required to run the CLI. Use copies of complete Shapefile datasets: results update the input DBF, and there is no separate `--output` option.

The English, Simplified Chinese and Traditional Chinese (Hong Kong, `zh-HK`) guides now cover installation, input/output paths, parameter lists, thread selection and OD result locations:

- [English](https://intelligiblecitylab.github.io/UrConnect/en/command-line.html)
- [简体中文](https://intelligiblecitylab.github.io/UrConnect/zh-CN/command-line.html)
- [繁體中文（香港）](https://intelligiblecitylab.github.io/UrConnect/zh-HK/command-line.html)
