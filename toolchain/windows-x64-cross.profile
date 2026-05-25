[settings]
os=Windows
arch=x86_64
compiler=clang
compiler.version=18
compiler.cppstd=17
build_type=Release

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.cmake.cmaketoolchain:toolchain_file={{os.getcwd()}}/toolchain/windows-x64-clang.cmake
