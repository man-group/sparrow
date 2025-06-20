Development build                             {#dev_build}
=================

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

Building with mamba/micromamba
------------------------------

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

Running the tests
-----------------

To run the tests, you can either invoke the test binary directly:
```bash
./bin/Debug/test_sparrow_lib
```

or use ctest:
```bash
make test
```

Running the examples
--------------------
To run all examples toat once you can use the `run_examples` target:
```bash
make run_examples
```

Building the documentation
--------------------------
To build the documentation, you can use the `docs` target:
```bash
make docs
```

The documentation will be located in the `docs/html` folder in the build directory. You can open `docs/html/index.html` in your browser to view it.

