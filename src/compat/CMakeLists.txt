if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(gvcam_compat STATIC ipc_windows.c)
else()
    add_library(gvcam_compat STATIC ipc_posix.c)
endif()

target_link_libraries(gvcam_compat PUBLIC gvcam_headers)
target_include_directories(gvcam_compat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
