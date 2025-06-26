# Development build                             {#dev_build}

Here we describe how to build the project for development purposes on **Linux** or **macOS**.
For **Windows**, the instructions are similar.

List of CMake options:
- `ACTIVATE_LINTER`: Create targets to run clang-format (default: OFF)
- `ACTIVATE_LINTER_DURING_COMPILATION`: Run linter during the compilation (default: OFF), 
  requires `ACTIVATE_LINTER` to be ON
- `BUILD_DOCS`: Build the documentation (default: OFF)
- `BUILD_EXAMPLES`: Build the examples (default: OFF)
- `BUILD_TESTS`: Build the tests (default: OFF)
- `ENABLE_COVERAGE`: Enable coverage reporting (default: OFF)
- `ENABLE_INTEGRATION_TEST`: Enable integration tests (default: OFF)
- `SPARROW_BUILD_SHARED`: "Build sparrow as a shared library (default: ON)
- `USE_DATE_POLYFILL`: Use date polyfill implementation (default: OFF)
- `USE_LARGE_INT_PLACEHOLDERS`: Use types without api for big integers 'ON' by default on 32 bit systems and MSVC compilers (default: ON on 32 bit systems and MSVC, OFF otherwise)
- `USE_SANITIZER`: Enable sanitizer(s). Options are: address;leak;memory;thread;undefined (default: )

## Building

### ... with mamba/micromamba

First we create a conda environment with all required development dependencies: 

```bash
mamba env create -f environment-dev.yml
```

Then we activate the environment:

```bash 
mamba activate sparrow
```

create a build directory and run cmake from it:

```bash
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX \
    ..
```

And finally, build the project:

```bash
make -j12
```

### ... with VCPKG
If you prefer to use VCPKG, you can follow these steps:
First, install the required dependencies using VCPKG:



### ...with Conan

If you prefer to use Conan, you can follow these steps:
First, install the required dependencies using Conan:

```bash
conan install .. --build=missing -s:a compiler.cppstd=20 -o:a use_date_polyfill=True -o:a build_tests=True ...
```
Available conan options:
- `use_date_polyfill`: Use date polyfill implementation (default: False)
- `build_tests`: Build the tests (default: False)
- `generate_documentation`: Generate documentation (default: False)

Then, run the cmake configuration

```bash
mkdir build
cd build
cmake --preset conan-release 
    ..
```

## Running the tests

To run the tests, the easy way is to use the cmake targets`:
- `run_tests`: Runs all tests without JUnit report
- `run_tests_with_junit_report`: Runs all tests and generates a JUnit report in the `test-reports` directory
- `run_integration_tests`: Runs the integration tests
- `run_integration_tests_with_junit_report`: Runs the integration tests and generates a JUnit report in the `test-reports` directory


```bash
cmake --build . --config [Release/Debug/...] --target [TARGET_NAME]
```

Running the examples
--------------------
To run all examples:
```bash
cmake --build . --target run_examples
```

Building the documentation
--------------------------
To build the documentation, you can use the `docs` target:
```bash
cmake --build . --target docs
```

The documentation will be located in the `docs/html` folder in the build directory. You can open `docs/html/index.html` in your browser to view it.
