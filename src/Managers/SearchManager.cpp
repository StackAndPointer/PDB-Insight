#include "SearchManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "LanguageManager.h"
#include "TreeNodeHelper.h"

#include <algorithm>
#include <vector>

namespace {

// Maximum number of characters we are willing to lowercase for a single
// candidate symbol. Extremely long signatures are skipped to keep the search
// responsive without dropping the whole category.
constexpr size_t kMaxSearchCandidateLength = 200000;

// How many matches are materialized as real TreeView nodes per batch when a
// result category is expanded. The remaining matches stay stored as indices
// and are appended on demand, so a query with hundreds of thousands of hits
// never blocks the UI.
constexpr size_t kSearchBatchSize = 1000;

enum class SearchNodeKind : DWORD {
    Category = 1,
    LoadMore = 2
};

struct SearchResultState {
    bool active = false;
    std::wstring query;
    std::wstring lowerQuery;
    bool truncated = false;
    size_t totalMatches = 0;
    std::vector<size_t> functions;
    std::vector<size_t> classes;
    std::vector<size_t> structs;
    std::vector<size_t> unions;
    std::vector<size_t> enums;
    std::vector<size_t> variables;
    size_t functionCursor = 0;
    size_t classCursor = 0;
    size_t structCursor = 0;
    size_t unionCursor = 0;
    size_t enumCursor = 0;
    size_t variableCursor = 0;
};

SearchResultState g_searchResults;

std::wstring ToLowerCopy(const std::wstring& value) {
    std::wstring lower = value;
    for (auto& c : lower) c = towlower(c);
    return lower;
}

HTREEITEM AddSearchNode(HTREEITEM parent, const std::wstring& text,
                        SearchNodeKind kind, TreeCategory category) {
    const LPARAM param = kSearchResultNodeParamBase +
        (static_cast<LPARAM>(kind) << 8) + static_cast<LPARAM>(category);
    return AddTreeItem(parent, text, param);
}

bool DecodeSearchNodeParam(LPARAM param, SearchNodeKind& kind,
                           TreeCategory& category) {
    if (param < kSearchResultNodeParamBase ||
        param >= kSearchResultNodeParamBase + 0x1000) {
        return false;
    }
    const LPARAM offset = param - kSearchResultNodeParamBase;
    kind = static_cast<SearchNodeKind>((offset >> 8) & 0xF);
    category = static_cast<TreeCategory>(offset & 0xFF);
    return true;
}


const std::vector<size_t>* GetSearchIndices(TreeCategory category) {
    switch (category) {
        case TreeCategory::Functions: return &g_searchResults.functions;
        case TreeCategory::Classes: return &g_searchResults.classes;
        case TreeCategory::Structs: return &g_searchResults.structs;
        case TreeCategory::Unions: return &g_searchResults.unions;
        case TreeCategory::Enums: return &g_searchResults.enums;
        case TreeCategory::GlobalVariables: return &g_searchResults.variables;
        default: return nullptr;
    }
}

size_t* GetSearchCursor(TreeCategory category) {
    switch (category) {
        case TreeCategory::Functions: return &g_searchResults.functionCursor;
        case TreeCategory::Classes: return &g_searchResults.classCursor;
        case TreeCategory::Structs: return &g_searchResults.structCursor;
        case TreeCategory::Unions: return &g_searchResults.unionCursor;
        case TreeCategory::Enums: return &g_searchResults.enumCursor;
        case TreeCategory::GlobalVariables: return &g_searchResults.variableCursor;
        default: return nullptr;
    }
}

std::wstring BuildLoadMoreText(TreeCategory category, size_t shown,
                               size_t total) {
    const std::wstring label = LANG_STR(L"search_load_more");
    std::wstring text = label.empty() ? L"Load more" : label;
    text += L" (";
    text += std::to_wstring(shown);
    text += L"/";
    text += std::to_wstring(total);
    text += L")";
    return text;
}

void AppendSearchBatch(HTREEITEM categoryItem, TreeCategory category) {
    const std::vector<size_t>* indices = GetSearchIndices(category);
    size_t* cursor = GetSearchCursor(category);
    if (!indices || !cursor) return;

    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteItem(hTreeView, TreeView_GetChild(hTreeView, categoryItem));

    const size_t total = indices->size();
    const size_t begin = *cursor;
    const size_t end = (std::min)(begin + kSearchBatchSize, total);
    for (size_t i = begin; i < end; ++i) {
        const size_t index = (*indices)[i];
        AddTreeItem(categoryItem, GetSearchResultItemText(category, index),
            GetSearchResultItemParam(category, index));
    }
    *cursor = end;
    if (end < total) {
        AddSearchNode(categoryItem, BuildLoadMoreText(category, end, total),
            SearchNodeKind::LoadMore, category);
    }

    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
}

}

void SearchItems(const std::wstring& text, bool addToHistory) {
    if (text.empty()) {
        g_searchResults = SearchResultState{};
        PopulateTreeView();
        return;
    }
    if (!g_pdbLoaded) return;

    if (addToHistory) ConfigManager::GetInstance().AddSearchHistory(text);

    g_searchResults = SearchResultState{};
    g_searchResults.active = true;
    g_searchResults.query = text;
    g_searchResults.lowerQuery = ToLowerCopy(text);

    auto matches = [&](const std::wstring& name) {
        if (name.empty()) return false;
        if (name.size() > kMaxSearchCandidateLength) return false;
        return ToLowerCopy(name).find(g_searchResults.lowerQuery) != std::wstring::npos;
    };

    for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i) {
        const auto& function = g_moduleInfo.functions[i];
        const std::wstring& name = function.displaySignature.empty()
            ? function.name : function.displaySignature;
        if (matches(name)) g_searchResults.functions.push_back(i);
    }
    for (size_t i = 0; i < g_moduleInfo.classes.size(); ++i) {
        if (matches(g_moduleInfo.classes[i].name)) g_searchResults.classes.push_back(i);
    }
    for (size_t i = 0; i < g_moduleInfo.structs.size(); ++i) {
        if (matches(g_moduleInfo.structs[i].name)) g_searchResults.structs.push_back(i);
    }
    for (size_t i = 0; i < g_moduleInfo.unions.size(); ++i) {
        if (matches(g_moduleInfo.unions[i].name)) g_searchResults.unions.push_back(i);
    }
    for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i) {
        if (matches(g_moduleInfo.enums[i].name)) g_searchResults.enums.push_back(i);
    }
    for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i) {
        if (matches(g_moduleInfo.globalVariables[i].name)) g_searchResults.variables.push_back(i);
    }

    g_searchResults.totalMatches =
        g_searchResults.functions.size() + g_searchResults.classes.size() +
        g_searchResults.structs.size() + g_searchResults.unions.size() +
        g_searchResults.enums.size() + g_searchResults.variables.size();

    SendMessageW(hTreeView, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(hTreeView);
    ResetVirtualTreeState();

    HTREEITEM root = AddTreeItem(TVI_ROOT,
        LANG_STR(L"search_results") + L" - " + text, 0);

    const struct {
        TreeCategory category;
        const wchar_t* key;
        const wchar_t* fallback;
        size_t count;
    } definitions[] = {
        {TreeCategory::Functions, L"tree_functions", L"Functions", g_searchResults.functions.size()},
        {TreeCategory::Classes, L"tree_classes", L"Classes", g_searchResults.classes.size()},
        {TreeCategory::Structs, L"tree_structs", L"Structs", g_searchResults.structs.size()},
        {TreeCategory::Unions, L"tree_unions", L"Unions", g_searchResults.unions.size()},
        {TreeCategory::Enums, L"tree_enums", L"Enums", g_searchResults.enums.size()},
        {TreeCategory::GlobalVariables, L"tree_global_variables", L"Global Variables",
         g_searchResults.variables.size()}
    };

    for (const auto& definition : definitions) {
        if (definition.count == 0) continue;
        std::wstring label = LanguageManager::GetInstance().GetString(
            definition.key, definition.fallback);
        label += L" (" + std::to_wstring(definition.count) + L")";
        HTREEITEM categoryItem = AddTreeItem(root, label,
            kSearchResultNodeParamBase + (static_cast<LPARAM>(SearchNodeKind::Category) << 8) + static_cast<LPARAM>(definition.category));
        if (categoryItem) {
            AddSearchNode(categoryItem, L"", SearchNodeKind::LoadMore,
                definition.category);
        }
    }

    if (g_searchResults.totalMatches == 0) {
        AddTreeItem(root, LANG_STR(L"search_no_results"), static_cast<LPARAM>(-1));
    }

    TreeView_Expand(hTreeView, root, TVE_EXPAND);
    SendMessageW(hTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTreeView, nullptr, TRUE);
    UpdateStatusBar(LANG_STR(L"search_results") + L": " +
        std::to_wstring(g_searchResults.totalMatches));
}

bool HandleSearchTreeNotification(const NMTREEVIEW* notification) {
    if (!notification) return false;
    SearchNodeKind kind{};
    TreeCategory category{};
    if (!DecodeSearchNodeParam(notification->itemNew.lParam, kind, category)) return false;

    if (notification->hdr.code == TVN_ITEMEXPANDING &&
        notification->action == TVE_EXPAND &&
        kind == SearchNodeKind::LoadMore) {
        HTREEITEM categoryItem = TreeView_GetParent(hTreeView,
            notification->itemNew.hItem);
        if (categoryItem) AppendSearchBatch(categoryItem, category);
        return true;
    }
    return false;
}

bool HandleSearchTreeSelection(HTREEITEM item) {
    if (!item) return false;
    TVITEM treeItem{};
    treeItem.hItem = item;
    treeItem.mask = TVIF_PARAM;
    if (!TreeView_GetItem(hTreeView, &treeItem)) return false;

    SearchNodeKind kind{};
    TreeCategory category{};
    if (!DecodeSearchNodeParam(treeItem.lParam, kind, category)) return false;

    if (kind == SearchNodeKind::LoadMore) {
        HTREEITEM categoryItem = TreeView_GetParent(hTreeView, item);
        if (categoryItem) AppendSearchBatch(categoryItem, category);
        return true;
    }
    return false;
}

void AutoLoadMoreSearchResults() {
    if (!g_searchResults.active || !hTreeView) return;

    // The load-more sentinel is the last child in each search category. When
    // scrolling makes it visible, pull in the next batch without requiring an
    // extra click.
    HTREEITEM root = TreeView_GetRoot(hTreeView);
    if (!root) return;
    for (HTREEITEM categoryItem = TreeView_GetChild(hTreeView, root);
         categoryItem; categoryItem = TreeView_GetNextSibling(hTreeView, categoryItem)) {
        TVITEM categoryTreeItem{};
        categoryTreeItem.hItem = categoryItem;
        categoryTreeItem.mask = TVIF_PARAM | TVIF_CHILDREN;
        if (!TreeView_GetItem(hTreeView, &categoryTreeItem)) continue;

        SearchNodeKind kind{};
        TreeCategory category{};
        if (!DecodeSearchNodeParam(categoryTreeItem.lParam, kind, category)) continue;
        if (kind != SearchNodeKind::Category) continue;

        HTREEITEM sentinel = TreeView_GetChild(hTreeView, categoryItem);
        while (sentinel && TreeView_GetNextSibling(hTreeView, sentinel)) {
            sentinel = TreeView_GetNextSibling(hTreeView, sentinel);
        }
        if (!sentinel) continue;

        TVITEM sentinelItem{};
        sentinelItem.hItem = sentinel;
        sentinelItem.mask = TVIF_PARAM;
        if (!TreeView_GetItem(hTreeView, &sentinelItem)) continue;
        if (!DecodeSearchNodeParam(sentinelItem.lParam, kind, category)) continue;
        if (kind != SearchNodeKind::LoadMore) continue;

        RECT itemRect{};
        if (!TreeView_GetItemRect(hTreeView, sentinel, &itemRect, TRUE)) continue;
        RECT clientRect{};
        GetClientRect(hTreeView, &clientRect);
        if (itemRect.top > clientRect.bottom || itemRect.bottom < 0) continue;

        AppendSearchBatch(categoryItem, category);
        return;
    }
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
