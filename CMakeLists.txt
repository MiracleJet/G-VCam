cmake_minimum_required(VERSION 3.20)
project(GVCam
    VERSION 0.1.0
    DESCRIPTION "Universal virtual camera — phone as webcam"
    LANGUAGES C CXX
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ── Options ──────────────────────────────────────────────────
option(GVCAM_WITH_OPENCV   "Use OpenCV for color conversion" OFF)
option(BUILD_IMPL_WINDOWS  "Build Windows virtual camera driver" ON)

# Note: libjpeg-turbo is optional — required only for gvcam_codec (MJPEG/JPEG).
# Install via Conan or system package manager to enable it.

# ── Public headers ───────────────────────────────────────────
add_library(gvcam_headers INTERFACE)
target_include_directories(gvcam_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# ── Common library (platform-independent) ────────────────────
add_subdirectory(src/common)

# ── Compat library (platform abstraction) ────────────────────
add_subdirectory(src/compat)

# ── Platform implementations ─────────────────────────────────
if(BUILD_IMPL_WINDOWS)
    add_subdirectory(src/impl/win11)
endif()
