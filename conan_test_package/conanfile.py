from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.microsoft import is_msvc
import os
import sys


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VirtualRunEnv"
    test_type = "explicit"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            # Run the main test
            self.run(
                os.path.join(self.cpp.build.bindirs[0], "standalone"), env="conanrun"
            )

            # Run the json_reader test if it exists
            json_reader_test_path = os.path.join(
                self.cpp.build.bindirs[0], "json_reader_test"
            )
            if os.path.exists(json_reader_test_path) or os.path.exists(
                json_reader_test_path + ".exe"
            ):
                self.output.info("Running json_reader test")
                self.run(json_reader_test_path, env="conanrun")
            else:
                self.output.error("json_reader test not found")
                sys.exit(1)
