#pragma once

#include <windows.h>
#include <string>

class SettingsManager {
public:
    static SettingsManager& GetInstance();

    void ShowSettingsWindow(HWND hParent);
    void CloseSettingsWindow();

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    static LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void CreateControls(HWND hWnd);
    void InitControls(HWND hWnd);
    void SaveSettings(HWND hWnd);
    void OnSize(HWND hWnd);

    HWND m_hWnd;
    HWND m_hParent;
    
    HWND m_hGroupEnhancedOptions;
    HWND m_hCheckFlattenNamespaces;
    HWND m_hCheckRemoveVoidParams;
    HWND m_hCheckIncludeEnumsInEnumsH;
    
    HWND m_hGroupIDACompatibility;
    HWND m_hCheckIDACompatible;
    
    HWND m_hButtonOK;
    HWND m_hButtonCancel;
};
