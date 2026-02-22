#include <windows.h>
#include <stdio.h>
#include "diaCreate.h"

typedef HRESULT(__stdcall* pDllGetClassObject)(
    _In_  REFCLSID rclsid,
    _In_  REFIID   riid,
    _Out_ LPVOID*   ppv
);

static wchar_t g_debugInfo[1024] = {0};

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
    
    swprintf_s(tempInfo, 512, L"Trying to load: %s", dllName);
    SetDebugInfo(tempInfo);

    HMODULE hModule = LoadLibraryExW(dllName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    
    if (!hModule)
    {
        DWORD err = GetLastError();
        swprintf_s(tempInfo, 512, L"LoadLibraryEx failed: %s, error: %lu", dllName, err);
        SetDebugInfo(tempInfo);
        hr = HRESULT_FROM_WIN32(err);
        return hr;
    }

    swprintf_s(tempInfo, 512, L"Successfully loaded: %s", dllName);
    SetDebugInfo(tempInfo);

    pDllGetClassObject DllGetClassObject;
    DllGetClassObject = (pDllGetClassObject)GetProcAddress(hModule, "DllGetClassObject");
    
    if (!DllGetClassObject)
    {
        DWORD err = GetLastError();
        swprintf_s(tempInfo, 512, L"GetProcAddress(DllGetClassObject) failed, error: %lu", err);
        SetDebugInfo(tempInfo);
        hr = HRESULT_FROM_WIN32(err);
        FreeLibrary(hModule);
        return hr;
    }

    swprintf_s(tempInfo, 512, L"Successfully found DllGetClassObject");
    SetDebugInfo(tempInfo);

    IClassFactory* classFactory;
    hr = DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&classFactory);
    
    if (FAILED(hr))
    {
        swprintf_s(tempInfo, 512, L"DllGetClassObject failed, hr: 0x%08X", hr);
        SetDebugInfo(tempInfo);
        FreeLibrary(hModule);
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
        FreeLibrary(hModule);
        return hr;
    }

    classFactory->AddRef();
    
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
