#include "TreeViewManager.h"
#include "ControlsManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "LanguageManager.h"
#include "LoadingManager.h"
#include "TreeNodeHelper.h"

void PopulateTreeView() {
    if (!hTreeView) return;
    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(hTreeView);

    if (!g_pdbLoaded) {
        SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
        return;
    }

    HTREEITEM root = AddTreeItem(TVI_ROOT,
        g_moduleInfo.name.empty() ? LANG_STR(L"tree_module") : g_moduleInfo.name, 0);

    HTREEITEM functions = AddTreeItem(root, LANG_STR(L"tree_functions"), 1);
    for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i) {
        const auto& function = g_moduleInfo.functions[i];
        AddTreeItem(functions, function.displaySignature.empty()
            ? PDBParser::GenerateFunctionSignature(function) : function.displaySignature,
            TreeNodeParamHelper::MakeParam(TreeNodeType::FunctionItem, i));
    }

    HTREEITEM classes = AddTreeItem(root, LANG_STR(L"tree_classes"), 2);
    for (size_t i = 0; i < g_moduleInfo.classes.size(); ++i) {
        AddTreeItem(classes, g_moduleInfo.classes[i].name,
            TreeNodeParamHelper::MakeParam(TreeNodeType::ClassItem, i));
    }

    HTREEITEM structs = AddTreeItem(root, LANG_STR(L"tree_structs"), 3);
    for (size_t i = 0; i < g_moduleInfo.structs.size(); ++i) {
        AddTreeItem(structs, g_moduleInfo.structs[i].name,
            TreeNodeParamHelper::MakeParam(TreeNodeType::StructItem, i));
    }

    HTREEITEM unions = AddTreeItem(root, LANG_STR(L"tree_unions"), 4);
    for (size_t i = 0; i < g_moduleInfo.unions.size(); ++i) {
        AddTreeItem(unions, g_moduleInfo.unions[i].name,
            TreeNodeParamHelper::MakeParam(TreeNodeType::UnionItem, i));
    }

    HTREEITEM enums = AddTreeItem(root, LANG_STR(L"tree_enums"), 5);
    for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i) {
        AddTreeItem(enums, g_moduleInfo.enums[i].name,
            TreeNodeParamHelper::MakeParam(TreeNodeType::EnumItem, i));
    }

    HTREEITEM variables = AddTreeItem(root, LANG_STR(L"tree_global_variables"), 6);
    for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i) {
        AddTreeItem(variables, g_moduleInfo.globalVariables[i].name,
            TreeNodeParamHelper::MakeParam(TreeNodeType::GlobalVarItem, i));
    }

    TreeView_Expand(hTreeView, root, TVE_EXPAND);
    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
}

HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, LPARAM lParam) {
    TVINSERTSTRUCT insert{};
    insert.hParent = hParent;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.item.pszText = const_cast<WCHAR*>(text.c_str());
    insert.item.lParam = lParam;
    return TreeView_InsertItem(hTreeView, &insert);
}

void OpenPDBFile(HWND hWnd) {
    WCHAR file[MAX_PATH] = L"";
    std::wstring filter = LANG_STR(L"filter_pdb");
    filter.push_back(L'\0');
    filter += L"*.pdb";
    filter.push_back(L'\0');
    filter += LANG_STR(L"filter_all");
    filter.push_back(L'\0');
    filter += L"*.*";
    filter.push_back(L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hWnd;
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = file;
    dialog.nMaxFile = _countof(file);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"pdb";

    if (GetOpenFileNameW(&dialog)) LoadingManager::StartPdbFile(hWnd, file);
}

void ClosePDBFile(HWND hWnd) {
    UNREFERENCED_PARAMETER(hWnd);
    if (LoadingManager::IsActive()) LoadingManager::CancelCurrentTask();
    if (!g_pdbLoaded) return;

    g_pdbLoaded = false;
    g_moduleInfo = ModuleInfo();
    TreeView_DeleteAllItems(hTreeView);
    ListView_DeleteAllItems(hListView);
    for (int i = Header_GetItemCount(ListView_GetHeader(hListView)) - 1; i >= 0; --i) {
        ListView_DeleteColumn(hListView, i);
    }
    SetWindowTextW(hRichEdit, L"");
    ClearSearchBox();
    UpdateStatusBar(LANG_STR(L"status_ready"));
}

void OpenDllFile(HWND hWnd) {
    WCHAR file[MAX_PATH] = L"";
    std::wstring filter = LANG_STR(L"filter_dll");
    filter.push_back(L'\0');
    filter += L"*.dll;*.exe";
    filter.push_back(L'\0');
    filter += LANG_STR(L"filter_all");
    filter.push_back(L'\0');
    filter += L"*.*";
    filter.push_back(L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hWnd;
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = file;
    dialog.nMaxFile = _countof(file);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"dll";

    if (GetOpenFileNameW(&dialog)) LoadingManager::StartDllFile(hWnd, file);
}

void RefreshCurrentSelection() {
    TabCtrl_SetCurSel(hTabCtrl, g_currentTabIndex);
    UpdateTabViews();
}
