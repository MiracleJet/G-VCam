find_library(TURBOJPEG_LIB turbojpeg)
if(TURBOJPEG_LIB)
    find_package(libjpeg-turbo QUIET)
endif()

if(libjpeg-turbo_FOUND)
    add_library(gvcam_codec STATIC
        mjpeg_parser.c
        jpeg_decoder.c
    )
    target_include_directories(gvcam_codec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(gvcam_codec PUBLIC
        gvcam_core
        libjpeg-turbo::turbojpeg
    )
    message(STATUS "gvcam_codec: turbojpeg found — MJPEG/JPEG support enabled")
else()
    message(STATUS "gvcam_codec: turbojpeg NOT found — MJPEG/JPEG support disabled")
    message(STATUS "           Install libjpeg-turbo or use Conan to enable")
endif()
