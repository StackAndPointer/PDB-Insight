#include "MainWindow.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "ExportManager.h"
#include "MenuManager.h"
#include "SearchManager.h"
#include "FontManager.h"
#include "DragDropManager.h"
#include "DPIManager.h"
#include "SettingsManager.h"
#include "CommandLineManager.h"
#include "LoadingManager.h"
#include "ThemeManager.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "wininet.lib")

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PDBVIEWER));
    if (!windowClass.hIcon) windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = MAKEINTRESOURCEW(IDC_PDBVIEWER);
    windowClass.lpszClassName = szWindowClass;
    windowClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!windowClass.hIconSm) windowClass.hIconSm = windowClass.hIcon;
    return RegisterClassExW(&windowClass);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPWSTR lpCmdLine) {
    hInst = hInstance;
    CommandLineManager::GetInstance().ParseCommandLine(lpCmdLine);
    const CommandLineOptions& options = CommandLineManager::GetInstance().GetOptions();
    if (options.showHelp) {
        CommandLineManager::GetInstance().ShowHelp();
        return FALSE;
    }

    if (!LoadLibraryW(L"riched20.dll")) {
        MessageBoxW(nullptr, L"Failed to load riched20.dll!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ConfigManager::GetInstance().Load();
    g_numberMode = ConfigManager::GetInstance().GetNumberMode();
    g_expandBaseClasses = ConfigManager::GetInstance().GetExpandBaseClasses();
    g_themeMode = ConfigManager::GetInstance().GetThemeMode();
    ThemeManager::GetInstance().SetMode(
        g_themeMode == 1 ? ThemeMode::Dark : ThemeMode::Light);
    g_currentLanguage = ConfigManager::GetInstance().GetLanguage();
    DPIManager::Initialize();
    FontManager::Initialize();
    if (!LanguageManager::GetInstance().LoadLanguageByCode(g_currentLanguage)) {
        LanguageManager::GetInstance().DetectAndLoadSystemLanguage();
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
    LoadStringW(hInstance, IDC_PDBVIEWER, szWindowClass, _countof(szWindowClass));

    RECT windowRect{0, 0, DPIManager::ScaleX(1200), DPIManager::ScaleY(800)};
    DPIManager::AdjustWindowRectForDPI(&windowRect, WS_OVERLAPPEDWINDOW, TRUE, 0);
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    CreateControls(hWnd);
    RebuildMenu(hWnd);
    ThemeManager::GetInstance().Apply(hWnd);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetFocus(hEditSearch);
    DragDropManager::Initialize(hWnd);

    if (CommandLineManager::GetInstance().ShouldAutoDownloadPdb()) {
        LoadingManager::StartDllFile(hWnd, options.dllPath);
    } else if (CommandLineManager::GetInstance().HasValidPdbPath()) {
        LoadingManager::StartPdbFile(hWnd, options.pdbPath);
    }
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DROPFILES:
        DragDropManager::HandleDropFiles(wParam, hWnd);
        return 0;

    case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_LBUTTONDOWN && HIWORD(wParam) == ID_BUTTON_CLEAR_SEARCH) {
            ClearSearchBox();
            return 0;
        }
        break;

    case WM_COMMAND: {
        const int command = LOWORD(wParam);
        switch (command) {
        case ID_MENU_EXIT:
            DestroyWindow(hWnd);
            return 0;
        case ID_MENU_OPEN:
            OpenPDBFile(hWnd);
            return 0;
        case ID_MENU_OPEN_DLL:
            OpenDllFile(hWnd);
            return 0;
        case ID_MENU_CLOSE:
            ClosePDBFile(hWnd);
            return 0;
        case ID_MENU_EXPORT_CSV: ExportToCSV(hWnd); return 0;
        case ID_MENU_EXPORT_XML: ExportToXML(hWnd); return 0;
        case ID_MENU_EXPORT_FUNCTIONS_CSV: ExportFunctionsToCSV(hWnd); return 0;
        case ID_MENU_EXPORT_CLASSES_CSV: ExportClassesToCSV(hWnd); return 0;
        case ID_MENU_EXPORT_HEADER: ExportHeader(hWnd); return 0;
        case ID_MENU_EXPORT_ALL_HEADERS: ExportAllHeaders(hWnd); return 0;
        case ID_MENU_EXPORT_ENUMS_H: ExportEnumsHeader(hWnd); return 0;
        case ID_EDIT_SEARCH:
            // EN_UPDATE/EN_CHANGE are notifications from the edit control.
            // Only the accelerator command (HIWORD == 0) should move focus
            // and select the query; doing this for EN_UPDATE makes each
            // keystroke replace the entire text.
            if (HIWORD(wParam) == EN_UPDATE || HIWORD(wParam) == EN_CHANGE) {
                UpdateSearchClearButton();
                return 0;
            }
            if (HIWORD(wParam) == 0) {
                SetFocus(hEditSearch);
                SendMessageW(hEditSearch, EM_SETSEL, 0, -1);
            }
            return 0;
        case ID_EDIT_COPY:
            CopyFocusedContent();
            return 0;
        case ID_BUTTON_COPY_HEADER:
            CopyHeaderText();
            return 0;
        case ID_BUTTON_CLEAR_SEARCH:
            ClearSearchBox();
            return 0;
        case ID_BUTTON_CANCEL_TASK:
            LoadingManager::CancelCurrentTask();
            return 0;
        case ID_BUTTON_SEARCH: {
            int length = GetWindowTextLengthW(hEditSearch) + 1;
            std::wstring text(static_cast<size_t>(length), L'\0');
            GetWindowTextW(hEditSearch, &text[0], length);
            text.resize(length - 1);
            SearchItems(text);
            return 0;
        }
        case ID_BUTTON_SEARCH_HISTORY: {
            RECT rect{};
            GetWindowRect(hButtonSearchHistory, &rect);
            ShowSearchHistoryMenu(hWnd, rect.left, rect.bottom);
            return 0;
        }
        case ID_CLEAR_SEARCH_HISTORY:
            ConfigManager::GetInstance().ClearSearchHistory();
            return 0;
        case ID_NUMBER_HEX:
        case ID_NUMBER_DEC:
        case ID_NUMBER_BOTH:
            g_numberMode = command == ID_NUMBER_HEX ? NUMBER_HEX : command == ID_NUMBER_DEC ? NUMBER_DEC : NUMBER_BOTH;
            ConfigManager::GetInstance().SetNumberMode(g_numberMode);
            ConfigManager::GetInstance().Save();
            CheckMenuRadioItem(GetSubMenu(GetMenu(hWnd), 1), ID_NUMBER_HEX, ID_NUMBER_BOTH, command, MF_BYCOMMAND);
            RefreshCurrentSelection();
            return 0;
        case ID_THEME_LIGHT:
        case ID_THEME_DARK:
            g_themeMode = command == ID_THEME_DARK ? 1 : 0;
            ThemeManager::GetInstance().SetMode(
                g_themeMode == 1 ? ThemeMode::Dark : ThemeMode::Light);
            ConfigManager::GetInstance().SetThemeMode(g_themeMode);
            ConfigManager::GetInstance().Save();
            RebuildMenu(hWnd);
            ThemeManager::GetInstance().Apply(hWnd);
            return 0;
        case ID_EXPAND_BASE_CLASSES:
            g_expandBaseClasses = !g_expandBaseClasses;
            ConfigManager::GetInstance().SetExpandBaseClasses(g_expandBaseClasses);
            ConfigManager::GetInstance().Save();
            CheckMenuItem(GetSubMenu(GetMenu(hWnd), 1), ID_EXPAND_BASE_CLASSES,
                MF_BYCOMMAND | (g_expandBaseClasses ? MF_CHECKED : MF_UNCHECKED));
            RefreshCurrentSelection();
            return 0;
        case ID_LANGUAGE_ZH_CN:
        case ID_LANGUAGE_EN_US:
            g_currentLanguage = command == ID_LANGUAGE_ZH_CN ? L"zh-CN" : L"en-US";
            ConfigManager::GetInstance().SetLanguage(g_currentLanguage);
            ConfigManager::GetInstance().Save();
            if (LanguageManager::GetInstance().LoadLanguageByCode(g_currentLanguage)) RefreshLanguage(hWnd);
            return 0;
        case ID_ASSOCIATE_PDB: AssociatePDBFiles(hWnd); return 0;
        case ID_UNASSOCIATE_PDB: UnassociatePDBFiles(hWnd); return 0;
        case ID_MENU_SETTINGS: SettingsManager::GetInstance().ShowSettingsWindow(hWnd); return 0;
        default:
            if (command >= ID_SEARCH_HISTORY_FIRST && command <= ID_SEARCH_HISTORY_LAST) {
                const auto& history = ConfigManager::GetInstance().GetSearchHistory();
                int index = command - ID_SEARCH_HISTORY_FIRST;
                if (index >= 0 && index < static_cast<int>(history.size())) {
                    SetWindowTextW(hEditSearch, history[index].c_str());
                    SearchItems(history[index], false);
                }
                return 0;
            }
            break;
        }
        break;
    }

    case WM_NOTIFY: {
        const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
        if (header->idFrom == ID_TREEVIEW && header->code == TVN_SELCHANGED) {
            const NMTREEVIEW* treeView = reinterpret_cast<const NMTREEVIEW*>(lParam);
            if (TabCtrl_GetCurSel(hTabCtrl) == 0) PopulateListView(treeView->itemNew.hItem);
            else ShowHeaderView(treeView->itemNew.hItem);
            return 0;
        }
        if (header->idFrom == ID_TABCTRL && header->code == TCN_SELCHANGE) {
            g_currentTabIndex = TabCtrl_GetCurSel(hTabCtrl);
            UpdateTabViews();
            return 0;
        }
        break;
    }

    case WM_CONTEXTMENU: {
        HWND control = reinterpret_cast<HWND>(wParam);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (control == hRichEdit) ShowRichEditContextMenu(hWnd, x, y);
        else if (control == hTreeView) ShowOffsetModeMenu(hWnd, x, y);
        return 0;
    }

    case WM_APP_LOADING_PROGRESS:
    case WM_APP_LOADING_COMPLETE:
        LoadingManager::HandleMessage(hWnd, message, wParam, lParam);
        return 0;

    case WM_SIZE:
        LayoutMainWindow(hWnd);
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        if (ThemeManager::GetInstance().GetMode() == ThemeMode::Dark) {
            const ThemePalette& palette = ThemeManager::GetInstance().GetPalette();
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, palette.text);
            SetBkColor(dc, palette.surface);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        }
        break;
    }

    case WM_DPICHANGED: {
        UINT dpi = HIWORD(wParam);
        DPIManager::SetDPI(static_cast<int>(dpi));
        FontManager::UpdateDPI(static_cast<int>(dpi));
        ApplyApplicationFonts();
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        LayoutMainWindow(hWnd);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = DPIManager::ScaleX(720);
        info->ptMinTrackSize.y = DPIManager::ScaleY(520);
        return 0;
    }

    case WM_SETFOCUS:
        if (hEditSearch && GetFocus() == hWnd) SetFocus(hEditSearch);
        return 0;

    case WM_DESTROY:
        LoadingManager::Shutdown();
        DragDropManager::Cleanup();
        FontManager::Cleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
    }
    return FALSE;
}
