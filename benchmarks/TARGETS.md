# Sparrow Benchmark Targets Reference

This document provides a comprehensive reference for all available CMake benchmark targets.

## Quick Reference

| Target | Purpose | Output |
|--------|---------|--------|
| `run_benchmarks` | Basic benchmark execution | Console only |
| `run_benchmarks_json` | File-based JSON reporting | JSON file |
| `run_benchmarks_csv` | File-based CSV reporting | CSV file |
| `run_benchmarks_console_json` | Dual output | Console + JSON file |
| `run_benchmarks_console_csv` | Dual output | Console + CSV file |
| `run_benchmarks_detailed` | Statistical analysis | Console + JSON with stats |
| `run_benchmarks_quick` | Fast testing | Console (small sizes only) |
| `run_benchmarks_int32` | Type-specific | Console (int32_t only) |
| `run_benchmarks_double` | Type-specific | Console (double only) |
| `run_benchmarks_bool` | Type-specific | Console (bool only) |
| `benchmarks_help` | Help information | Lists all targets |

## Target Details

### Basic Execution Targets

#### `run_benchmarks`
- **Purpose**: Run all benchmarks with standard console output
- **Command**: `make run_benchmarks` or `cmake --build . --target run_benchmarks`
- **Output**: Console output only
- **Use Case**: Quick verification that benchmarks work

#### `benchmarks_help`
- **Purpose**: Display help information about available targets
- **Command**: `make benchmarks_help`
- **Output**: List of all available benchmark targets with descriptions

### File Output Targets

#### `run_benchmarks_json`
- **Purpose**: Generate machine-readable JSON output
- **Command**: `make run_benchmarks_json`
- **Output**: `sparrow_benchmarks.json`
- **Use Case**: Automated analysis, CI/CD integration, data visualization

#### `run_benchmarks_csv`
- **Purpose**: Generate spreadsheet-compatible CSV output
- **Command**: `make run_benchmarks_csv`  
- **Output**: `sparrow_benchmarks.csv`
- **Use Case**: Excel analysis, data processing, reporting

### Dual Output Targets

#### `run_benchmarks_console_json`
- **Purpose**: Show results on console while saving JSON file
- **Command**: `make run_benchmarks_console_json`
- **Output**: Console + `sparrow_benchmarks.json`
- **Use Case**: Interactive development with persistent results

#### `run_benchmarks_console_csv`
- **Purpose**: Show results on console while saving CSV file
- **Command**: `make run_benchmarks_console_csv`
- **Output**: Console + `sparrow_benchmarks.csv`
- **Use Case**: Interactive development with spreadsheet-friendly data

### Advanced Analysis Targets

#### `run_benchmarks_detailed`
- **Purpose**: Statistical analysis with multiple repetitions
- **Command**: `make run_benchmarks_detailed`
- **Output**: Console + `sparrow_benchmarks_detailed.json`
- **Configuration**: 5 repetitions with aggregated statistics
- **Use Case**: Performance validation, detecting measurement variability

### Quick Testing Targets

#### `run_benchmarks_quick`
- **Purpose**: Fast execution with small data sizes only
- **Command**: `make run_benchmarks_quick`
- **Filter**: Array sizes 100 and 1000 only
- **Use Case**: Quick smoke tests, development iteration

### Type-Specific Targets

#### `run_benchmarks_int32`
- **Purpose**: Test integer primitive arrays only
- **Command**: `make run_benchmarks_int32`
- **Filter**: `.*int32.*`
- **Use Case**: Focused performance analysis of integer operations

#### `run_benchmarks_double`
- **Purpose**: Test floating-point primitive arrays only
- **Command**: `make run_benchmarks_double`
- **Filter**: `.*double.*`
- **Use Case**: Focused performance analysis of floating-point operations

#### `run_benchmarks_bool`
- **Purpose**: Test boolean primitive arrays only
- **Command**: `make run_benchmarks_bool`
- **Filter**: `.*bool.*`
- **Use Case**: Focused performance analysis of boolean operations

## Workflow Examples

### Development Workflow
```bash
# Quick smoke test during development
make run_benchmarks_quick

# Test specific type you're working on
make run_benchmarks_int32

# Full validation before commit
make run_benchmarks_detailed
```

### CI/CD Workflow
```bash
# Generate machine-readable results
make run_benchmarks_json

# Upload sparrow_benchmarks.json to build artifacts
# Compare with baseline performance data
```

### Performance Analysis Workflow
```bash
# Generate comprehensive data
make run_benchmarks_detailed

# Export for spreadsheet analysis  
make run_benchmarks_csv

# Analyze sparrow_benchmarks_detailed.json for trends
# Use sparrow_benchmarks.csv for graphing
```

## File Outputs

### Generated Files
- `sparrow_benchmarks.json` - Standard JSON results
- `sparrow_benchmarks.csv` - Standard CSV results  
- `sparrow_benchmarks_detailed.json` - Statistical analysis results

### File Locations
All output files are generated in the CMake build directory where the benchmarks are executed.

## Custom Execution

For custom benchmark execution beyond these targets, run the executable directly:

```bash
# Custom filter
./sparrow_benchmarks --benchmark_filter=".*ConstructFromVector.*"

# Custom repetitions
./sparrow_benchmarks --benchmark_repetitions=10

# Custom output format
./sparrow_benchmarks --benchmark_format=json --benchmark_out=custom_results.json

# Combine multiple options
./sparrow_benchmarks --benchmark_filter=".*int32.*" --benchmark_repetitions=3 --benchmark_format=console --benchmark_out_format=csv --benchmark_out=int32_results.csv
```
