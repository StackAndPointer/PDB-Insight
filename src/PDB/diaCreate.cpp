#include "diaCreate.h"
#include <windows.h>
#include <stdio.h>
#include <mutex>

typedef HRESULT(__stdcall* pDllGetClassObject)(
    _In_  REFCLSID rclsid,
    _In_  REFIID   riid,
    _Out_ LPVOID*   ppv
);

static thread_local wchar_t g_debugInfo[1024] = {0};
static HMODULE g_diaModule = nullptr;
static pDllGetClassObject g_dllGetClassObject = nullptr;
static std::mutex g_diaModuleMutex;

void SetDebugInfo(const wchar_t* info)
{
    wcscpy_s(g_debugInfo, 1024, info);
}

const wchar_t* GetDebugInfo()
{
    return g_debugInfo;
}

HRESULT STDMETHODCALLTYPE NoRegCoCreate(const __wchar_t* dllName,
                                        REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv)
{
    HRESULT hr;
    wchar_t tempInfo[512];

    std::lock_guard<std::mutex> lock(g_diaModuleMutex);
    if (!g_diaModule) {
        swprintf_s(tempInfo, 512, L"Trying to load: %s", dllName);
        SetDebugInfo(tempInfo);

        g_diaModule = LoadLibraryExW(dllName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!g_diaModule) {
            DWORD err = GetLastError();
            swprintf_s(tempInfo, 512, L"LoadLibraryEx failed: %s, error: %lu", dllName, err);
            SetDebugInfo(tempInfo);
            return HRESULT_FROM_WIN32(err);
        }

        g_dllGetClassObject = reinterpret_cast<pDllGetClassObject>(
            GetProcAddress(g_diaModule, "DllGetClassObject"));
        if (!g_dllGetClassObject) {
            DWORD err = GetLastError();
            FreeLibrary(g_diaModule);
            g_diaModule = nullptr;
            swprintf_s(tempInfo, 512, L"GetProcAddress(DllGetClassObject) failed, error: %lu", err);
            SetDebugInfo(tempInfo);
            return HRESULT_FROM_WIN32(err);
        }
    }

    swprintf_s(tempInfo, 512, L"Successfully found DllGetClassObject");
    SetDebugInfo(tempInfo);

    IClassFactory* classFactory;
    hr = g_dllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&classFactory);
    
    if (FAILED(hr))
    {
        swprintf_s(tempInfo, 512, L"DllGetClassObject failed, hr: 0x%08X", hr);
        SetDebugInfo(tempInfo);
        return hr;
    }

    swprintf_s(tempInfo, 512, L"Successfully got class factory");
    SetDebugInfo(tempInfo);

    hr = classFactory->CreateInstance(nullptr, riid, ppv);
    
    if (FAILED(hr))
    {
        swprintf_s(tempInfo, 512, L"CreateInstance failed, hr: 0x%08X", hr);
        SetDebugInfo(tempInfo);
        classFactory->Release();
        return hr;
    }

    classFactory->Release();

    swprintf_s(tempInfo, 512, L"Successfully created DIA data source from: %s", dllName);
    SetDebugInfo(tempInfo);
    
    return hr;
}

HRESULT STDMETHODCALLTYPE NoOleCoCreate(REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv)
{
    return CoCreateInstance(rclsid, nullptr, CLSCTX_INPROC_SERVER, riid, ppv);
}
