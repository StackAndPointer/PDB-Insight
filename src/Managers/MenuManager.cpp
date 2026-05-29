#include "MenuManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"

void ShowOffsetModeMenu(HWND hWnd, int x, int y)
{
    HMENU hMenu = CreatePopupMenu();
    
    UINT flagsHex = MF_STRING;
    UINT flagsDec = MF_STRING;
    UINT flagsBoth = MF_STRING;
    
    if (g_numberMode == NUMBER_HEX) flagsHex |= MF_CHECKED;
    else if (g_numberMode == NUMBER_DEC) flagsDec |= MF_CHECKED;
    else if (g_numberMode == NUMBER_BOTH) flagsBoth |= MF_CHECKED;
    
    AppendMenuW(hMenu, flagsHex, ID_NUMBER_HEX, L"数值 - 十六进制(Hex)");
    AppendMenuW(hMenu, flagsDec, ID_NUMBER_DEC, L"数值 - 十进制(Dec)");
    AppendMenuW(hMenu, flagsBoth, ID_NUMBER_BOTH, L"数值 - 两者都显示");
    
    POINT pt = { x, y };
    if (x == -1 && y == -1) {
        GetCursorPos(&pt);
    }
    
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

void ShowSearchHistoryMenu(HWND hWnd, int x, int y)
{
    HMENU hMenu = CreatePopupMenu();
    const auto& history = ConfigManager::GetInstance().GetSearchHistory();
    
    if (history.empty()) {
        AppendMenuW(hMenu, MF_GRAYED, 0, LanguageManager::GetInstance().GetString(L"menu_no_history", L"无搜索历史").c_str());
    } else {
        for (size_t i = 0; i < history.size(); ++i) {
            AppendMenuW(hMenu, MF_STRING, ID_SEARCH_HISTORY_FIRST + (UINT)i, history[i].c_str());
        }
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, ID_CLEAR_SEARCH_HISTORY, LanguageManager::GetInstance().GetString(L"menu_clear_history", L"清除历史").c_str());
    }
    
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

void RebuildMenu(HWND hWnd)
{
    HMENU hMenuBar = GetMenu(hWnd);
    if (hMenuBar) {
        while (GetMenuItemCount(hMenuBar) > 0) {
            RemoveMenu(hMenuBar, 0, MF_BYPOSITION);
        }
    } else {
        hMenuBar = CreateMenu();
        SetMenu(hWnd, hMenuBar);
    }

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_OPEN, LanguageManager::GetInstance().GetString(L"menu_open", L"打开 PDB 文件(&O)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_OPEN_DLL, LanguageManager::GetInstance().GetString(L"menu_open_dll", L"打开 DLL/EXE 文件(&D)...").c_str());
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_CSV, LanguageManager::GetInstance().GetString(L"menu_export_csv", L"导出全部为 CSV(&C)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_FUNCTIONS_CSV, LanguageManager::GetInstance().GetString(L"menu_export_functions_csv", L"导出函数为 CSV(&F)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_CLASSES_CSV, LanguageManager::GetInstance().GetString(L"menu_export_classes_csv", L"导出类为 CSV(&L)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_XML, LanguageManager::GetInstance().GetString(L"menu_export_xml", L"导出为 XML(&X)...").c_str());
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_HEADER, LanguageManager::GetInstance().GetString(L"menu_export_header", L"导出当前类头文件(&H)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_ALL_HEADERS, LanguageManager::GetInstance().GetString(L"menu_export_all_headers", L"导出所有头文件(&A)...").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT_ENUMS_H, LanguageManager::GetInstance().GetString(L"menu_export_enums_h", L"导出所有类到 Enums.h(&E)...").c_str());
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_CLOSE, LanguageManager::GetInstance().GetString(L"menu_close", L"关闭文件(&C)").c_str());
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXIT, LanguageManager::GetInstance().GetString(L"menu_exit", L"退出(&E)").c_str());
    InsertMenuW(hMenuBar, 0, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hFileMenu, LanguageManager::GetInstance().GetString(L"menu_file", L"文件(&F)").c_str());
    
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING | (g_expandBaseClasses ? MF_CHECKED : 0), ID_EXPAND_BASE_CLASSES, LanguageManager::GetInstance().GetString(L"menu_expand_base_classes", L"展开基类成员(&E)").c_str());
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hViewMenu, MF_STRING | (g_numberMode == NUMBER_HEX ? MF_CHECKED : 0), ID_NUMBER_HEX, LanguageManager::GetInstance().GetString(L"menu_number_hex", L"数值 - 十六进制(Hex)").c_str());
    AppendMenuW(hViewMenu, MF_STRING | (g_numberMode == NUMBER_DEC ? MF_CHECKED : 0), ID_NUMBER_DEC, LanguageManager::GetInstance().GetString(L"menu_number_dec", L"数值 - 十进制(Dec)").c_str());
    AppendMenuW(hViewMenu, MF_STRING | (g_numberMode == NUMBER_BOTH ? MF_CHECKED : 0), ID_NUMBER_BOTH, LanguageManager::GetInstance().GetString(L"menu_number_both", L"数值 - 两者都显示").c_str());
    InsertMenuW(hMenuBar, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hViewMenu, LanguageManager::GetInstance().GetString(L"menu_view", L"显示(&V)").c_str());
    
    HMENU hLangMenu = CreatePopupMenu();
    AppendMenuW(hLangMenu, MF_STRING | (g_currentLanguage == L"zh-CN" ? MF_CHECKED : 0), ID_LANGUAGE_ZH_CN, LanguageManager::GetInstance().GetString(L"menu_lang_zh_cn", L"中文(简体)(&C)").c_str());
    AppendMenuW(hLangMenu, MF_STRING | (g_currentLanguage == L"en-US" ? MF_CHECKED : 0), ID_LANGUAGE_EN_US, LanguageManager::GetInstance().GetString(L"menu_lang_en_us", L"English(&E)").c_str());
    InsertMenuW(hMenuBar, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hLangMenu, LanguageManager::GetInstance().GetString(L"menu_language", L"语言(&L)").c_str());
    
    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenuW(hToolsMenu, MF_STRING, ID_MENU_SETTINGS, LanguageManager::GetInstance().GetString(L"menu_settings", L"导出头文件设置(&S)...").c_str());
    AppendMenuW(hToolsMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hToolsMenu, MF_STRING, ID_ASSOCIATE_PDB, LanguageManager::GetInstance().GetString(L"menu_associate_pdb", L"关联 PDB 文件(&A)").c_str());
    AppendMenuW(hToolsMenu, MF_STRING, ID_UNASSOCIATE_PDB, LanguageManager::GetInstance().GetString(L"menu_unassociate_pdb", L"取消关联 PDB 文件(&U)").c_str());
    InsertMenuW(hMenuBar, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hToolsMenu, LanguageManager::GetInstance().GetString(L"menu_tools", L"工具(&T)").c_str());
    
    DrawMenuBar(hWnd);
}

void RefreshLanguage(HWND hWnd)
{
    RebuildMenu(hWnd);
    
    SetWindowTextW(hButtonSearch, LanguageManager::GetInstance().GetString(L"button_search", L"搜索").c_str());
    SetWindowTextW(hButtonSearchHistory, LanguageManager::GetInstance().GetString(L"button_search_history", L"▼").c_str());
    
    wcscpy_s(g_szTabText1, LanguageManager::GetInstance().GetString(L"tab_details", L"详细信息").c_str());
    wcscpy_s(g_szTabText2, LanguageManager::GetInstance().GetString(L"tab_header", L"头文件视图").c_str());
    
    TCITEM tci;
    tci.mask = TCIF_TEXT;
    tci.pszText = g_szTabText1;
    TabCtrl_SetItem(hTabCtrl, 0, &tci);
    tci.pszText = g_szTabText2;
    TabCtrl_SetItem(hTabCtrl, 1, &tci);
    
    UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_ready", L"就绪 - 请打开一个 PDB 文件"));
    
    // 更新提示文本
    if (hInfoText) {
        SetWindowTextW(hInfoText, LanguageManager::GetInstance().GetString(L"info_export_hint", L"如果符号信息查看不全请导出后查看").c_str());
    }
    
    if (g_pdbLoaded) {
        PopulateTreeView();
    }
    
    int currentTab = TabCtrl_GetCurSel(hTabCtrl);
    HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
    if (hSelected && g_pdbLoaded) {
        PopulateListView(hSelected);
        ShowHeaderView(hSelected);
        TabCtrl_SetCurSel(hTabCtrl, currentTab);
        UpdateTabViews();
    }
}

void AutoAssociatePDBFiles()
{
    WCHAR modulePath[MAX_PATH];
    GetModuleFileNameW(hInst, modulePath, MAX_PATH);

    HKEY hKey;
    LONG result;

    // 关联 .pdb 文件
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.pdb", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result == ERROR_SUCCESS) {
        std::wstring value = L"PDB Insight.PDBFile";
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.length() + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }

    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\PDB Insight.PDBFile", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result == ERROR_SUCCESS) {
        std::wstring value = L"PDB Debug File";
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.length() + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }

    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\PDB Insight.PDBFile\\shell\\open\\command", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result == ERROR_SUCCESS) {
        std::wstring command = std::wstring(L"\"") + modulePath + L"\" \"%1\"";
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)command.c_str(), (DWORD)(command.length() + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }



    // 为 PDB 文件添加图标关联
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\PDB Insight.PDBFile\\DefaultIcon", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result == ERROR_SUCCESS) {
        std::wstring iconPath = std::wstring(L"\"") + modulePath + L"\", 0";
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)iconPath.c_str(), (DWORD)(iconPath.length() + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }

    // 通知系统文件关联已更改
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

void AssociatePDBFiles(HWND hWnd)
{
    AutoAssociatePDBFiles();
    MessageBoxW(hWnd, L"PDB 文件关联成功！", L"提示", MB_OK | MB_ICONINFORMATION);
}

void UnassociatePDBFiles(HWND hWnd)
{
    HKEY hKey;
    LONG result;

    // 取消 .pdb 文件关联
    result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.pdb", 0, KEY_READ | KEY_WRITE, &hKey);
    if (result == ERROR_SUCCESS) {
        WCHAR currentValue[MAX_PATH];
        DWORD valueSize = sizeof(currentValue);
        result = RegQueryValueExW(hKey, nullptr, nullptr, nullptr, (LPBYTE)currentValue, &valueSize);
        if (result == ERROR_SUCCESS && wcscmp(currentValue, L"PDB Insight.PDBFile") == 0) {
            RegDeleteValueW(hKey, nullptr);
        }
        RegCloseKey(hKey);
    }

    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\PDB Insight.PDBFile");

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    MessageBoxW(hWnd, L"已取消 PDB 文件关联！", L"提示", MB_OK | MB_ICONINFORMATION);
}
