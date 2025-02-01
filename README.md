# sparrow

[![GHA Linux](https://github.com/man-group/sparrow/actions/workflows/linux.yml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/linux.yml)
[![GHA OSX](https://github.com/man-group/sparrow/actions/workflows/osx.yml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/osx.yml)
[![GHA Windows](https://github.com/man-group/sparrow/actions/workflows/windows.yml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/windows.yml)
[![GHA Docs](https://github.com/man-group/sparrow/actions/workflows/docs.yaml/badge.svg)](https://github.com/man-group/sparrow/actions/workflows/docs.yaml)
[![Codecov](https://codecov.io/gh/man-group/sparrow/graph/badge.svg)](https://app.codecov.io/gh/man-group/sparrow)

C++20 idiomatic APIs for the Apache Arrow Columnar Format

## Introduction

`sparrow` is an implementation of the Apache Arrow Columnar format in C++. It provides array structures
with idiomatic APIs and convenient conversions from and to the [C interface](https://arrow.apache.org/docs/dev/format/CDataInterface.html#structure-definitions).

`sparrow` requires a modern C++ compiler supporting C++20.

## Installation

### Package managers

We provide a package for the mamba (or conda) package manager:

```bash
mamba install -c conda-forge sparrow
```

### Install from sources

`sparrow` has a few dependencies that you can install in a mamba environment:

```bash
mamba env create -f environment-dev.yml
mamba activate sparrow
```

You can then create a build directory, and build the project and install it with cmake:

```bash
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON \
    -BUILD_DOCS=ON \
    ..
make install
```

## Usage

### Requirements

Compilers:
- Clang 18 or higher
- GCC 12 or higher
- Apple Clang 16 or higher
- MSVC 19.41 or higher

### Initialize data with sparrow and extract C data structures

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> ar = { 1, 3, 5, 7, 9 };
auto [arrow_array, arrow_schema] = sp::extract_arrow_structures(std::move(ar));
// Use arrow_array and arrow_schema as you need (serialization, passing it to
// a third party library)
// ...
// You are responsible for releasing the structure in the end
arrow_array.release(&arrow_array);
arrow_schema.release(&arrow_schema);
```

### Initialize data with sparrow and use C data structures

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> ar = { 1, 3, 5, 7, 9 };
// Caution: get_arrow_structures returns pointers, not values
auto [arrow_array, arrow_schema] = sp::get_arrow_structures(ar);
// Use arrow_array and arrow_schema as you need (serialization, passing it to
// a third party library)
// ...
// do NOT release the C structures in the end, the "ar" variable will do it for you
```

### Read data from somewhere and pass it to sparrow

```cpp
#include "sparrow/sparrow.hpp"
#include "thrid-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray array;
ArrowSchema schema;
tpl::read_arrow_structures(&array, &schema);

sp::array ar(&array, &schema);
// Use ar as you need
// ...
// You are responsible for releasing the structure in the end
arrow_array.release(&arrow_array);
arrow_schema.release(&arrow_schema);
```

### Read data from somewhere and move it into sparrow

```cpp
#include "sparrow/sparrow.hpp"
#include "thrid-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray array;
ArrowSchema schema;
tpl::read_arrow_structures(&array, &schema);

sp::array ar(std::move(array), std::move(schema));
// Use ar as you need
// ...
// do NOT release the C structures in the end, the "ar" variable will do it for you
```

## Documentation

The documentation (currently being written) can be found at https://man-group.github.io/sparrow/index.html

## Acknowledgements

This development has been funded as part of a collaboration between [ArcticDB](https://github.com/man-group/ArcticDB), Bloomberg, and [QuantStack](https://quantstack.net/).

## License

This software is licensed under the Apache License 2.0. See the [LICENSE](LICENSE) file for details.
