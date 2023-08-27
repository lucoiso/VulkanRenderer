# Copyright Notices: [...]

from conan import ConanFile
from conan.tools.cmake import cmake_layout

class VulkanProjectRecipe(ConanFile):
    name = "vulkan-project"
    version = "0.0.1"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        # https://conan.io/center/recipes/glfw
        self.requires("glfw/[~3.3]")

        # https://conan.io/center/recipes/boost
        self.requires("boost/[~1.83]")

        # https://conan.io/center/recipes/benchmark
        self.requires("benchmark/[~1.8]")

        # https://conan.io/center/recipes/catch2
        self.requires("catch2/[~3.4]")

    def build_requirements(self):
        self.tool_requires("cmake/3.22.6")

    def layout(self):
        cmake_layout(self)