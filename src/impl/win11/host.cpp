// GVCamHost.exe — Registers the virtual camera via MFCreateVirtualCamera.
// Requires Windows 11 22H2+ and Administrator privileges.
#include <windows.h>
#include <mfvirtualcamera.h>
#include <mfapi.h>
#include <mfidl.h>
#include <wrl/client.h>
#include <initguid.h>
#include "gvcam_clsid.h"
#include "mjpeg_bridge.h"
#include <stdio.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfsensorgroup.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

static const WCHAR* SOURCE_CLSID = L"{C8D7E3A1-5BC9-4E92-98F3-2A4D6E8B1C7F}";
static WCHAR g_dllPath[MAX_PATH];

typedef HRESULT (STDAPICALLTYPE *RegisterFn)(void);

static HRESULT SelfRegister()
{
    WCHAR exePath[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return E_FAIL;

    WCHAR *lastSep = wcsrchr(exePath, L'\\');
    if (!lastSep) return E_FAIL;
    *(lastSep + 1) = L'\0';
    wcscpy_s(g_dllPath, MAX_PATH, exePath);
    wcscat_s(g_dllPath, MAX_PATH, L"GVCamSource.dll");

    HMODULE dll = LoadLibraryW(g_dllPath);
    if (!dll) {
        printf("[WARN]  Cannot load %ls (error %lu)\n", g_dllPath, GetLastError());
        return E_FAIL;
    }

    RegisterFn fn = (RegisterFn)GetProcAddress(dll, "DllRegisterServer");
    HRESULT hr = E_FAIL;
    if (fn) {
        hr = fn();
        if (SUCCEEDED(hr))
            printf("[OK]   Registered %ls\n", g_dllPath);
        else
            printf("[WARN] DllRegisterServer failed: 0x%08lX\n", (long)hr);
    } else {
        printf("[WARN] DllRegisterServer not found\n");
    }

    FreeLibrary(dll);
    return hr;
}

static void SelfUnregister()
{
    if (g_dllPath[0] == L'\0') return;

    HMODULE dll = LoadLibraryW(g_dllPath);
    if (!dll) return;

    RegisterFn fn = (RegisterFn)GetProcAddress(dll, "DllUnregisterServer");
    if (fn) {
        fn();
        printf("[OK]   Unregistered %ls\n", g_dllPath);
    }

    FreeLibrary(dll);
}

int wmain(int argc, wchar_t *argv[])
{
    printf("[GVCam] Initializing Virtual Camera Host...\n");

    const char *androidIp = "127.0.0.1";
    if (argc >= 2) {
        // Convert wide-char argument to ANSI
        static char ipBuf[64];
        WideCharToMultiByte(CP_ACP, 0, argv[1], -1, ipBuf, sizeof(ipBuf), nullptr, nullptr);
        androidIp = ipBuf;
    }
    printf("[GVCam] Android IP: %s\n", androidIp);

    printf("[GVCam] Auto-registering GVCamSource.dll ...\n");
    SelfRegister();

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { printf("[ERROR] CoInitializeEx: 0x%08lX\n", (long)hr); return 1; }

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { printf("[ERROR] MFStartup: 0x%08lX\n", (long)hr); return 1; }

    BOOL supported = FALSE;
    hr = MFIsVirtualCameraTypeSupported(MFVirtualCameraType_SoftwareCameraSource, &supported);
    if (FAILED(hr) || !supported) {
        printf("[ERROR] Virtual cameras not supported. Need Windows 11 22H2+.\n");
        MFShutdown();
        CoUninitialize();
        return 1;
    }
    printf("[OK] System supports virtual cameras.\n");

    ComPtr<IMFVirtualCamera> virtualCamera;
    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"GVCam Virtual Camera",
        SOURCE_CLSID,
        NULL, 0,
        &virtualCamera);

    if (FAILED(hr)) {
        printf("[ERROR] MFCreateVirtualCamera: 0x%08lX\n", (long)hr);
        printf("        Ensure GVCamSource.dll is in the same directory "
               "and you are running as Administrator.\n");
        MFShutdown();
        CoUninitialize();
        return 1;
    }
    printf("[OK] Virtual camera created.\n");

    hr = virtualCamera->Start(NULL);
    if (FAILED(hr)) {
        printf("[ERROR] Failed to start: 0x%08lX\n", (long)hr);
        virtualCamera->Shutdown();
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    HANDLE hBridge = StartMjpegBridge(androidIp);

    printf("\n"
           "========================================================\n"
           "  GVCam Virtual Camera is LIVE!\n"
           "========================================================\n"
           "  Visible in Zoom, Teams, OBS, Camera app, etc.\n"
           "  MJPEG bridge started (port 8080).\n"
           "  Press ENTER to stop.\n"
           "========================================================\n");

    getchar();

    printf("[GVCam] Shutting down...\n");
    if (hBridge) {
        TerminateThread(hBridge, 0);
        CloseHandle(hBridge);
    }
    virtualCamera->Stop();
    virtualCamera->Shutdown();
    virtualCamera.Reset();

    MFShutdown();
    CoUninitialize();

    SelfUnregister();
    return 0;
}
