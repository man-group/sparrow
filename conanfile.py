from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import copy
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.scm import Version
import os


class SparrowRecipe(ConanFile):
    name = "sparrow"
    description = "C++20 idiomatic APIs for the Apache Arrow Columnar Format"
    license = "Apache-2.0"
    author = "Man Group"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/man-group/sparrow"
    topics = ("arrow", "header-only")
    settings = "os", "arch", "compiler", "build_type"
    package_type = "header-library"
    no_copy_source = True
    exports_sources = "include/*", "LICENSE"
    generators = "CMakeDeps"
    options = {
        "use_date_polyfill": [True, False],
        "generate_documentation": [True, False],
    }
    default_options = {
        "use_date_polyfill": True,
        "generate_documentation": False,
    }

    def requirements(self):
        if self.options.get_safe("use_date_polyfill"):
            self.requires("date/3.0.1#032e24ad8bd1fd136dd33c932563d3d1")
        self.test_requires("doctest/2.4.11")

    def build_requirements(self):
        if self.options.get_safe("generate_documentation"):
            self.tool_requires("doxygen/1.9.4", options={"enable_app": "True"})

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {
            "apple-clang": "13",
            "clang": "16",
            "gcc": "12",
            "msvc": "193"
        }

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)

        minimum_version = self._compilers_minimum_version.get(
            str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USE_DATE_POLYFILL"] = self.options.get_safe(
            "use_date_polyfill", False)
        tc.variables["BUILD_DOCS"] = self.options.get_safe(
            "generate_documentation", False)
        tc.generate()
        
    def package(self):
        copy(self, "LICENSE",
             dst=os.path.join(self.package_folder, "licenses"),
             src=self.source_folder)
        copy(self, "*.hpp", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()
