# 3rd Party sources

This directory contains 3rd-party source code that we decided not to handle through automatic dependency management for various reasons described below.
Please prefer automatic dependency management if you can!

Reasonning for:
- `float16_t.hpp`: we need a `float16_t` type but currently cannot use C++23's standard definition as some toolchains cannot use the most recent versions of compilers supporting C++23. Hence we decided to use this header (modified with a note about where it was taken from) but only if building with C++<23

- `value_ptr_lite`: we need a value_ptr type to have value semantics to heap resources.
