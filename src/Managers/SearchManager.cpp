#include "SearchManager.h"
#include "TreeViewManager.h"
#include "LanguageManager.h"
#include "TreeNodeHelper.h"

void SearchItems(const std::wstring& text, bool addToHistory)
{
    if (text.empty())
    {
        PopulateTreeView();
        return;
    }

    if (addToHistory)
    {
        ConfigManager::GetInstance().AddSearchHistory(text);
    }

    if (!g_pdbLoaded) return;

    std::wstring lowerText = text;
    for (auto& c : lowerText) c = towlower(c);

    TreeView_DeleteAllItems(hTreeView);

    HTREEITEM hRoot = AddTreeItem(TVI_ROOT, g_moduleInfo.name.empty() ? LanguageManager::GetInstance().GetString(L"tree_module", L"模块") : g_moduleInfo.name, 0);

    HTREEITEM hFunctions = nullptr;
    for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i)
    {
        std::wstring funcName = PDBParser::GenerateFunctionSignature(g_moduleInfo.functions[i]);
        std::wstring lowerFuncName = funcName;
        for (auto& c : lowerFuncName) c = towlower(c);
        
        if (lowerFuncName.find(lowerText) != std::wstring::npos)
        {
            if (!hFunctions)
            {
                hFunctions = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_functions", L"函数"), 1);
            }
            AddTreeItem(hFunctions, funcName, TreeNodeParamHelper::MakeParam(TreeNodeType::FunctionItem, i));
        }
    }

    HTREEITEM hClasses = nullptr;
    for (size_t i = 0; i < g_moduleInfo.classes.size(); ++i)
    {
        std::wstring lowerClassName = g_moduleInfo.classes[i].name;
        for (auto& c : lowerClassName) c = towlower(c);
        
        if (lowerClassName.find(lowerText) != std::wstring::npos)
        {
            if (!hClasses)
            {
                hClasses = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_classes", L"类"), 2);
            }
            AddTreeItem(hClasses, g_moduleInfo.classes[i].name, TreeNodeParamHelper::MakeParam(TreeNodeType::ClassItem, i));
        }
    }

    HTREEITEM hStructs = nullptr;
    for (size_t i = 0; i < g_moduleInfo.structs.size(); ++i)
    {
        std::wstring lowerStructName = g_moduleInfo.structs[i].name;
        for (auto& c : lowerStructName) c = towlower(c);
        
        if (lowerStructName.find(lowerText) != std::wstring::npos)
        {
            if (!hStructs)
            {
                hStructs = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体"), 3);
            }
            AddTreeItem(hStructs, g_moduleInfo.structs[i].name, TreeNodeParamHelper::MakeParam(TreeNodeType::StructItem, i));
        }
    }

    HTREEITEM hUnions = nullptr;
    for (size_t i = 0; i < g_moduleInfo.unions.size(); ++i)
    {
        std::wstring lowerUnionName = g_moduleInfo.unions[i].name;
        for (auto& c : lowerUnionName) c = towlower(c);
        
        if (lowerUnionName.find(lowerText) != std::wstring::npos)
        {
            if (!hUnions)
            {
                hUnions = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体"), 4);
            }
            AddTreeItem(hUnions, g_moduleInfo.unions[i].name, TreeNodeParamHelper::MakeParam(TreeNodeType::UnionItem, i));
        }
    }

    HTREEITEM hEnums = nullptr;
    for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i)
    {
        std::wstring lowerEnumName = g_moduleInfo.enums[i].name;
        for (auto& c : lowerEnumName) c = towlower(c);
        
        if (lowerEnumName.find(lowerText) != std::wstring::npos)
        {
            if (!hEnums)
            {
                hEnums = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举"), 5);
            }
            AddTreeItem(hEnums, g_moduleInfo.enums[i].name, 50000 + (DWORD)i);
        }
    }

    HTREEITEM hGlobalVars = nullptr;
    for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i)
    {
        std::wstring lowerVarName = g_moduleInfo.globalVariables[i].name;
        for (auto& c : lowerVarName) c = towlower(c);
        
        if (lowerVarName.find(lowerText) != std::wstring::npos)
        {
            if (!hGlobalVars)
            {
                hGlobalVars = AddTreeItem(hRoot, LanguageManager::GetInstance().GetString(L"tree_global_variables", L"全局变量"), 6);
            }
            AddTreeItem(hGlobalVars, g_moduleInfo.globalVariables[i].name, TreeNodeParamHelper::MakeParam(TreeNodeType::GlobalVarItem, i));
        }
    }

    TreeView_Expand(hTreeView, hRoot, TVE_EXPAND);
}

LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if ((message == WM_CHAR && wParam == VK_RETURN) || 
        (message == WM_KEYDOWN && wParam == VK_RETURN)) {
        int len = GetWindowTextLengthW(hEditSearch) + 1;
        std::wstring searchText(len, L'\0');
        GetWindowTextW(hEditSearch, &searchText[0], len);
        searchText.resize(len - 1);
        SearchItems(searchText);
        return 0;
    }
    return CallWindowProcW(g_pOldEditProc, hWnd, message, wParam, lParam);
}
