# Sparrow Benchmarks

This directory contains performance benchmarks for the Sparrow library, focusing on primitive array operations.

## Overview

The benchmarks use [Google Benchmark](https://github.com/google/benchmark) to measure the performance of various operations on `primitive_array` including:

- **Construction**: Building arrays from vectors, ranges, and nullable data
- **Element Access**: Random access through `operator[]`
- **Iteration**: Iterator traversal, range-based for loops, and value iterators
- **Modification**: Push-back operations and resizing
- **Memory Operations**: Copy operations

## Benchmark Types

### Construction Benchmarks
- `BM_PrimitiveArray_ConstructFromVector`: Tests construction from `std::vector<T>`
- `BM_PrimitiveArray_ConstructWithNulls`: Tests construction with nullable data (10% null values)

### Access Benchmarks
- `BM_PrimitiveArray_ElementAccess`: Tests random element access via `operator[]`

### Iteration Benchmarks
- `BM_PrimitiveArray_IteratorTraversal`: Tests iteration using iterators
- `BM_PrimitiveArray_RangeBasedFor`: Tests range-based for loop traversal  
- `BM_PrimitiveArray_ValueIterator`: Tests iteration through values only (skipping null checks)

### Modification Benchmarks
- `BM_PrimitiveArray_PushBack`: Tests append operations with `push_back`

### Memory Benchmarks
- `BM_PrimitiveArray_Copy`: Tests copy constructor performance

## Tested Types

Benchmarks are run for the following primitive types:
- `std::int32_t`
- `double` 
- `bool`

## Building and Running

### Prerequisites

The benchmarks require Google Benchmark to be installed or will be fetched automatically if CMake is configured to do so.

### Build Configuration

Enable benchmark building in CMake:

```bash
cmake -DBUILD_BENCHMARKS=ON ..
```

Or with package manager integration:

**Using vcpkg:**
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DBUILD_BENCHMARKS=ON ..
```

**Using Conan:**
```bash
conan install . -o build_benchmarks=True
cmake --preset conan-default -DBUILD_BENCHMARKS=ON
```

**Using Conda:**
```bash
conda env create -f environment-dev.yml
conda activate sparrow
cmake -DBUILD_BENCHMARKS=ON ..
```

### Running Benchmarks

After building, run the benchmarks:

```bash
# Run all benchmarks
./sparrow_benchmarks

# Run benchmarks with specific filters
./sparrow_benchmarks --benchmark_filter=".*int32.*"

# Output results in JSON format
./sparrow_benchmarks --benchmark_format=json --benchmark_out=results.json

# Run with different repetitions
./sparrow_benchmarks --benchmark_repetitions=5
```

### Using CMake Targets

The project provides convenient CMake targets for common benchmark scenarios:

```bash
# Basic benchmark execution
make run_benchmarks                    # Console output only
make run_benchmarks_json              # JSON output to file
make run_benchmarks_csv               # CSV output to file

# Combined output (console + file)
make run_benchmarks_console_json      # Console + JSON file
make run_benchmarks_console_csv       # Console + CSV file

# Statistical analysis
make run_benchmarks_detailed          # 5 repetitions with aggregated statistics

# Quick testing
make run_benchmarks_quick             # Test only small data sizes (100, 1000)

# Type-specific benchmarks
make run_benchmarks_int32             # Test int32_t primitives only
make run_benchmarks_double            # Test double primitives only  
make run_benchmarks_bool              # Test bool primitives only

# Help
make benchmarks_help                  # Show all available targets
```

## Benchmark Parameters

Most benchmarks test various array sizes ranging from 100 to 100,000 elements with a multiplier of 10 (100, 1000, 10000, 100000).

Some specific configurations:
- **Element Access**: Measures per-operation time in nanoseconds
- **Construction/Iteration**: Measures throughput in microseconds
- **Push Back**: Uses manual timing for more accurate measurements of individual insert operations

## Interpreting Results

Benchmark results show:
- **Time**: Average time per iteration
- **CPU**: CPU time used
- **Iterations**: Number of times the benchmark was run
- **Items Processed**: Throughput metric (elements processed per second)

Lower times indicate better performance. The "items processed" metric helps compare performance across different array sizes.

## Output File Formats

### JSON Output
JSON output provides structured data suitable for automated analysis and visualization:

```json
{
  "context": {
    "date": "2025-07-08T10:30:00+00:00",
    "host_name": "hostname",
    "executable": "./sparrow_benchmarks",
    "num_cpus": 8,
    "mhz_per_cpu": 3200
  },
  "benchmarks": [
    {
      "name": "BM_PrimitiveArray_ConstructFromVector<int>/1000",
      "family_index": 0,
      "per_family_instance_index": 1,
      "run_name": "BM_PrimitiveArray_ConstructFromVector<int>/1000",
      "run_type": "iteration",
      "repetitions": 1,
      "repetition_index": 0,
      "threads": 1,
      "iterations": 12345,
      "real_time": 42.5,
      "cpu_time": 42.3,
      "time_unit": "us",
      "items_processed": 12345000
    }
  ]
}
```

### CSV Output
CSV output is suitable for spreadsheet analysis:

```csv
name,iterations,real_time,cpu_time,time_unit,items_processed
"BM_PrimitiveArray_ConstructFromVector<int>/100",123456,4.25,4.23,us,12345600
"BM_PrimitiveArray_ConstructFromVector<int>/1000",12345,42.5,42.3,us,12345000
```

### Detailed Statistics
When using `run_benchmarks_detailed`, additional statistical measures are provided:

- **mean**: Average across repetitions
- **median**: Middle value across repetitions  
- **stddev**: Standard deviation
- **cv**: Coefficient of variation (stddev/mean)

## Adding New Benchmarks

To add benchmarks for new array types or operations:

1. Create a new benchmark function following the pattern:
   ```cpp
   template <typename T>
   static void BM_PrimitiveArray_NewOperation(::benchmark::State& state) {
       // Setup
       for (auto _ : state) {
           // Benchmark code
           ::benchmark::DoNotOptimize(result);
       }
       state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
   }
   ```

2. Register the benchmark:
   ```cpp
   BENCHMARK_TEMPLATE(BM_PrimitiveArray_NewOperation, std::int32_t)
       ->RangeMultiplier(10)->Range(100, 100000);
   ```

3. Add the source file to `CMakeLists.txt` if creating a new file.
