Development build                             {#dev_build}
=================

Here we describe how to build the project for development purposes on **Linux** or **macOS**.
For **Windows**, the instructions are similar.

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
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON \
    -BUILD_DOCS=ON \
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

