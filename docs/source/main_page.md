Sparrow                             {#mainpage}
=======

<div style="text-align: center; margin: 20px 0;">
<a href="./jupyterlite/" target="_blank">
<img src="https://jupyterlite.rtfd.io/en/latest/_static/badge.svg" alt="Try Sparrow in JupyterLite" style="margin: 10px;"/>
</a>
</div>

Introduction
------------

Sparrow stands for "Simple Arrow" and is an implementation of the [Apache Arrow Columnar format](https://arrow.apache.org/docs/format/Columnar.html) in C++20. It provides array structures with idiomatic APIs and convenient conversions from and to the [C interface](https://arrow.apache.org/docs/dev/format/CDataInterface.html#structure-definitions).

Sparrow requires a modern C++ compiler supporting C++20:

| Compiler    | Version         |
| ----------- | --------------- |
| Clang       | 18 or higher    |
| GCC         | 11.2 or higher  |
| Apple Clang | 16 or higher    |
| MSVC        | 19.41 or higher |

This software is licensed under the Apache License 2.0. See the LICENSE file for details.

\subpage installation

\subpage basic_usage

\subpage typed_arrays

\subpage array

\subpage builder

\subpage dev_build

