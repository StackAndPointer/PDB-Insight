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
    UpdateScale();
}

void DPIManager::Cleanup() {
}

void DPIManager::UpdateScale() {
    s_scaleX = static_cast<float>(s_dpi) / 96.0f;
    s_scaleY = s_scaleX;
}

int DPIManager::GetDPI() {
    return s_dpi;
}

void DPIManager::SetDPI(int dpi) {
    s_dpi = max(96, dpi);
    UpdateScale();
}

int DPIManager::ScaleX(int value) {
    return MulDiv(value, s_dpi, 96);
}

int DPIManager::ScaleY(int value) {
    return MulDiv(value, s_dpi, 96);
}

int DPIManager::UnscaleX(int value) {
    return MulDiv(value, 96, s_dpi);
}

int DPIManager::UnscaleY(int value) {
    return MulDiv(value, 96, s_dpi);
}

void DPIManager::SetProcessDPIAware() {
    using SetProcessDpiAwarenessContextFunc = BOOL(WINAPI*)(HANDLE);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto setContext = reinterpret_cast<SetProcessDpiAwarenessContextFunc>(
            GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
        if (setContext && setContext(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))) {
            return;
        }
    }

    using SetProcessDPIAwareFunc = BOOL(WINAPI*)();
    if (hUser32) {
        auto setAware = reinterpret_cast<SetProcessDPIAwareFunc>(GetProcAddress(hUser32, "SetProcessDPIAware"));
        if (setAware) setAware();
    }
}

void DPIManager::AdjustWindowRectForDPI(LPRECT rect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle) {
    using AdjustWindowRectExForDpiFunc = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto adjustForDpi = reinterpret_cast<AdjustWindowRectExForDpiFunc>(
            GetProcAddress(hUser32, "AdjustWindowRectExForDpi"));
        if (adjustForDpi && adjustForDpi(rect, dwStyle, bMenu, dwExStyle, s_dpi)) {
            return;
        }
    }
    AdjustWindowRectEx(rect, dwStyle, bMenu, dwExStyle);
}
