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
#include "CacheManager.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PDBVIEWER));
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    wcex.hIcon = hIcon;

    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!LoadStringW(hInstance, IDC_PDBVIEWER, szWindowClass, MAX_LOADSTRING)) {
        wcscpy_s(szWindowClass, L"PDBINSIGHT");
    }

    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCEW(IDC_PDBVIEWER));
    if (hMenu) {
        wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PDBVIEWER);
    }
    else {
        wcex.lpszMenuName = NULL;
    }

    wcex.lpszClassName = szWindowClass;

    HICON hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!hIconSm) {
        hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }
    wcex.hIconSm = hIconSm;

    ATOM atom = RegisterClassExW(&wcex);
    if (!atom) {
        DWORD dwError = GetLastError();
        std::wstringstream ss;
        ss << L"RegisterClassEx failed! Error code: " << dwError;
        ss << L", ClassName: " << szWindowClass;
        MessageBoxW(NULL, ss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);

        wcscpy_s(szWindowClass, L"PDBINSIGHT");
        wcex.lpszClassName = szWindowClass;
        atom = RegisterClassExW(&wcex);
        if (!atom) {
            dwError = GetLastError();
            std::wstringstream ss2;
            ss2 << L"RegisterClassEx failed again! Error code: " << dwError;
            MessageBoxW(NULL, ss2.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }
    return atom;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPWSTR lpCmdLine)
{
    hInst = hInstance;

    if (!LoadLibraryW(L"riched20.dll")) {
        MessageBoxW(NULL, L"Failed to load riched20.dll!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ConfigManager::GetInstance().Load();
    g_numberMode = ConfigManager::GetInstance().GetNumberMode();
    g_expandBaseClasses = ConfigManager::GetInstance().GetExpandBaseClasses();
    g_currentLanguage = ConfigManager::GetInstance().GetLanguage();
    
    DPIManager::Initialize();
    FontManager::Initialize();

    if (!LanguageManager::GetInstance().LoadLanguageByCode(g_currentLanguage)) {
        LanguageManager::GetInstance().DetectAndLoadSystemLanguage();
    }

    if (!LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING)) {
        wcscpy_s(szTitle, L"PDB Insight");
    }

    if (!LoadStringW(hInstance, IDC_PDBVIEWER, szWindowClass, MAX_LOADSTRING)) {
        wcscpy_s(szWindowClass, L"PDBINSIGHT");
    }

    int windowWidth = DPIManager::ScaleX(1200);
    int windowHeight = DPIManager::ScaleY(800);
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        DWORD dwError = GetLastError();
        std::wstringstream ss;
        ss << L"CreateWindow failed! Error code: " << dwError;
        MessageBoxW(NULL, ss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    CreateControls(hWnd);

    RebuildMenu(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    DragDropManager::Initialize(hWnd);
    
    AutoAssociatePDBFiles();

    if (lpCmdLine && wcslen(lpCmdLine) > 0)
    {
        std::wstring filePath = lpCmdLine;
        if (filePath[0] == L'"')
        {
            filePath = filePath.substr(1, filePath.find(L'"', 1) - 1);
        }
        if (!filePath.empty())
        {
            size_t dotPos = filePath.find_last_of(L'.');
            if (dotPos != std::wstring::npos)
            {
                std::wstring extension = filePath.substr(dotPos);
                for (size_t i = 0; i < extension.length(); i++)
                {
                    extension[i] = towlower(extension[i]);
                }
                
                if (extension == L".pdb")
                {
                    if (g_parser.LoadPDB(filePath))
                    {
                        g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                            std::wstring status = L"解析中: " + text + L" (" + std::to_wstring(progress) + L"%)";
                            UpdateStatusBar(status);
                            });
                        g_moduleInfo = g_parser.ParseModule();
                        g_moduleInfo.pdbFileName = filePath;
                        g_pdbLoaded = true;

                        PopulateTreeView();

                        std::wstringstream ss;
                        ss << L"已加载: " << filePath
                            << L" | 函数: " << g_moduleInfo.functions.size()
                            << L" | 类: " << g_moduleInfo.classes.size()
                            << L" | 结构体: " << g_moduleInfo.structs.size()
                            << L" | 联合体: " << g_moduleInfo.unions.size();
                        UpdateStatusBar(ss.str());
                    }
                }
                else if (extension == L".pdbbc")
                {
                    ModuleInfo moduleInfo;
                    std::wstring errorMsg;
                    if (CacheManager::GetInstance().LoadCache(moduleInfo, filePath, errorMsg))
                    {
                        g_moduleInfo = moduleInfo;
                        g_pdbLoaded = true;

                        PopulateTreeView();

                        std::wstringstream ss;
                        ss << L"已从缓存加载: " << filePath
                            << L" | 函数: " << g_moduleInfo.functions.size()
                            << L" | 类: " << g_moduleInfo.classes.size()
                            << L" | 结构体: " << g_moduleInfo.structs.size()
                            << L" | 联合体: " << g_moduleInfo.unions.size();
                        UpdateStatusBar(ss.str());
                    }
                }
            }
        }
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        DragDropManager::HandleDropFiles(wParam, hWnd);
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_MENU_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_MENU_OPEN:
            OpenPDBFile(hWnd);
            break;
        case ID_MENU_OPEN_CACHE:
            OpenCacheFile(hWnd);
            break;
        case ID_MENU_EXPORT_CSV:
            ExportToCSV(hWnd);
            break;
        case ID_MENU_EXPORT_XML:
            ExportToXML(hWnd);
            break;
        case ID_MENU_EXPORT_FUNCTIONS_CSV:
            ExportFunctionsToCSV(hWnd);
            break;
        case ID_MENU_EXPORT_CLASSES_CSV:
            ExportClassesToCSV(hWnd);
            break;
        case ID_MENU_EXPORT_HEADER:
            ExportHeader(hWnd);
            break;
        case ID_MENU_EXPORT_ALL_HEADERS:
            ExportAllHeaders(hWnd);
            break;
        case ID_MENU_EXPORT_ENUMS_H:
            ExportEnumsHeader(hWnd);
            break;
        case ID_MENU_CLOSE:
            ClosePDBFile(hWnd);
            break;
        case ID_BUTTON_SEARCH:
        {
            int len = GetWindowTextLengthW(hEditSearch) + 1;
            std::wstring searchText(len, L'\0');
            GetWindowTextW(hEditSearch, &searchText[0], len);
            searchText.resize(len - 1);
            SearchItems(searchText);
        }
        break;
        case ID_BUTTON_SEARCH_HISTORY:
        {
            RECT rect;
            GetWindowRect(hButtonSearchHistory, &rect);
            ShowSearchHistoryMenu(hWnd, rect.left, rect.bottom);
        }
        break;
        case ID_CLEAR_SEARCH_HISTORY:
            ConfigManager::GetInstance().ClearSearchHistory();
            break;
        case ID_NUMBER_HEX:
            g_numberMode = NUMBER_HEX;
            ConfigManager::GetInstance().SetNumberMode(NUMBER_HEX);
            ConfigManager::GetInstance().Save();
            {
                HMENU hMenuBar = GetMenu(hWnd);
                HMENU hViewMenu = GetSubMenu(hMenuBar, 1);
                CheckMenuRadioItem(hViewMenu, ID_NUMBER_HEX, ID_NUMBER_BOTH, ID_NUMBER_HEX, MF_BYCOMMAND);

                RefreshCurrentSelection();
            }
            break;
        case ID_NUMBER_DEC:
            g_numberMode = NUMBER_DEC;
            ConfigManager::GetInstance().SetNumberMode(NUMBER_DEC);
            ConfigManager::GetInstance().Save();
            {
                HMENU hMenuBar = GetMenu(hWnd);
                HMENU hViewMenu = GetSubMenu(hMenuBar, 1);
                CheckMenuRadioItem(hViewMenu, ID_NUMBER_HEX, ID_NUMBER_BOTH, ID_NUMBER_DEC, MF_BYCOMMAND);

                RefreshCurrentSelection();
            }
            break;
        case ID_NUMBER_BOTH:
            g_numberMode = NUMBER_BOTH;
            ConfigManager::GetInstance().SetNumberMode(NUMBER_BOTH);
            ConfigManager::GetInstance().Save();
            {
                HMENU hMenuBar = GetMenu(hWnd);
                HMENU hViewMenu = GetSubMenu(hMenuBar, 1);
                CheckMenuRadioItem(hViewMenu, ID_NUMBER_HEX, ID_NUMBER_BOTH, ID_NUMBER_BOTH, MF_BYCOMMAND);

                RefreshCurrentSelection();
            }
            break;
        case ID_EXPAND_BASE_CLASSES:
            g_expandBaseClasses = !g_expandBaseClasses;
            ConfigManager::GetInstance().SetExpandBaseClasses(g_expandBaseClasses);
            ConfigManager::GetInstance().Save();
            {
                HMENU hMenuBar = GetMenu(hWnd);
                HMENU hViewMenu = GetSubMenu(hMenuBar, 1);
                CheckMenuItem(hViewMenu, ID_EXPAND_BASE_CLASSES, MF_BYCOMMAND | (g_expandBaseClasses ? MF_CHECKED : MF_UNCHECKED));

                RefreshCurrentSelection();
            }
            break;
        case ID_LANGUAGE_ZH_CN:
            g_currentLanguage = L"zh-CN";
            ConfigManager::GetInstance().SetLanguage(L"zh-CN");
            ConfigManager::GetInstance().Save();
            if (LanguageManager::GetInstance().LoadLanguageByCode(L"zh-CN")) {
                RefreshLanguage(hWnd);
            }
            break;
        case ID_LANGUAGE_EN_US:
            g_currentLanguage = L"en-US";
            ConfigManager::GetInstance().SetLanguage(L"en-US");
            ConfigManager::GetInstance().Save();
            if (LanguageManager::GetInstance().LoadLanguageByCode(L"en-US")) {
                RefreshLanguage(hWnd);
            }
            break;
        case ID_ASSOCIATE_PDB:
            AssociatePDBFiles(hWnd);
            break;
        case ID_UNASSOCIATE_PDB:
            UnassociatePDBFiles(hWnd);
            break;
        case ID_MENU_SETTINGS:
            SettingsManager::GetInstance().ShowSettingsWindow(hWnd);
            break;
        default:
            if (wmId >= ID_SEARCH_HISTORY_FIRST && wmId <= ID_SEARCH_HISTORY_LAST) {
                const auto& history = ConfigManager::GetInstance().GetSearchHistory();
                int index = wmId - ID_SEARCH_HISTORY_FIRST;
                if (index >= 0 && index < (int)history.size()) {
                    SetWindowTextW(hEditSearch, history[index].c_str());
                    SearchItems(history[index], false);
                }
            }
            else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    }
    break;
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == ID_TREEVIEW && pnmh->code == TVN_SELCHANGED)
        {
            LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
            int tabIndex = TabCtrl_GetCurSel(hTabCtrl);
            if (tabIndex == 0) {
                PopulateListView(pnmtv->itemNew.hItem);
            }
            else {
                ShowHeaderView(pnmtv->itemNew.hItem);
            }
        }
        else if (pnmh->idFrom == ID_TABCTRL && pnmh->code == TCN_SELCHANGE)
        {
            g_currentTabIndex = TabCtrl_GetCurSel(hTabCtrl);
            UpdateTabViews();
        }
        else if (pnmh->idFrom == ID_TREEVIEW && pnmh->code == NM_RCLICK)
        {
            LPNMMOUSE pnmouse = (LPNMMOUSE)lParam;
            POINT pt = pnmouse->pt;
            ClientToScreen(hTreeView, &pt);
            ShowOffsetModeMenu(hWnd, pt.x, pt.y);
        }
        else if (pnmh->idFrom == ID_RICHEDIT && pnmh->code == NM_RCLICK)
        {
            LPNMMOUSE pnmouse = (LPNMMOUSE)lParam;
            POINT pt = pnmouse->pt;
            ClientToScreen(hRichEdit, &pt);
            ShowRichEditContextMenu(hWnd, pt.x, pt.y);
        }
        else if (pnmh->idFrom == ID_LISTVIEW && pnmh->code == NM_RCLICK)
        {
            LPNMMOUSE pnmouse = (LPNMMOUSE)lParam;
            
            LVHITTESTINFO hti;
            ZeroMemory(&hti, sizeof(hti));
            hti.pt = pnmouse->pt;
            ListView_SubItemHitTest(hListView, &hti);
            g_lastClickedSubItem = (hti.iSubItem >= 0) ? hti.iSubItem : 0;
            
            POINT pt = pnmouse->pt;
            ClientToScreen(hListView, &pt);
            ShowListViewContextMenu(hWnd, pt.x, pt.y);
        }
    }
    break;
    case WM_CONTEXTMENU:
    {
        HWND hCtrl = (HWND)wParam;
        if (hCtrl == hTreeView)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ShowOffsetModeMenu(hWnd, pt.x, pt.y);
        }
        else if (hCtrl == hRichEdit)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ShowRichEditContextMenu(hWnd, pt.x, pt.y);
        }
    }
    break;
    case WM_SIZE:
    {
        if (hTreeView && hTabCtrl && hStatusBar)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);

            int statusBarHeight = DPIManager::ScaleY(24);
            int searchBarHeight = DPIManager::ScaleY(30);

            SetWindowPos(hEditSearch, nullptr, DPIManager::ScaleX(10), DPIManager::ScaleY(5), DPIManager::ScaleX(300), DPIManager::ScaleY(24), SWP_NOZORDER);
            SetWindowPos(hButtonSearch, nullptr, DPIManager::ScaleX(320), DPIManager::ScaleY(5), DPIManager::ScaleX(80), DPIManager::ScaleY(24), SWP_NOZORDER);
            SetWindowPos(hButtonSearchHistory, nullptr, DPIManager::ScaleX(410), DPIManager::ScaleY(5), DPIManager::ScaleX(30), DPIManager::ScaleY(24), SWP_NOZORDER);

            SetWindowPos(hStatusBar, nullptr, 0, rect.bottom - statusBarHeight, rect.right, statusBarHeight, SWP_NOZORDER);

            if (g_splitterPos == 0) {
                g_splitterPos = rect.right / 3;
            }
            UpdateSplitterPosition(hWnd);

            UpdateTabViews();
        }
    }
    break;
    case WM_CREATE:
    {
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        DragDropManager::Cleanup();
        FontManager::Cleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}