# ── Windows SDK path resolution ────────────────────────────
if(DEFINED WIN_SDK_ROOT)
    set(WIN_SDK_LIB     "${WIN_SDK_ROOT}/sdk/lib/um/x86_64")
    set(WIN_SDK_INC     "${WIN_SDK_ROOT}/sdk/include/um")
    set(WIN_SDK_SHARED  "${WIN_SDK_ROOT}/sdk/include/shared")
    set(WIN_CRT_LIB     "${WIN_SDK_ROOT}/crt/lib/x86_64")
    set(WIN_UCRT_LIB    "${WIN_SDK_ROOT}/sdk/lib/ucrt/x86_64")
    set(WIN_LINK_DIRS   "${WIN_SDK_LIB}" "${WIN_CRT_LIB}" "${WIN_UCRT_LIB}")
else()
    set(WIN_SDK_LIB     "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64")
    set(WIN_SDK_INC     "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um")
    set(WIN_SDK_SHARED  "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared")
    set(WIN_LINK_DIRS   "${WIN_SDK_LIB}")
endif()

# ── Compiler definitions ────────────────────────────────────
add_compile_definitions(
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A00000B
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
)

set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${WIN_SDK_INC}"
    "${WIN_SDK_SHARED}"
)

# ── GVCamSource.dll — Virtual Camera COM DLL ────────────────
add_library(GVCamSource SHARED
    dllmain.cpp
    mediasource.cpp
    mediastream.cpp
    frame_server.cpp
    ${CMAKE_SOURCE_DIR}/src/export/gvcam_source.def
)
target_include_directories(GVCamSource PRIVATE ${COMMON_INCLUDES})
target_link_directories(GVCamSource PRIVATE ${WIN_LINK_DIRS})
target_link_libraries(GVCamSource PRIVATE
    gvcam_core
    gvcam_compat
    mf.lib
    mfplat.lib
    mfuuid.lib
    user32.lib
    ole32.lib
    advapi32.lib
)

# ── GVCamHost.exe — Camera registration host ────────────────
add_executable(GVCamHost
    host.cpp
    mjpeg_bridge.cpp
)
target_include_directories(GVCamHost PRIVATE ${COMMON_INCLUDES})
target_link_directories(GVCamHost PRIVATE ${WIN_LINK_DIRS})
target_link_libraries(GVCamHost PRIVATE
    gvcam_core
    gvcam_compat
    mf.lib
    mfplat.lib
    mfuuid.lib
    mfsensorgroup.lib
    ole32.lib
    ws2_32.lib
    windowscodecs.lib
)
