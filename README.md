# sparrow

[![GHA Linux](https://github.com/man-group/sparrow/actions/workflows/linux.yml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/linux.yml)
[![GHA OSX](https://github.com/man-group/sparrow/actions/workflows/osx.yml/badge.svg)](https://github.com/man-group/sparrowr/actions/workflows/osx.yml)
[![GHA Windows](https://github.com/man-group/sparrow/actions/workflows/windows.yml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/windows.yml)

C++20 idiomatic APIs for the Apache Arrow Columnar Format

## Introduction

`sparrow` is an implementation of the Apache Arrow Columnar format in C++. It provides array structures
with idiomatic APIs and convenient conversions from and to the [C interface](https://arrow.apache.org/docs/dev/format/CDataInterface.html#structure-definitions).

`sparrow` requires a modern C++ compiler supporting C++20.

WARNING: `sparrow` is still in preview and should not be considered production-ready.

## Installation

### Package managers

We provide a package for the mamba (or conda) package manager:

```bash
mamba install -c conda-forge sparrow
```

### Install from sources

`sparrow` is a header-only library.

You can directly install it from the sources:

```bash
cmake -DCMAKE_INSTALL_PREFIX=your_install_prefix
make install
```
