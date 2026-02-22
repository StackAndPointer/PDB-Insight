#include "DPIManager.h"

int DPIManager::s_dpi = 96;
float DPIManager::s_scaleX = 1.0f;
float DPIManager::s_scaleY = 1.0f;

void DPIManager::Initialize() {
    HDC hDC = GetDC(nullptr);
    if (hDC) {
        s_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(nullptr, hDC);
    }
    
    s_scaleX = static_cast<float>(s_dpi) / 96.0f;
    s_scaleY = static_cast<float>(s_dpi) / 96.0f;
}

void DPIManager::Cleanup() {
}

int DPIManager::GetDPI() {
    return s_dpi;
}

int DPIManager::ScaleX(int value) {
    return static_cast<int>(value * s_scaleX);
}

int DPIManager::ScaleY(int value) {
    return static_cast<int>(value * s_scaleY);
}

int DPIManager::UnscaleX(int value) {
    return static_cast<int>(value / s_scaleX);
}

int DPIManager::UnscaleY(int value) {
    return static_cast<int>(value / s_scaleY);
}

void DPIManager::SetProcessDPIAware() {
    typedef BOOL(WINAPI* SetProcessDPIAwareFunc)();
    
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32) {
        SetProcessDPIAwareFunc pFunc = 
            (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (pFunc) {
            pFunc();
        }
        FreeLibrary(hUser32);
    }
}

void DPIManager::AdjustWindowRectForDPI(LPRECT rect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle) {
    AdjustWindowRectEx(rect, dwStyle, bMenu, dwExStyle);
    
    int width = rect->right - rect->left;
    int height = rect->bottom - rect->top;
    
    rect->right = rect->left + ScaleX(width);
    rect->bottom = rect->top + ScaleY(height);
}
