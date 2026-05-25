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

# ── Common library (platform-independent, zero deps) ─────────
add_subdirectory(src/common)

# ── Codec library (MJPEG/JPEG, optional turbojpeg) ───────────
add_subdirectory(src/codec)

# ── Compat library (platform IPC) ────────────────────────────
add_subdirectory(src/compat)

# ── Platform implementations ─────────────────────────────────
if(BUILD_IMPL_WINDOWS)
    add_subdirectory(src/impl/win11)
endif()

# ── Install ──────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS gvcam_core gvcam_compat
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(TARGET gvcam_codec)
    install(TARGETS gvcam_codec
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(BUILD_IMPL_WINDOWS)
    install(TARGETS GVCamSource GVCamHost
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
