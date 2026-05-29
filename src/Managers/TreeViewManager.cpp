#include "TreeViewManager.h"
#include "ControlsManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "LanguageManager.h"
#include "PDBDownloader.h"


void PopulateTreeView()
{
    TreeView_DeleteAllItems(hTreeView);

    if (!g_pdbLoaded) return;

    size_t totalItems = g_moduleInfo.functions.size() + g_moduleInfo.classes.size() + 
                        g_moduleInfo.structs.size() + g_moduleInfo.unions.size() + 
                        g_moduleInfo.enums.size() + g_moduleInfo.globalVariables.size();
    size_t processedItems = 0;

    UpdateStatusBar(LanguageManager::GetInstance().GetString(L"tree_module", L"模块") + L" - " + 
                    LanguageManager::GetInstance().GetString(L"status_loading", L"正在加载..."));

    HTREEITEM hRoot = AddTreeItem(TVI_ROOT, g_moduleInfo.name.empty() ? LanguageManager::GetInstance().GetString(L"tree_module", L"模块") : g_moduleInfo.name, 0);

    HTREEITEM hFunctions = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_functions", L"函数"), 1);
    for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i)
    {
        std::wstring displayName = PDBParser::GenerateFunctionSignature(g_moduleInfo.functions[i]);
        AddTreeItem(hFunctions, displayName, 10000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_functions", L"函数") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    HTREEITEM hClasses = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_classes", L"类"), 2);
    for (size_t i = 0; i < g_moduleInfo.classes.size(); ++i)
    {
        AddTreeItem(hClasses, g_moduleInfo.classes[i].name, 20000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_classes", L"类") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    HTREEITEM hStructs = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体"), 3);
    for (size_t i = 0; i < g_moduleInfo.structs.size(); ++i)
    {
        AddTreeItem(hStructs, g_moduleInfo.structs[i].name, 30000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    HTREEITEM hUnions = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体"), 4);
    for (size_t i = 0; i < g_moduleInfo.unions.size(); ++i)
    {
        AddTreeItem(hUnions, g_moduleInfo.unions[i].name, 40000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    HTREEITEM hEnums = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举"), 5);
    for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i)
    {
        AddTreeItem(hEnums, g_moduleInfo.enums[i].name, 50000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    HTREEITEM hGlobalVars = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_global_variables", L"全局变量"), 6);
    for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i)
    {
        AddTreeItem(hGlobalVars, g_moduleInfo.globalVariables[i].name, 60000 + (DWORD)i);
        processedItems++;
        
        if (totalItems > 100 && processedItems % 50 == 0) {
            int percent = (int)(processedItems * 100 / totalItems);
            std::wstring status = LanguageManager::GetInstance().GetString(L"tree_global_variables", L"全局变量") + L": " + 
                                  std::to_wstring(percent) + L"% (" + std::to_wstring(processedItems) + L"/" + std::to_wstring(totalItems) + L")";
            UpdateStatusBar(status);
            
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    TreeView_Expand(hTreeView, hRoot, TVE_EXPAND);
}

HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, LPARAM lParam)
{
    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvis.item.pszText = (LPWSTR)text.c_str();
    tvis.item.lParam = lParam;
    return TreeView_InsertItem(hTreeView, &tvis);
}

void OpenPDBFile(HWND hWnd) {
    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"";
    WCHAR filterBuffer[] = L"PDB Files (*.pdb)\0*.pdb\0All Files (*.*)\0*.*\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = filterBuffer;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"pdb";

    if (GetOpenFileNameW(&ofn)) {
        UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_loading", L"正在加载 PDB 文件..."));
        
        if (g_parser.LoadPDB(ofn.lpstrFile)) {
            g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                std::wstring status = LanguageManager::GetInstance().GetString(L"status_parsing", L"解析中: ") + text + L" (" + std::to_wstring(progress) + L"%)";
                UpdateStatusBar(status);
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            });
            g_moduleInfo = g_parser.ParseModule();
            g_moduleInfo.pdbFileName = ofn.lpstrFile;
            g_pdbLoaded = true;

            PopulateTreeView();

            std::wstringstream ss;
            ss << LanguageManager::GetInstance().GetString(L"status_loaded", L"已加载: ") << ofn.lpstrFile
               << L" | " << LanguageManager::GetInstance().GetString(L"tree_functions", L"函数: ") << g_moduleInfo.functions.size()
               << L" | " << LanguageManager::GetInstance().GetString(L"tree_classes", L"类: ") << g_moduleInfo.classes.size()
               << L" | " << LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体: ") << g_moduleInfo.structs.size()
               << L" | " << LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体: ") << g_moduleInfo.unions.size()
               << L" | " << LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举: ") << g_moduleInfo.enums.size();
            UpdateStatusBar(ss.str());
        }
        else {
            std::wstring errorMsg = LanguageManager::GetInstance().GetString(L"msg_pdb_load_fail", L"无法加载 PDB 文件: ") + g_parser.GetLastError();
            MessageBoxW(hWnd, errorMsg.c_str(), 
                       LanguageManager::GetInstance().GetString(L"msg_error", L"错误").c_str(), MB_OK | MB_ICONERROR);
            UpdateStatusBar(L"加载失败");
        }
    }
}



void ClosePDBFile(HWND hWnd)
{
    if (!g_pdbLoaded) {
        return;
    }

    g_pdbLoaded = false;
    g_moduleInfo = ModuleInfo();

    TreeView_DeleteAllItems(hTreeView);
    ListView_DeleteAllItems(hListView);

    for (int i = Header_GetItemCount(ListView_GetHeader(hListView)) - 1; i >= 0; --i) {
        ListView_DeleteColumn(hListView, i);
    }

    SetWindowTextW(hRichEdit, L"");

    UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_ready", L"就绪 - 请打开一个 PDB 文件"));
}

void OpenDllFile(HWND hWnd) {
    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"";
    WCHAR filterBuffer[] = L"DLL/EXE Files (*.dll;*.exe)\0*.dll;*.exe\0All Files (*.*)\0*.*\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = filterBuffer;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_downloading_pdb", L"正在从微软服务器下载 PDB 文件..."));
        
        PDBDownloadResult result = PDBDownloader::GetInstance().DownloadPDBForDll(ofn.lpstrFile, L"", [](int progress, const std::wstring& text) {
            UpdateStatusBar(text);
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        });
        
        if (result.success) {
            std::wstring pdbPath = result.pdbPath;
            
            if (g_parser.LoadPDB(pdbPath)) {
                g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                    std::wstring status = LanguageManager::GetInstance().GetString(L"status_parsing", L"解析中: ") + text + L" (" + std::to_wstring(progress) + L"%)";
                    UpdateStatusBar(status);
                    MSG msg;
                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                });
                g_moduleInfo = g_parser.ParseModule();
                g_moduleInfo.pdbFileName = pdbPath;
                g_pdbLoaded = true;

                PopulateTreeView();

                std::wstringstream ss;
                ss << LanguageManager::GetInstance().GetString(L"status_loaded", L"已加载: ") << pdbPath
                   << L" | " << LanguageManager::GetInstance().GetString(L"tree_functions", L"函数: ") << g_moduleInfo.functions.size()
                   << L" | " << LanguageManager::GetInstance().GetString(L"tree_classes", L"类: ") << g_moduleInfo.classes.size()
                   << L" | " << LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体: ") << g_moduleInfo.structs.size()
                   << L" | " << LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体: ") << g_moduleInfo.unions.size()
                   << L" | " << LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举: ") << g_moduleInfo.enums.size();
                UpdateStatusBar(ss.str());
            }
            else {
                std::wstring errorMsg = LanguageManager::GetInstance().GetString(L"msg_pdb_load_fail", L"无法加载 PDB 文件: ") + g_parser.GetLastError();
                MessageBoxW(hWnd, errorMsg.c_str(), 
                           LanguageManager::GetInstance().GetString(L"msg_error", L"错误").c_str(), MB_OK | MB_ICONERROR);
                UpdateStatusBar(L"加载失败");
            }
        }
        else {
            std::wstring errorMsg = LanguageManager::GetInstance().GetString(L"msg_pdb_download_fail", L"PDB 文件下载失败: ") + result.errorMessage;
            MessageBoxW(hWnd, errorMsg.c_str(), 
                       LanguageManager::GetInstance().GetString(L"msg_error", L"错误").c_str(), MB_OK | MB_ICONERROR);
            UpdateStatusBar(result.errorMessage);
        }
    }
}

void RefreshCurrentSelection()
{
    HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
    if (hSelected && g_pdbLoaded) {
        PopulateListView(hSelected);
        ShowHeaderView(hSelected);
        TabCtrl_SetCurSel(hTabCtrl, g_currentTabIndex);
        UpdateTabViews();
    }
}
