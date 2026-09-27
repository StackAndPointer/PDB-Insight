#include "SearchManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "LanguageManager.h"
#include "TreeNodeHelper.h"


void SearchItems(const std::wstring& text, bool addToHistory) {
    if (text.empty()) {
        PopulateTreeView();
        return;
    }
    if (!g_pdbLoaded) return;

    if (addToHistory) ConfigManager::GetInstance().AddSearchHistory(text);

    std::wstring lowerText = text;
    for (auto& c : lowerText) c = towlower(c);

    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(hTreeView);

    HTREEITEM root = AddTreeItem(TVI_ROOT,
        LANG_STR(L"search_results") + L" - " + text, 0);
    HTREEITEM functions = nullptr;
    HTREEITEM classes = nullptr;
    HTREEITEM structs = nullptr;
    HTREEITEM unions = nullptr;
    HTREEITEM enums = nullptr;
    HTREEITEM variables = nullptr;
    size_t resultCount = 0;

    for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i) {
        std::wstring name = g_moduleInfo.functions[i].displaySignature.empty()
            ? PDBParser::GenerateFunctionSignature(g_moduleInfo.functions[i])
            : g_moduleInfo.functions[i].displaySignature;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!functions) functions = AddTreeItem(root, LANG_STR(L"tree_functions"), 1);
        AddTreeItem(functions, name, TreeNodeParamHelper::MakeParam(TreeNodeType::FunctionItem, i));
        ++resultCount;
    }

    for (size_t i = 0; i < g_moduleInfo.classes.size(); ++i) {
        std::wstring name = g_moduleInfo.classes[i].name;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!classes) classes = AddTreeItem(root, LANG_STR(L"tree_classes"), 2);
        AddTreeItem(classes, name, TreeNodeParamHelper::MakeParam(TreeNodeType::ClassItem, i));
        ++resultCount;
    }

    for (size_t i = 0; i < g_moduleInfo.structs.size(); ++i) {
        std::wstring name = g_moduleInfo.structs[i].name;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!structs) structs = AddTreeItem(root, LANG_STR(L"tree_structs"), 3);
        AddTreeItem(structs, name, TreeNodeParamHelper::MakeParam(TreeNodeType::StructItem, i));
        ++resultCount;
    }

    for (size_t i = 0; i < g_moduleInfo.unions.size(); ++i) {
        std::wstring name = g_moduleInfo.unions[i].name;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!unions) unions = AddTreeItem(root, LANG_STR(L"tree_unions"), 4);
        AddTreeItem(unions, name, TreeNodeParamHelper::MakeParam(TreeNodeType::UnionItem, i));
        ++resultCount;
    }

    for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i) {
        std::wstring name = g_moduleInfo.enums[i].name;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!enums) enums = AddTreeItem(root, LANG_STR(L"tree_enums"), 5);
        AddTreeItem(enums, name, TreeNodeParamHelper::MakeParam(TreeNodeType::EnumItem, i));
        ++resultCount;
    }

    for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i) {
        std::wstring name = g_moduleInfo.globalVariables[i].name;
        std::wstring lowerName = name;
        for (auto& c : lowerName) c = towlower(c);
        if (lowerName.find(lowerText) == std::wstring::npos) continue;
        if (!variables) variables = AddTreeItem(root, LANG_STR(L"tree_global_variables"), 6);
        AddTreeItem(variables, name, TreeNodeParamHelper::MakeParam(TreeNodeType::GlobalVarItem, i));
        ++resultCount;
    }

    if (resultCount == 0) {
        AddTreeItem(root, LANG_STR(L"search_no_results"), static_cast<LPARAM>(-1));
    }

    TreeView_Expand(hTreeView, root, TVE_EXPAND);
    for (HTREEITEM category : { functions, classes, structs, unions, enums, variables }) {
        if (category) TreeView_Expand(hTreeView, category, TVE_EXPAND);
    }
    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
    UpdateStatusBar(LANG_STR(L"search_results") + L": " + std::to_wstring(resultCount));
}

LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const bool trackedMessage = message == WM_KEYDOWN || message == WM_APP_UPDATE_SEARCH_UI ||
        message == WM_CHAR || message == WM_PASTE || message == WM_CUT ||
        message == WM_CLEAR || message == WM_SETTEXT || message == WM_UNDO;
    if (!trackedMessage) {
        return CallWindowProcW(g_pOldEditProc, hWnd, message, wParam, lParam);
    }
    if (message == WM_KEYDOWN && wParam == VK_RETURN) {
        int length = GetWindowTextLengthW(hWnd) + 1;
        std::wstring searchText(static_cast<size_t>(length), L'\0');
        GetWindowTextW(hWnd, &searchText[0], length);
        searchText.resize(length - 1);
        SearchItems(searchText);
        return 0;
    }
    if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
        ClearSearchBox();
        return 0;
    }
    if (message == WM_APP_UPDATE_SEARCH_UI) {
        UpdateSearchClearButton();
        return 0;
    }
    LRESULT result = CallWindowProcW(g_pOldEditProc, hWnd, message, wParam, lParam);
    if (message == WM_CHAR || message == WM_PASTE || message == WM_CUT || message == WM_CLEAR ||
        message == WM_SETTEXT || message == WM_UNDO) {
        if (message == WM_SETTEXT) {
            PostMessageW(hWnd, WM_APP_UPDATE_SEARCH_UI, 0, 0);
        } else {
            UpdateSearchClearButton();
        }
    }
    return result;
}
