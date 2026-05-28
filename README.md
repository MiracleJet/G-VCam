# G-VCam — Universal Virtual Camera

Turn your smartphone into a Windows virtual camera. The phone captures video, encodes to MJPEG, and streams over TCP. The PC side decodes frames and feeds them into a virtual camera driver visible to Zoom, OBS, Teams, etc.

```
Phone (CameraX → JPEG → MJPEG/TCP) ──WiFi/USB──→ PC (GVCamHost → WIC decode → NV12 → Shared Memory → Virtual Camera Driver)
```

## Architecture

| Component | Location | Role |
|---|---|---|
| Android App | `src/app/GVCam/` | Camera capture + MJPEG TCP server (port 8080) |
| GVCamSource.dll | `src/impl/win11/` | COM virtual camera driver, runs in Session 0 |
| GVCamHost.exe | `src/impl/win11/` | Registers camera, bridges MJPEG stream to shared memory |
| Shared libraries | `src/common/`, `src/codec/`, `src/compat/` | Frame buffer sizing, color conversion, MJPEG parsing, IPC |

## Directory Structure

```
src/
  common/         C11 core (frame.h/c, color_convert.h/c) — zero dependencies
  codec/          MJPEG/JPEG decoder — optional libjpeg-turbo
  compat/         Platform IPC (Windows shared memory, POSIX stub)
  impl/win11/     Windows 11 virtual camera driver + MJPEG bridge host
  app/GVCam/      Android Compose Multiplatform app
  export/         DLL symbol exports
toolchain/        Cross-compilation toolchain + Conan profile
script/debug/     Debug tools (see below)
```

## Build

### PC (Windows driver + host)

Cross-compile from Linux:

```bash
# Bare CMake
export WIN_SDK_ROOT=/mnt/data/windows-dev-debug
cmake -B build/win11 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=toolchain/windows-x64-clang.cmake
cmake --build build/win11

# Conan (output → output/)
conan build . -pr:h toolchain/windows-x64-cross.profile -pr:b default
```

Native Linux (core libraries only):

```bash
conan build . -s build_type=Release
```

### Android App

```bash
cd src/app/GVCam
JAVA_HOME=/usr/lib/jvm/java-25-openjdk ./gradlew :androidApp:assembleDebug
# APK → androidApp/build/outputs/apk/debug/androidApp-debug.apk
```

## Usage

### 1. WiFi (no ADB needed)

- Phone and PC on same WiFi
- Install APK on phone, open GVCam app → note the **Device IP** displayed
- Copy `GVCamHost.exe` + `GVCamSource.dll` to Windows PC
- Run as Administrator:
  ```
  GVCamHost.exe 192.168.1.100
  ```
- Open Zoom/OBS → select "GVCam Virtual Camera"

### 2. USB + ADB

- Phone connected via USB, USB debugging ON
- `adb forward tcp:8080 tcp:8080`
- Run `GVCamHost.exe` (no arguments, defaults to 127.0.0.1)

### 3. Test pattern (no phone)

```bash
# Windows, with Python + numpy + opencv
python script/debug/reference_companion.py
```
Generates a moving rectangle test pattern directly into shared memory.

## Debug Scripts

| Script | Purpose |
|---|---|
| `script/debug/debug_mjpeg.py` | Connect to MJPEG stream, save JPEG frames, show FPS |
| `script/debug/reference_companion.py` | Generate test pattern → shared memory (no phone needed) |

```bash
python script/debug/debug_mjpeg.py                    # save 5 frames
python script/debug/debug_mjpeg.py --infinite         # live stats only
python script/debug/debug_mjpeg.py --url http://192.168.1.100:8080  # WiFi
```

## Requirements

| Component | Requirement |
|---|---|
| Windows | Windows 11 22H2+, Administrator |
| Android | API 26+ |
| Cross-compile | clang-cl, lld-link, Windows SDK 10.0.26100+ |
| Python scripts | numpy, opencv-python |
