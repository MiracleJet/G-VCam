add_library(gvcam_compat STATIC)
target_include_directories(gvcam_compat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(gvcam_compat PRIVATE windows/ipc.c)
else()
    target_sources(gvcam_compat PRIVATE posix/ipc.c)
endif()
