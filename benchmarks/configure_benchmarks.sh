#!/bin/bash

# Sparrow Benchmark Configuration Script
# This script helps configure and build Sparrow with benchmarks enabled

set -e

echo "🏁 Configuring Sparrow with benchmarks..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Please run this script from the Sparrow root directory"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

echo "🔧 Running CMake configuration..."

# Configure with benchmarks enabled
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_TESTS=OFF \
    -DFETCH_DEPENDENCIES_WITH_CMAKE=ON

echo "🏗️  Building benchmarks..."
cmake --build . --target sparrow_benchmarks --parallel

echo "✅ Benchmarks built successfully!"
echo ""
echo "Available CMake targets:"
echo "  make run_benchmarks                    # Console output only"
echo "  make run_benchmarks_json              # JSON output to file"  
echo "  make run_benchmarks_csv               # CSV output to file"
echo "  make run_benchmarks_console_json      # Console + JSON file"
echo "  make run_benchmarks_detailed          # 5 repetitions with statistics"
echo "  make run_benchmarks_quick             # Quick test (small sizes)"
echo "  make benchmarks_help                  # Show all available targets"
echo ""
echo "Or run directly:"
echo "  ./bin/Release/sparrow_benchmarks"
echo ""
echo "With custom options:"
echo "  ./bin/Release/sparrow_benchmarks --benchmark_filter=\".*int32.*\""
echo "  ./bin/Release/sparrow_benchmarks --benchmark_format=json --benchmark_out=results.json"
