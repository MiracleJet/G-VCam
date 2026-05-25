# src/impl — Platform Virtual Camera Drivers

| Directory | Platform | Artifacts |
|---|---|---|
| `win11/` | Windows 11 22H2+ | `GVCamSource.dll` (COM media source), `GVCamHost.exe` (camera registration host) |

### win11/ source files

| File | Purpose |
|---|---|
| `host.cpp` | Registers virtual camera via `MFCreateVirtualCamera` |
| `dllmain.cpp` | COM DLL entry points, class factory, `DllRegisterServer` |
| `activator.h` | `IMFActivate` implementation — instantiates the media source |
| `mediasource.h/.cpp` | `IMFMediaSourceEx` — owns the stream, manages lifecycle |
| `mediastream.h/.cpp` | `IMFMediaStream` — delivers NV12 samples from shared memory |
| `frame_server.h/.cpp` | Shared memory reader — `CreateFileMappingW` + `MapViewOfFile` |
| `logger.h` | Thread-safe logging to `C:\GVCam.log` |
| `gvcam_clsid.h` | COM CLSID `{C8D7E3A1-5BC9-4E92-98F3-2A4D6E8B1C7F}` |
