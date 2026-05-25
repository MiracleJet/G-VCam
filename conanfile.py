from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout


class GVCamConan(ConanFile):
    name = "gvcam"
    version = "0.1.0"
    description = "Turn your phone into a universal virtual camera"
    license = "GPL-2.0-only"
    url = "https://github.com/MiracleJet/G-VCam"

    exports_sources = "CMakeLists.txt", "src/**"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_jpeg":   [True, False],
        "with_opencv": [True, False],
    }
    default_options = {
        "with_jpeg":   False,
        "with_opencv": False,
    }

    def requirements(self):
        if self.options.with_jpeg:
            self.requires("libjpeg-turbo/3.1.0")
        if self.options.with_opencv:
            self.requires("opencv/4.10.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        is_windows = self.settings.os == "Windows"
        tc = CMakeToolchain(self)
        tc.variables["GVCAM_WITH_OPENCV"] = self.options.with_opencv
        tc.variables["BUILD_IMPL_WINDOWS"] = is_windows
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
