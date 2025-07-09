# Sparrow Benchmark Configuration Script (PowerShell)
# This script helps configure and build Sparrow with benchmarks enabled

$ErrorActionPreference = "Stop"

Write-Host "🏁 Configuring Sparrow with benchmarks..." -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "❌ Error: Please run this script from the Sparrow root directory" -ForegroundColor Red
    exit 1
}

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

Set-Location build

Write-Host "🔧 Running CMake configuration..." -ForegroundColor Yellow

# Configure with benchmarks enabled
cmake .. `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_BENCHMARKS=ON `
    -DBUILD_TESTS=OFF `
    -DFETCH_DEPENDENCIES_WITH_CMAKE=ON

Write-Host "🏗️  Building benchmarks..." -ForegroundColor Yellow
cmake --build . --target sparrow_benchmarks --parallel

Write-Host "✅ Benchmarks built successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Available CMake targets:"
Write-Host "  make run_benchmarks                    # Console output only"
Write-Host "  make run_benchmarks_json              # JSON output to file"
Write-Host "  make run_benchmarks_csv               # CSV output to file"
Write-Host "  make run_benchmarks_console_json      # Console + JSON file"
Write-Host "  make run_benchmarks_detailed          # 5 repetitions with statistics"
Write-Host "  make run_benchmarks_quick             # Quick test (small sizes)"
Write-Host "  make benchmarks_help                  # Show all available targets"
Write-Host ""
Write-Host "Or run directly:"
Write-Host "  .\bin\Release\sparrow_benchmarks.exe"
Write-Host ""
Write-Host "With custom options:"
Write-Host "  .\bin\Release\sparrow_benchmarks.exe --benchmark_filter=`".*int32.*`""
Write-Host "  .\bin\Release\sparrow_benchmarks.exe --benchmark_format=json --benchmark_out=results.json"
