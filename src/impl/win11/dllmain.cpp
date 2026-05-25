// DllMain.cpp — COM DLL Entry Points & Registration for GVCam
#include <windows.h>
#include <new>
#include <initguid.h>
#include "gvcam_clsid.h"
#include "activator.h"
#include "logger.h"

static HMODULE g_hModule  = NULL;
static LONG    g_objCount = 0;

class GVCamClassFactory : public IClassFactory
{
    LONG _refCount = 1;

public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&_refCount); }
    STDMETHODIMP_(ULONG) Release() override {
        LONG ref = InterlockedDecrement(&_refCount);
        if (ref == 0) delete this;
        return ref;
    }

    STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override
    {
        GVCamLog("GVCamClassFactory::CreateInstance");
        if (pOuter) return CLASS_E_NOAGGREGATION;
        if (!ppv)   return E_POINTER;
        *ppv = NULL;

        auto activator = Microsoft::WRL::Make<GVCamActivator>();
        if (!activator) return E_OUTOFMEMORY;

        HRESULT hr = activator->RuntimeClassInitialize();
        if (FAILED(hr)) return hr;

        InterlockedIncrement(&g_objCount);
        hr = activator->QueryInterface(riid, ppv);
        if (FAILED(hr)) InterlockedDecrement(&g_objCount);
        return hr;
    }

    STDMETHODIMP LockServer(BOOL fLock) override
    {
        if (fLock) InterlockedIncrement(&g_objCount);
        else       InterlockedDecrement(&g_objCount);
        return S_OK;
    }
};

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hModule = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        GVCamLog("GVCamSource.dll loaded");
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = NULL;

    if (rclsid != CLSID_GVCamMediaSource)
        return CLASS_E_CLASSNOTAVAILABLE;

    auto factory = new (std::nothrow) GVCamClassFactory();
    if (!factory) return E_OUTOFMEMORY;

    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_objCount == 0) ? S_OK : S_FALSE;
}

static const WCHAR* CLSID_STRING = L"{C8D7E3A1-5BC9-4E92-98F3-2A4D6E8B1C7F}";

STDAPI DllRegisterServer()
{
    WCHAR dllPath[MAX_PATH];
    GetModuleFileNameW(g_hModule, dllPath, MAX_PATH);

    HKEY hKey = NULL;
    WCHAR keyPath[256];

    wsprintfW(keyPath, L"SOFTWARE\\Classes\\CLSID\\%s", CLSID_STRING);
    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS) return HRESULT_FROM_WIN32(status);

    const WCHAR desc[] = L"GVCam Virtual Camera Source";
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)desc, sizeof(desc));
    RegCloseKey(hKey);

    wsprintfW(keyPath, L"SOFTWARE\\Classes\\CLSID\\%s\\InprocServer32", CLSID_STRING);
    status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS) return HRESULT_FROM_WIN32(status);

    RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)dllPath, (DWORD)((wcslen(dllPath) + 1) * sizeof(WCHAR)));

    const WCHAR threading[] = L"Both";
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)threading, sizeof(threading));
    RegCloseKey(hKey);

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    WCHAR keyPath[256];
    wsprintfW(keyPath, L"SOFTWARE\\Classes\\CLSID\\%s", CLSID_STRING);
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, keyPath);
    return S_OK;
}
