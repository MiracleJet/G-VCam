# G-VCam Project Conventions

## Project Overview

G-VCam turns a smartphone into a universal virtual camera for Windows/macOS/Linux.
Architecture: **Phone (MJPEG server) → TCP → Companion (decode/convert) → IPC → Platform virtual camera driver**.

## Build System

- **CMake 3.20+** with **Conan 2.x** for third-party deps
- C11 for platform-independent code, C++17 only where COM/WRL is required
- Cross-compile from Linux to Windows: `cmake -B build/win11 -DCMAKE_TOOLCHAIN_FILE=.../windows-msvc-cross.amd64 -DWIN_SDK_ROOT=/mnt/data/windows-dev-debug -G Ninja`
- `BUILD_IMPL_WINDOWS=ON` by default; `GVCAM_WITH_OPENCV=OFF`
- libjpeg-turbo is **optional** (only for `gvcam_codec`); build works without it

## Source Tree

```
src/
  include/gvcam/    — public headers (frame.h, ipc.h); pure C, no platform deps
  common/            — gvcam_core (frame.c, color_convert.c — no deps)
                       gvcam_codec (mjpeg_parser.c, jpeg_decoder.c — needs turbojpeg, optional)
  compat/            — platform IPC (ipc_windows.c, future ipc_posix.c)
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

- **IPC struct (`gvcam_ipc_t`) is opaque** — full definition only in `compat/ipc_windows.h` (internal, not installed). Consumers store a pointer.
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

- Cross-compile verification: `cmake --build build/win11` produces `GVCamHost.exe` + `GVCamSource.dll` (PE32+ x86-64)
- Unit tests: `src/common/` can be natively compiled on Linux with `gcc -std=c11 -Isrc/include -Isrc/common test.c src/common/frame.c src/common/color_convert.c`
- Runtime tests require Windows 11 22H2+ with Administrator privileges
