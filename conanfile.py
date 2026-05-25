from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy


class GVCamConan(ConanFile):
    name = "gvcam"
    version = "0.1.0"
    description = "Turn your phone into a universal virtual camera"
    license = "GPL-2.0-only"
    url = "https://github.com/GVCam/G-VCam"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "with_opencv": [True, False],
    }
    default_options = {
        "shared": True,
        "with_opencv": False,
    }

    def requirements(self):
        self.requires("libjpeg-turbo/3.1.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["GVCAM_WITH_OPENCV"] = self.options.with_opencv
        tc.variables["BUILD_IMPL_WINDOWS"] = True
        tc.generate()
