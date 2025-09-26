Sparrow                             {#mainpage}
=======

<div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; text-align: center; margin: 20px 0;">
<h2 style="color: white; margin-top: 0;">🚀 Try Sparrow Interactive Notebooks</h2>
<p style="margin-bottom: 15px;">Explore Sparrow with interactive examples directly in your browser - no installation required!</p>
<a href="./jupyterlite/" target="_blank" style="display: inline-block; background: rgba(255,255,255,0.2); color: white; text-decoration: none; padding: 12px 24px; border: 2px solid rgba(255,255,255,0.3); border-radius: 25px; font-weight: bold; transition: all 0.3s ease;" onmouseover="this.style.background='rgba(255,255,255,0.3)'; this.style.transform='translateY(-2px)';" onmouseout="this.style.background='rgba(255,255,255,0.2)'; this.style.transform='translateY(0)';">Launch JupyterLite →</a>
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

