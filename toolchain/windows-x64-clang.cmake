# windows-x64-clang.cmake — cross-compile from Linux to Windows x86-64 via clang-cl
#
# Usage:
#   export WIN_SDK_ROOT=/path/to/windows-sdk
#   cmake -B build/win11 -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=toolchain/windows-x64-clang.cmake

if(NOT DEFINED ENV{WIN_SDK_ROOT})
    message(FATAL_ERROR "WIN_SDK_ROOT environment variable is not set. "
        "Export it to your Windows SDK root, e.g.:\n"
        "  export WIN_SDK_ROOT=/mnt/data/windows-dev-debug")
endif()

set(WIN_SDK_ROOT "$ENV{WIN_SDK_ROOT}")

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_CROSSCOMPILING ON)

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_MT llvm-mt)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_INIT "/vctoolsdir ${WIN_SDK_ROOT}/crt /winsdkdir ${WIN_SDK_ROOT}/sdk")
set(CMAKE_CXX_FLAGS_INIT "/vctoolsdir ${WIN_SDK_ROOT}/crt /winsdkdir ${WIN_SDK_ROOT}/sdk")

set(CRT_LIBPATHS "/libpath:${WIN_SDK_ROOT}/crt/lib/x86_64 /libpath:${WIN_SDK_ROOT}/sdk/lib/um/x86_64 /libpath:${WIN_SDK_ROOT}/sdk/lib/ucrt/x86_64")
set(CMAKE_EXE_LINKER_FLAGS_INIT     "${CRT_LIBPATHS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT  "${CRT_LIBPATHS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT  "${CRT_LIBPATHS}")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
