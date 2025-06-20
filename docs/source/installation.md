Installation         {#installation}
============

Using the conda-forge package
-----------------------------

A package for the mamba package manager is available on conda-forge:

```bash
mamba install sparrow -c conda-forge
```

Using the VCPKG package
-----------------------
A package for VCPKG is available:

https://vcpkg.io/en/package/arcticdb-sparrow

```bash
vcpkg install sparrow
```

Using the Conan package
-----------------------
A package for Conan is available:

https://conan.io/center/recipes/sparrow

```bash
conan install sparrow/X.X.X
```

Using the Fedora package
------------------------

A package for DNF, the Fedora package manager, is available:

```bash
dnf install sparrow
```

From source with cmake
----------------------

After cloning the [repo](https://github.com/man-group/sparrow), you can build and install
sparrow as described in the \ref dev_build section.
