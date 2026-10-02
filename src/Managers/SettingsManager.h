#pragma once

#include "PDBViewerGlobals.h"

class SettingsManager {
public:
    static SettingsManager& GetInstance();
    void ShowSettingsWindow(HWND parent);
    void CloseSettingsWindow();

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    static LRESULT CALLBACK SettingsWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    void CreateControls(HWND window);
    void InitControls();
    bool SaveSettings(HWND owner);
    void OnSize(HWND window);
    void UpdateMirrorControls();
    void PopulateFontControls();
    void RefreshFontControls();

    HWND m_window;
    HWND m_parent;
    HWND m_groupEnhanced;
    HWND m_checkFlattenNamespaces;
    HWND m_checkRemoveVoidParams;
    HWND m_checkIncludeEnums;
    HWND m_checkExpandAnonymous;
    HWND m_groupIda;
    HWND m_checkIdaCompatible;
    HWND m_groupFonts;
    HWND m_labelUiFont;
    HWND m_comboUiFont;
    HWND m_labelCodeFont;
    HWND m_comboCodeFont;
    HWND m_groupDownload;
    HWND m_checkUseMirror;
    HWND m_labelMirrorUrl;
    HWND m_editMirrorUrl;
    HWND m_buttonOk;
    HWND m_buttonCancel;
};
