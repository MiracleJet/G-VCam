# Core library — no external dependencies
add_library(gvcam_core STATIC
    frame.c
    color_convert.c
)
target_link_libraries(gvcam_core PUBLIC gvcam_headers)

# Codec library — depends on libjpeg-turbo (optional at build time)
find_package(libjpeg-turbo QUIET)

if(libjpeg-turbo_FOUND)
    add_library(gvcam_codec STATIC
        mjpeg_parser.c
        jpeg_decoder.c
    )
    target_link_libraries(gvcam_codec PUBLIC
        gvcam_core
        libjpeg-turbo::libjpeg-turbo
    )
    message(STATUS "gvcam_codec: turbojpeg found — MJPEG/JPEG support enabled")
else()
    message(STATUS "gvcam_codec: turbojpeg NOT found — MJPEG/JPEG support disabled")
    message(STATUS "           Install libjpeg-turbo or use Conan to enable")
endif()

# Alias for backward compat — common = core + codec (if available)
add_library(gvcam_common INTERFACE)
target_link_libraries(gvcam_common INTERFACE gvcam_core)
if(TARGET gvcam_codec)
    target_link_libraries(gvcam_common INTERFACE gvcam_codec)
endif()
