#include "SettingsManager.h"
#include "PDBViewerGlobals.h"
#include "LanguageManager.h"
#include "DPIManager.h"
#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

static const WCHAR g_szSettingsWindowClass[] = L"PDBViewerSettingsWindow";
static const int IDC_CHECK_FLATTEN = 1010;
static const int IDC_CHECK_REMOVEVOID = 1011;
static const int IDC_CHECK_IDACOMPAT = 1012;
static const int IDC_CHECK_INCLUDEENUMS = 1013;
static const int IDC_CHECK_USEMIRROR = 1014;
static const int IDC_EDIT_MIRRORURL = 1015;
static const int IDC_BUTTON_OK = 2001;
static const int IDC_BUTTON_CANCEL = 2002;

SettingsManager& SettingsManager::GetInstance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() : m_hWnd(nullptr), m_hParent(nullptr) {
}

void SettingsManager::CreateControls(HWND hWnd) {
    LanguageManager& langMgr = LanguageManager::GetInstance();
    const auto& settings = ConfigManager::GetInstance().GetExportSettings();
    
    int xMargin = DPIManager::ScaleX(10);
    int yMargin = DPIManager::ScaleY(10);
    int groupWidth = DPIManager::ScaleX(350);
    int groupHeight = DPIManager::ScaleY(70);
    
    int y = yMargin;
    
    m_hGroupEnhancedOptions = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"group_enhanced_options", L"增强选项").c_str(),
        BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
        xMargin, y, groupWidth, groupHeight, hWnd, nullptr, hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hCheckFlattenNamespaces = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"opt_flatten_namespaces", L"扁平化命名空间").c_str(),
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_FLATTEN)), hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hCheckRemoveVoidParams = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"opt_remove_void_params", L"消除void参数").c_str(),
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_REMOVEVOID)), hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hCheckIncludeEnumsInEnumsH = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"opt_include_enums_in_enumsh", L"Enums.h包含枚举").c_str(),
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_INCLUDEENUMS)), hInst, nullptr);
    
    y += groupHeight - DPIManager::ScaleY(54) + yMargin;
    
    groupHeight = DPIManager::ScaleY(45);
    m_hGroupIDACompatibility = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"group_ida_compatibility", L"IDA兼容性").c_str(),
        BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
        xMargin, y, groupWidth, groupHeight, hWnd, nullptr, hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hCheckIDACompatible = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"opt_ida_compatible", L"IDA兼容格式").c_str(),
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_IDACOMPAT)), hInst, nullptr);
    
    y += groupHeight - DPIManager::ScaleY(18) + yMargin;
    
    groupHeight = DPIManager::ScaleY(70);
    m_hGroupDownloadOptions = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"group_download_options", L"下载选项").c_str(),
        BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
        xMargin, y, groupWidth, groupHeight, hWnd, nullptr, hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hCheckUseMirrorSource = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"opt_use_mirror_source", L"使用镜像源加速下载").c_str(),
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_USEMIRROR)), hInst, nullptr);
    
    y += DPIManager::ScaleY(20);
    m_hLabelMirrorUrl = CreateWindowExW(0, L"STATIC", langMgr.GetString(L"opt_custom_mirror_url", L"自定义镜像源地址").c_str(),
        WS_CHILD | WS_VISIBLE,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(16),
        hWnd, nullptr, hInst, nullptr);
    
    y += DPIManager::ScaleY(18);
    m_hEditMirrorUrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        xMargin + DPIManager::ScaleX(15), y, groupWidth - DPIManager::ScaleX(30), DPIManager::ScaleY(20),
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_EDIT_MIRRORURL)), hInst, nullptr);
    
    y += groupHeight - DPIManager::ScaleY(56) + yMargin + DPIManager::ScaleY(10);
    
    int buttonWidth = DPIManager::ScaleX(70);
    int buttonHeight = DPIManager::ScaleY(25);
    m_hButtonOK = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"button_ok", L"确定").c_str(),
        BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(80), y, buttonWidth, buttonHeight,
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BUTTON_OK)), hInst, nullptr);
    
    m_hButtonCancel = CreateWindowExW(0, L"BUTTON", langMgr.GetString(L"button_cancel", L"取消").c_str(),
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        xMargin + DPIManager::ScaleX(200), y, buttonWidth, buttonHeight,
        hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BUTTON_CANCEL)), hInst, nullptr);
}

void SettingsManager::InitControls(HWND hWnd) {
    const auto& settings = ConfigManager::GetInstance().GetExportSettings();
    
    SendMessageW(m_hCheckFlattenNamespaces, BM_SETCHECK, settings.flattenNamespaces ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckRemoveVoidParams, BM_SETCHECK, settings.removeVoidParams ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckIncludeEnumsInEnumsH, BM_SETCHECK, settings.includeEnumsInEnumsH ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckIDACompatible, BM_SETCHECK, settings.idaCompatible ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckUseMirrorSource, BM_SETCHECK, ConfigManager::GetInstance().GetUseMirrorSource() ? BST_CHECKED : BST_UNCHECKED, 0);
    
    std::wstring mirrorUrl = ConfigManager::GetInstance().GetMirrorSourceUrl();
    if (mirrorUrl.empty()) {
        mirrorUrl = L"https://symbols.yandex.ru/";
    }
    SetWindowTextW(m_hEditMirrorUrl, mirrorUrl.c_str());
}

void SettingsManager::SaveSettings(HWND hWnd) {
    auto settings = ConfigManager::GetInstance().GetExportSettings();
    
    settings.flattenNamespaces = (SendMessageW(m_hCheckFlattenNamespaces, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings.removeVoidParams = (SendMessageW(m_hCheckRemoveVoidParams, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings.includeEnumsInEnumsH = (SendMessageW(m_hCheckIncludeEnumsInEnumsH, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings.idaCompatible = (SendMessageW(m_hCheckIDACompatible, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    ConfigManager::GetInstance().SetExportSettings(settings);
    ConfigManager::GetInstance().SetUseMirrorSource(SendMessageW(m_hCheckUseMirrorSource, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    int len = GetWindowTextLengthW(m_hEditMirrorUrl) + 1;
    std::wstring mirrorUrl(len, L'\0');
    GetWindowTextW(m_hEditMirrorUrl, &mirrorUrl[0], len);
    mirrorUrl.resize(len - 1);
    ConfigManager::GetInstance().SetMirrorSourceUrl(mirrorUrl);
    
    ConfigManager::GetInstance().Save();
}

LRESULT CALLBACK SettingsManager::SettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsManager* pThis = nullptr;
    
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<SettingsManager*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
    } else {
        pThis = reinterpret_cast<SettingsManager*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }
    
    if (!pThis) {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    
    switch (message) {
    case WM_CREATE:
        pThis->CreateControls(hWnd);
        pThis->InitControls(hWnd);
        break;
    
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BUTTON_OK:
            pThis->SaveSettings(hWnd);
            pThis->CloseSettingsWindow();
            break;
        case IDC_BUTTON_CANCEL:
            pThis->CloseSettingsWindow();
            break;
        }
        break;
    }
    
    case WM_SIZE:
        pThis->OnSize(hWnd);
        break;
    
    case WM_CLOSE:
        pThis->CloseSettingsWindow();
        break;
    
    case WM_DESTROY:
        pThis->m_hWnd = nullptr;
        break;
    
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

void SettingsManager::OnSize(HWND hWnd) {
}

void SettingsManager::ShowSettingsWindow(HWND hParent) {
    if (m_hWnd) {
        SetForegroundWindow(m_hWnd);
        return;
    }
    
    m_hParent = hParent;
    
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = SettingsWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = g_szSettingsWindowClass;
    
    HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    if (!hIcon) {
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    wcex.hIcon = hIcon;
    wcex.hIconSm = hIcon;
    
    ATOM atom = RegisterClassExW(&wcex);
    if (!atom) {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxW(hParent, L"Failed to register window class!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
    }
    
    LanguageManager& langMgr = LanguageManager::GetInstance();
    int windowWidth = DPIManager::ScaleX(380);
    int windowHeight = DPIManager::ScaleY(450);
    
    RECT parentRect;
    GetWindowRect(hParent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - windowWidth) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - windowHeight) / 2;
    
    m_hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        g_szSettingsWindowClass,
        langMgr.GetString(L"dlg_settings_title", L"导出头文件功能设置").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, windowWidth, windowHeight,
        hParent, nullptr, hInst, this);
    
    if (!m_hWnd) {
        DWORD dwError = GetLastError();
        std::wstringstream ss;
        ss << L"Failed to create settings window! Error: " << dwError;
        MessageBoxW(hParent, ss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    EnableWindow(hParent, FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
    
    MSG msg;
    while (m_hWnd && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(m_hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void SettingsManager::CloseSettingsWindow() {
    if (m_hWnd) {
        EnableWindow(m_hParent, TRUE);
        SetForegroundWindow(m_hParent);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}
