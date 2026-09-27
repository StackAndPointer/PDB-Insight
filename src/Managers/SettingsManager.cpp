#include "SettingsManager.h"
#include "FontManager.h"
#include "DPIManager.h"
#include <cwctype>

namespace {
constexpr int IDC_CHECK_FLATTEN = 1010;
constexpr int IDC_CHECK_REMOVE_VOID = 1011;
constexpr int IDC_CHECK_IDA = 1012;
constexpr int IDC_CHECK_ENUMS = 1013;
constexpr int IDC_CHECK_ANONYMOUS = 1016;
constexpr int IDC_CHECK_MIRROR = 1014;
constexpr int IDC_EDIT_MIRROR = 1015;
constexpr int IDC_BUTTON_OK = 2001;
constexpr int IDC_BUTTON_CANCEL = 2002;
constexpr WCHAR kSettingsClass[] = L"PDBViewerSettingsWindow";
}

SettingsManager& SettingsManager::GetInstance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() : m_window(nullptr), m_parent(nullptr) {}

void SettingsManager::CreateControls(HWND window) {
    LanguageManager& language = LanguageManager::GetInstance();
    m_groupEnhanced = CreateWindowExW(0, L"BUTTON", language.GetString(L"group_enhanced_options").c_str(),
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 0, 0, window, nullptr, hInst, nullptr);
    m_checkFlattenNamespaces = CreateWindowExW(0, L"BUTTON", language.GetString(L"opt_flatten_namespaces").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_FLATTEN)), hInst, nullptr);
    m_checkRemoveVoidParams = CreateWindowExW(0, L"BUTTON", language.GetString(L"opt_remove_void_params").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_REMOVE_VOID)), hInst, nullptr);
    m_checkIncludeEnums = CreateWindowExW(0, L"BUTTON", language.GetString(L"opt_include_enums_in_enumsh").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_ENUMS)), hInst, nullptr);
    m_checkExpandAnonymous = CreateWindowExW(0, L"BUTTON",
        language.GetString(L"opt_expand_anonymous_aggregates").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_ANONYMOUS)), hInst, nullptr);

    m_groupIda = CreateWindowExW(0, L"BUTTON", language.GetString(L"group_ida_compatibility").c_str(),
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 0, 0, window, nullptr, hInst, nullptr);
    m_checkIdaCompatible = CreateWindowExW(0, L"BUTTON", language.GetString(L"opt_ida_compatible").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_IDA)), hInst, nullptr);

    m_groupDownload = CreateWindowExW(0, L"BUTTON", language.GetString(L"group_download_options").c_str(),
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 0, 0, window, nullptr, hInst, nullptr);
    m_checkUseMirror = CreateWindowExW(0, L"BUTTON", language.GetString(L"opt_use_mirror_source").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CHECK_MIRROR)), hInst, nullptr);
    m_labelMirrorUrl = CreateWindowExW(0, L"STATIC", language.GetString(L"opt_custom_mirror_url").c_str(),
        WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, 0, 0, 0, 0, window, nullptr, hInst, nullptr);
    m_editMirrorUrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_EDIT_MIRROR)), hInst, nullptr);

    m_buttonOk = CreateWindowExW(0, L"BUTTON", language.GetString(L"button_ok").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BUTTON_OK)), hInst, nullptr);
    m_buttonCancel = CreateWindowExW(0, L"BUTTON", language.GetString(L"button_cancel").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, window,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_BUTTON_CANCEL)), hInst, nullptr);

    HFONT font = FontManager::GetDefaultFont();
    HWND controls[] = {
        m_groupEnhanced, m_checkFlattenNamespaces, m_checkRemoveVoidParams, m_checkIncludeEnums,
        m_checkExpandAnonymous,
        m_groupIda, m_checkIdaCompatible, m_groupDownload, m_checkUseMirror,
        m_labelMirrorUrl, m_editMirrorUrl, m_buttonOk, m_buttonCancel
    };
    for (HWND control : controls) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(m_editMirrorUrl, EM_SETCUEBANNER, TRUE,
        reinterpret_cast<LPARAM>(language.GetString(L"opt_mirror_url_hint").c_str()));
}

void SettingsManager::InitControls() {
    const ExportSettings& settings = ConfigManager::GetInstance().GetExportSettings();
    SendMessageW(m_checkFlattenNamespaces, BM_SETCHECK, settings.flattenNamespaces ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_checkRemoveVoidParams, BM_SETCHECK, settings.removeVoidParams ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_checkIncludeEnums, BM_SETCHECK, settings.includeEnumsInEnumsH ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_checkExpandAnonymous, BM_SETCHECK,
        settings.expandAnonymousAggregates ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_checkIdaCompatible, BM_SETCHECK, settings.idaCompatible ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_checkUseMirror, BM_SETCHECK,
        ConfigManager::GetInstance().GetUseMirrorSource() ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextW(m_editMirrorUrl, ConfigManager::GetInstance().GetMirrorSourceUrl().c_str());
    UpdateMirrorControls();
}

void SettingsManager::UpdateMirrorControls() {
    bool enabled = SendMessageW(m_checkUseMirror, BM_GETCHECK, 0, 0) == BST_CHECKED;
    EnableWindow(m_labelMirrorUrl, enabled);
    EnableWindow(m_editMirrorUrl, enabled);
}

bool SettingsManager::SaveSettings(HWND owner) {
    bool useMirror = SendMessageW(m_checkUseMirror, BM_GETCHECK, 0, 0) == BST_CHECKED;
    int length = GetWindowTextLengthW(m_editMirrorUrl) + 1;
    std::wstring mirrorUrl(static_cast<size_t>(length), L'\0');
    GetWindowTextW(m_editMirrorUrl, &mirrorUrl[0], length);
    mirrorUrl.resize(length - 1);
    while (!mirrorUrl.empty() && iswspace(mirrorUrl.back())) mirrorUrl.pop_back();

    if (useMirror && (mirrorUrl.rfind(L"https://", 0) != 0 && mirrorUrl.rfind(L"http://", 0) != 0)) {
        MessageBoxW(owner, LANG_STR(L"msg_invalid_mirror_url").c_str(), LANG_STR(L"msg_error").c_str(),
            MB_OK | MB_ICONERROR);
        SetFocus(m_editMirrorUrl);
        return false;
    }

    ExportSettings settings = ConfigManager::GetInstance().GetExportSettings();
    settings.flattenNamespaces = SendMessageW(m_checkFlattenNamespaces, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.removeVoidParams = SendMessageW(m_checkRemoveVoidParams, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.includeEnumsInEnumsH = SendMessageW(m_checkIncludeEnums, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.expandAnonymousAggregates =
        SendMessageW(m_checkExpandAnonymous, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.idaCompatible = SendMessageW(m_checkIdaCompatible, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ConfigManager::GetInstance().SetExportSettings(settings);
    ConfigManager::GetInstance().SetUseMirrorSource(useMirror);
    ConfigManager::GetInstance().SetMirrorSourceUrl(mirrorUrl);
    ConfigManager::GetInstance().Save();
    return true;
}

void SettingsManager::OnSize(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    int margin = DPIManager::ScaleX(12);
    int gap = DPIManager::ScaleY(8);
    int groupWidth = max(DPIManager::ScaleX(320), client.right - margin * 2);
    int y = margin;
    int groupEnhancedHeight = DPIManager::ScaleY(104);
    SetWindowPos(m_groupEnhanced, nullptr, margin, y, groupWidth, groupEnhancedHeight, SWP_NOZORDER);
    int checkX = margin + DPIManager::ScaleX(18);
    int checkWidth = groupWidth - DPIManager::ScaleX(32);
    int checkHeight = DPIManager::ScaleY(20);
    SetWindowPos(m_checkFlattenNamespaces, nullptr, checkX, y + DPIManager::ScaleY(22), checkWidth, checkHeight, SWP_NOZORDER);
    SetWindowPos(m_checkRemoveVoidParams, nullptr, checkX, y + DPIManager::ScaleY(44), checkWidth, checkHeight, SWP_NOZORDER);
    SetWindowPos(m_checkIncludeEnums, nullptr, checkX, y + DPIManager::ScaleY(62), checkWidth, checkHeight, SWP_NOZORDER);
    SetWindowPos(m_checkExpandAnonymous, nullptr, checkX, y + DPIManager::ScaleY(84), checkWidth, checkHeight, SWP_NOZORDER);

    y += groupEnhancedHeight + gap;
    int groupIdaHeight = DPIManager::ScaleY(48);
    SetWindowPos(m_groupIda, nullptr, margin, y, groupWidth, groupIdaHeight, SWP_NOZORDER);
    SetWindowPos(m_checkIdaCompatible, nullptr, checkX, y + DPIManager::ScaleY(22), checkWidth, checkHeight, SWP_NOZORDER);

    y += groupIdaHeight + gap;
    int groupDownloadHeight = DPIManager::ScaleY(102);
    SetWindowPos(m_groupDownload, nullptr, margin, y, groupWidth, groupDownloadHeight, SWP_NOZORDER);
    SetWindowPos(m_checkUseMirror, nullptr, checkX, y + DPIManager::ScaleY(22), checkWidth, checkHeight, SWP_NOZORDER);
    SetWindowPos(m_labelMirrorUrl, nullptr, checkX, y + DPIManager::ScaleY(48), checkWidth, checkHeight, SWP_NOZORDER);
    SetWindowPos(m_editMirrorUrl, nullptr, checkX, y + DPIManager::ScaleY(70), checkWidth, DPIManager::ScaleY(24), SWP_NOZORDER);

    int buttonWidth = DPIManager::ScaleX(88);
    int buttonHeight = DPIManager::ScaleY(28);
    int buttonY = max(y + groupDownloadHeight + gap, client.bottom - margin - buttonHeight);
    SetWindowPos(m_buttonCancel, nullptr, client.right - margin - buttonWidth, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(m_buttonOk, nullptr, client.right - margin - buttonWidth * 2 - gap, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
}

LRESULT CALLBACK SettingsManager::SettingsWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsManager* self = reinterpret_cast<SettingsManager*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<SettingsManager*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_window = window;
    }
    if (!self) return DefWindowProcW(window, message, wParam, lParam);

    switch (message) {
    case WM_CREATE:
        self->CreateControls(window);
        self->InitControls();
        self->OnSize(window);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CHECK_MIRROR && HIWORD(wParam) == BN_CLICKED) {
            self->UpdateMirrorControls();
            return 0;
        }
        if (LOWORD(wParam) == IDC_BUTTON_OK || LOWORD(wParam) == IDOK) {
            if (self->SaveSettings(window)) self->CloseSettingsWindow();
            return 0;
        }
        if (LOWORD(wParam) == IDC_BUTTON_CANCEL || LOWORD(wParam) == IDCANCEL) {
            self->CloseSettingsWindow();
            return 0;
        }
        break;
    case WM_SIZE:
        self->OnSize(window);
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = DPIManager::ScaleX(520);
        info->ptMinTrackSize.y = DPIManager::ScaleY(454);
        return 0;
    }
    case WM_CLOSE:
        self->CloseSettingsWindow();
        return 0;
    case WM_DESTROY:
        self->m_window = nullptr;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void SettingsManager::ShowSettingsWindow(HWND parent) {
    if (m_window) {
        SetForegroundWindow(m_window);
        return;
    }
    m_parent = parent;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = SettingsWndProc;
    windowClass.hInstance = hInst;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kSettingsClass;
    windowClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    windowClass.hIconSm = windowClass.hIcon;
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;

    int width = DPIManager::ScaleX(560);
    int height = DPIManager::ScaleY(474);
    RECT parentRect{};
    GetWindowRect(parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    m_window = CreateWindowExW(WS_EX_DLGMODALFRAME, kSettingsClass,
        LANG_STR(L"dlg_settings_title").c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        x, y, width, height, parent, nullptr, hInst, this);
    if (!m_window) return;

    EnableWindow(parent, FALSE);
    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);

    MSG message{};
    while (m_window && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(m_window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

void SettingsManager::CloseSettingsWindow() {
    HWND window = m_window;
    if (!window) return;
    EnableWindow(m_parent, TRUE);
    SetForegroundWindow(m_parent);
    m_window = nullptr;
    DestroyWindow(window);
}
