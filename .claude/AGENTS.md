# G-VCam Project Conventions

## Project Overview

G-VCam turns a smartphone into a universal virtual camera for Windows/macOS/Linux.
Architecture: **Phone (MJPEG server) → TCP → Companion (decode/convert) → IPC → Platform virtual camera driver**.

## Build System

- **CMake 3.20+** with **Conan 2.x** for third-party deps
- C11 for platform-independent code, C++17 only where COM/WRL is required
- libjpeg-turbo is **optional** (only for `gvcam_codec`); build works without it
- `BUILD_IMPL_WINDOWS=ON` by default; `GVCAM_WITH_OPENCV=OFF`
- All commands must be run from the **project root**

### `conan build` — local dev (→ `output/`)

```bash
# Native Linux
conan build . -s build_type=Release

# Cross-compile Linux → Windows
export WIN_SDK_ROOT=/mnt/data/windows-dev-debug
conan build . -pr:h toolchain/windows-x64-cross.profile -pr:b default
```

Artifacts land in `output/`:
```
output/
  bin/  → GVCamSource.dll, GVCamHost.exe (Windows only)
  lib/  → gvcam_core, gvcam_compat
```

### `conan create` — packaging (→ Conan cache)

```bash
# Native Linux
conan create . -s build_type=Release

# Cross-compile Linux → Windows
export WIN_SDK_ROOT=/mnt/data/windows-dev-debug
conan create . -pr:h toolchain/windows-x64-cross.profile -pr:b default
```

### Bare CMake (cross-compile only)

```bash
export WIN_SDK_ROOT=/mnt/data/windows-dev-debug
cmake -B build/win11 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=toolchain/windows-x64-clang.cmake
cmake --build build/win11
```

### Switching build type

`conan build` and `conan create` share the same `build/` layout. When switching between them (or changing profiles / build_type), clean first:

```bash
rm -rf build/ output/
```

### toolchain/ directory

| File | Purpose |
|---|---|
| `windows-x64-clang.cmake` | Cross-compile toolchain; reads `$WIN_SDK_ROOT` from env |
| `windows-x64-cross.profile` | Conan host profile for Windows target |

## Source Tree

```
src/
  common/            — gvcam_core (frame.h/c, color_convert.h/c); C11, zero deps
  codec/             — gvcam_codec (mjpeg_parser, jpeg_decoder); optional turbojpeg
  compat/            — gvcam_compat
      ipc.h          — shared IPC API
      windows/ipc.c  — Windows shared-memory + mutex
      posix/ipc.c    — POSIX stubs
  impl/win11/        — Windows 11 virtual camera driver (C++/WRL, COM)
  export/            — .def files for DLL symbol exports
  app/               — phone-side camera apps (future)
```

## Coding Style

- **C code** (`src/common/`, `src/compat/`): C11, no comments unless the WHY is non-obvious. `snake_case` for functions, `PascalCase` for types when typedef'd. Public API returns `int` (0 = success, -1 = error).
- **C++ code** (`src/impl/win11/`): Follows WRL COM conventions — `STDMETHODIMP`, `ComPtr<>`, member `_prefix`. Deviate from WRL conventions only when obvious (e.g., `GVCamLog` for logging).
- No emojis, no docstrings, no "added for" or "used by" comments.
- Header guards: `GVCAM_FILENAME_H`.

## Key Design Rules

- **IPC struct (`gvcam_ipc_t`) is opaque** — full definition only in each platform's `ipc.c` (internal). Consumers store a pointer.
- **Frame descriptor layout** is `GVCAM_FRAME_DESC_SIZE` bytes (computed via sizeof). Producer and consumer must agree.
- **IPC channel names**: Windows uses `Global\` prefix for cross-session visibility.
- **COM CLSID**: `{C8D7E3A1-5BC9-4E92-98F3-2A4D6E8B1C7F}` — generated for GVCam, do not reuse.
- **Log file**: `C:\GVCam.log` (Session-0 accessible path).

## Dependencies

| Dependency | Required? | Managed by |
|---|---|---|
| libjpeg-turbo | Optional (codec only) | Conan |
| Windows SDK 10.0.26100+ | Yes (impl/win11) | System / cross-compile SDK |
| OpenCV | No (GVCAM_WITH_OPENCV=OFF) | — |

## Testing

- Build verification: `conan build . -s build_type=Release` (native) or cross-compile
- Cross-compile verification: check `file output/bin/*` shows `PE32+` for Windows artifacts
- Unit tests: `gcc -std=c11 -Isrc/common test.c src/common/frame.c src/common/color_convert.c`
- Runtime tests require Windows 11 22H2+ with Administrator privileges
