// GVCamHost.exe — Registers the virtual camera via MFCreateVirtualCamera.
// Requires Windows 11 22H2+ and Administrator privileges.
#include <windows.h>
#include <mfvirtualcamera.h>
#include <mfapi.h>
#include <mfidl.h>
#include <wrl/client.h>
#include <initguid.h>
#include "gvcam_clsid.h"
#include <stdio.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfsensorgroup.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

static const WCHAR* SOURCE_CLSID = L"{C8D7E3A1-5BC9-4E92-98F3-2A4D6E8B1C7F}";

int wmain()
{
    printf("[GVCam] Initializing Virtual Camera Host...\n");

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
        printf("        Did you run 'regsvr32 GVCamSource.dll' as Administrator?\n");
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

    printf("\n"
           "========================================================\n"
           "  GVCam Virtual Camera is LIVE!\n"
           "========================================================\n"
           "  Visible in Zoom, Teams, OBS, Camera app, etc.\n"
           "  Run the companion to feed frames into the driver.\n"
           "  Press ENTER to stop.\n"
           "========================================================\n");

    getchar();

    printf("[GVCam] Shutting down...\n");
    virtualCamera->Stop();
    virtualCamera->Shutdown();
    virtualCamera.Reset();

    MFShutdown();
    CoUninitialize();
    return 0;
}
