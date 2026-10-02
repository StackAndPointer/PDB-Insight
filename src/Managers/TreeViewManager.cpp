#include "TreeViewManager.h"
#include "ControlsManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "LanguageManager.h"
#include "LoadingManager.h"
#include "TreeNodeHelper.h"

namespace {
constexpr size_t kLazyTreeThreshold = 5000;
constexpr size_t kTreeBatchSize = 1000;

std::vector<size_t> BuildSequentialIndices(size_t count) {
    std::vector<size_t> indices;
    if (count > 0) {
        indices.reserve(count);
        for (size_t i = 0; i < count; ++i) indices.push_back(i);
    }
    return indices;
}

size_t GetCategoryItemCount(TreeCategory category) {
    switch (category) {
        case TreeCategory::Functions: return g_moduleInfo.functions.size();
        case TreeCategory::Classes: return g_moduleInfo.classes.size();
        case TreeCategory::Structs: return g_moduleInfo.structs.size();
        case TreeCategory::Unions: return g_moduleInfo.unions.size();
        case TreeCategory::Enums: return g_moduleInfo.enums.size();
        case TreeCategory::GlobalVariables: return g_moduleInfo.globalVariables.size();
        default: return 0;
    }
}

std::wstring GetCategoryItemText(TreeCategory category, size_t index) {
    switch (category) {
        case TreeCategory::Functions: {
            if (index >= g_moduleInfo.functions.size()) return L"";
            const auto& function = g_moduleInfo.functions[index];
            return function.displaySignature.empty()
                ? PDBParser::GenerateFunctionSignature(function) : function.displaySignature;
        }
        case TreeCategory::Classes:
            return index < g_moduleInfo.classes.size() ? g_moduleInfo.classes[index].name : L"";
        case TreeCategory::Structs:
            return index < g_moduleInfo.structs.size() ? g_moduleInfo.structs[index].name : L"";
        case TreeCategory::Unions:
            return index < g_moduleInfo.unions.size() ? g_moduleInfo.unions[index].name : L"";
        case TreeCategory::Enums:
            return index < g_moduleInfo.enums.size() ? g_moduleInfo.enums[index].name : L"";
        case TreeCategory::GlobalVariables:
            return index < g_moduleInfo.globalVariables.size()
                ? g_moduleInfo.globalVariables[index].name : L"";
        default:
            return L"";
    }
}

LPARAM GetCategoryItemParam(TreeCategory category, size_t index) {
    switch (category) {
        case TreeCategory::Functions:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::FunctionItem, index);
        case TreeCategory::Classes:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::ClassItem, index);
        case TreeCategory::Structs:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::StructItem, index);
        case TreeCategory::Unions:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::UnionItem, index);
        case TreeCategory::Enums:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::EnumItem, index);
        case TreeCategory::GlobalVariables:
            return TreeNodeParamHelper::MakeParam(TreeNodeType::GlobalVarItem, index);
        default:
            return 0;
    }
}

void AddCategoryItems(HTREEITEM parent, TreeCategory category) {
    const size_t itemCount = GetCategoryItemCount(category);
    const auto& filter = g_virtualTreeState.filteredIndices;
    if (!filter.empty()) {
        for (size_t index : filter) {
            if (index >= itemCount) continue;
            AddTreeItem(parent, GetCategoryItemText(category, index),
                GetCategoryItemParam(category, index));
        }
        return;
    }

    for (size_t index = 0; index < itemCount; ++index) {
        AddTreeItem(parent, GetCategoryItemText(category, index),
            GetCategoryItemParam(category, index));
    }
}

HTREEITEM AddLazyCategory(HTREEITEM root, const std::wstring& text,
                          TreeCategory category, size_t itemCount) {
    HTREEITEM categoryItem = AddTreeItem(root, text, static_cast<LPARAM>(category));
    if (itemCount > 0) {
        // A single placeholder keeps the expand glyph; it is removed and
        // replaced by the first real batch when the node is expanded.
        AddTreeItem(categoryItem, L"", static_cast<LPARAM>(-1));
    }
    return categoryItem;
}
}

void PopulateTreeView() {
    if (!hTreeView) return;
    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(hTreeView);

    if (!g_pdbLoaded) {
        SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
        return;
    }

    ResetVirtualTreeState();
    ResetPagedTreeCategory();
    HTREEITEM root = AddTreeItem(TVI_ROOT,
        g_moduleInfo.name.empty() ? LANG_STR(L"tree_module") : g_moduleInfo.name, 0);

    struct CategoryDefinition {
        TreeCategory category;
        const wchar_t* key;
        const wchar_t* fallback;
        size_t count;
    };
    const CategoryDefinition categories[] = {
        {TreeCategory::Functions, L"tree_functions", L"Functions", g_moduleInfo.functions.size()},
        {TreeCategory::Classes, L"tree_classes", L"Classes", g_moduleInfo.classes.size()},
        {TreeCategory::Structs, L"tree_structs", L"Structs", g_moduleInfo.structs.size()},
        {TreeCategory::Unions, L"tree_unions", L"Unions", g_moduleInfo.unions.size()},
        {TreeCategory::Enums, L"tree_enums", L"Enums", g_moduleInfo.enums.size()},
        {TreeCategory::GlobalVariables, L"tree_global_variables", L"Global Variables",
         g_moduleInfo.globalVariables.size()}
    };

    for (const auto& category : categories) {
        const std::wstring label = LanguageManager::GetInstance().GetString(category.key,
            category.fallback);
        if (category.count >= kLazyTreeThreshold) {
            AddLazyCategory(root, label + L" (" + std::to_wstring(category.count) + L")",
                category.category, category.count);
        } else {
            HTREEITEM item = AddTreeItem(root, label, static_cast<LPARAM>(category.category));
            AddCategoryItems(item, category.category);
        }
    }

    TreeView_Expand(hTreeView, root, TVE_EXPAND);
    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
}

std::wstring GetSearchResultItemText(TreeCategory category, size_t index) {
    return GetCategoryItemText(category, index);
}

LPARAM GetSearchResultItemParam(TreeCategory category, size_t index) {
    return GetCategoryItemParam(category, index);
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

void ExpandLazyTreeItem(HTREEITEM item) {
    if (!item || !g_pdbLoaded) return;

    TVITEM treeItem{};
    treeItem.hItem = item;
    treeItem.mask = TVIF_PARAM | TVIF_CHILDREN;
    if (!TreeView_GetItem(hTreeView, &treeItem)) return;

    const LPARAM param = treeItem.lParam;
    if (param < 1 || param > 6 || treeItem.cChildren != 1) return;

    const TreeCategory category = static_cast<TreeCategory>(param);
    const size_t itemCount = GetCategoryItemCount(category);
    if (itemCount < kLazyTreeThreshold) return;

    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    HTREEITEM child = TreeView_GetChild(hTreeView, item);
    while (child) {
        HTREEITEM next = TreeView_GetNextSibling(hTreeView, child);
        TreeView_DeleteItem(hTreeView, child);
        child = next;
    }

    g_virtualTreeState.category = category;
    g_virtualTreeState.itemCount = itemCount;
    g_virtualTreeState.pendingTreeIndex = 0;
    g_virtualTreeState.treeParent = reinterpret_cast<void*>(item);
    g_virtualTreeState.filteredIndices.clear();

    // Materialize the first batch only. The rest is appended as the user
    // scrolls, so expanding a category never blocks for tens of thousands of
    // TreeView nodes.
    const size_t end = (std::min)(kTreeBatchSize, itemCount);
    for (size_t index = 0; index < end; ++index) {
        AddTreeItem(item, GetCategoryItemText(category, index),
            GetCategoryItemParam(category, index));
    }
    g_virtualTreeState.pendingTreeIndex = end;

    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
}

void AutoExtendTreeCategory() {
    if (!g_pdbLoaded || !hTreeView) return;
    if (g_virtualTreeState.category == TreeCategory::None) return;
    if (g_virtualTreeState.treeParent == nullptr) return;
    if (g_virtualTreeState.pendingTreeIndex >= g_virtualTreeState.itemCount) return;

    // Append the next batch only when the last already-materialized child is
    // visible, so a small wheel tick does not eagerly build every batch.
    HTREEITEM lastChild = TreeView_GetChild(hTreeView, reinterpret_cast<HTREEITEM>(g_virtualTreeState.treeParent));
    if (!lastChild) return;
    while (TreeView_GetNextSibling(hTreeView, lastChild)) {
        lastChild = TreeView_GetNextSibling(hTreeView, lastChild);
    }
    RECT itemRect{};
    if (!TreeView_GetItemRect(hTreeView, lastChild, &itemRect, TRUE)) return;
    RECT clientRect{};
    GetClientRect(hTreeView, &clientRect);
    const int triggerLine = clientRect.bottom - (clientRect.bottom - clientRect.top) / 4;
    if (itemRect.bottom < triggerLine) return;

    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    size_t index = g_virtualTreeState.pendingTreeIndex;
    const size_t end = (std::min)(index + kTreeBatchSize, g_virtualTreeState.itemCount);
    for (; index < end; ++index) {
        AddTreeItem(reinterpret_cast<HTREEITEM>(g_virtualTreeState.treeParent),
            GetCategoryItemText(g_virtualTreeState.category, index),
            GetCategoryItemParam(g_virtualTreeState.category, index));
    }
    g_virtualTreeState.pendingTreeIndex = index;
    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
}

void ResetPagedTreeCategory() {
    g_virtualTreeState.pendingTreeIndex = 0;
    g_virtualTreeState.treeParent = nullptr;
    g_virtualTreeState.itemCount = 0;
    g_virtualTreeState.category = TreeCategory::None;
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
