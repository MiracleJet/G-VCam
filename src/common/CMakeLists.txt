add_library(gvcam_core STATIC
    frame.c
    color_convert.c
)
target_include_directories(gvcam_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
