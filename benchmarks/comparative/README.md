# Comparative Benchmarks (sparrow vs Apache Arrow)

This directory contains micro-benchmarks comparing the performance of sparrow against the Apache Arrow C++ library.

## Targets

- `sparrow_benchmarks_comparative`: Benchmark executable.
- `run_comparative_benchmarks`: Convenience custom target running benchmarks and printing results.
- `run_comparative_benchmarks_json`: Same but outputs JSON (`sparrow_comparative_benchmarks.json`).

## Building

Configure the project enabling both regular and comparative benchmarks:

```pwsh
cmake -S . -B build/comparative \` 
  -DBUILD_BENCHMARKS=ON \` 
  -DBUILD_COMPARATIVE_BENCHMARKS=ON \` 
  -DFETCH_DEPENDENCIES_WITH_CMAKE=ON \` 
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/comparative --target sparrow_benchmarks_comparative --config Release
```

If Apache Arrow is already installed and discoverable via `find_package(Arrow)`, it will be used. Otherwise a minimal Arrow build is fetched automatically (static library, limited components).

## Running

From the build directory:

```pwsh
cmake --build build/comparative --target run_comparative_benchmarks --config Release
```

Or run the executable directly:

```pwsh
build/comparative/benchmarks/comparative/sparrow_benchmarks_comparative.exe
```

JSON output:

```pwsh
cmake --build build/comparative --target run_comparative_benchmarks_json --config Release
```

## Adding More Comparative Benchmarks

Add new `.cpp` files here and list them in `benchmarks/comparative/CMakeLists.txt` (or glob/variable if expanded). Match both sparrow and Arrow implementations side-by-side with Google Benchmark registration.

## Notes

The fetched Arrow version is pinned to `apache-arrow-16.1.0`. Update the `GIT_TAG` in `CMakeLists.txt` to test newer versions.
