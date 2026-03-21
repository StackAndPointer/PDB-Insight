#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "SearchManager.h"
#include "FontManager.h"
#include "DPIManager.h"
#include <set>

LRESULT CALLBACK SplitterProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        return TRUE;
    case WM_LBUTTONDOWN:
        g_splitterDragging = true;
        SetCapture(hWnd);
        return 0;
    case WM_LBUTTONUP:
        if (g_splitterDragging) {
            g_splitterDragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEMOVE:
        if (g_splitterDragging) {
            POINT pt;
            GetCursorPos(&pt);
            HWND hParent = GetParent(hWnd);
            ScreenToClient(hParent, &pt);
            g_splitterPos = pt.x;
            UpdateSplitterPosition(hParent);
        }
        return 0;
    }
    return CallWindowProc(g_pOldSplitterProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_KEYDOWN)
    {
        if (GetKeyState(VK_CONTROL) < 0)
        {
            if (wParam == 'C')
            {
                int iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (iItem != -1)
                {
                    LVITEM lvi;
                    lvi.iItem = iItem;
                    lvi.iSubItem = g_lastClickedSubItem;
                    lvi.mask = LVIF_TEXT;
                    WCHAR szText[1024];
                    lvi.pszText = szText;
                    lvi.cchTextMax = 1024;
                    ListView_GetItem(hListView, &lvi);
                    std::wstring selectedText = szText;

                    if (OpenClipboard(NULL)) {
                        EmptyClipboard();
                        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (selectedText.length() + 1) * sizeof(WCHAR));
                        if (hGlobal) {
                            LPWSTR pData = (LPWSTR)GlobalLock(hGlobal);
                            if (pData) {
                                wcscpy_s(pData, selectedText.length() + 1, selectedText.c_str());
                                GlobalUnlock(hGlobal);
                                SetClipboardData(CF_UNICODETEXT, hGlobal);
                            }
                        }
                        CloseClipboard();
                    }
                }
                return 0;
            }
            else if (wParam == 'F')
            {
                int iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (iItem != -1)
                {
                    LVITEM lvi;
                    lvi.iItem = iItem;
                    lvi.iSubItem = g_lastClickedSubItem;
                    lvi.mask = LVIF_TEXT;
                    WCHAR szText[1024];
                    lvi.pszText = szText;
                    lvi.cchTextMax = 1024;
                    ListView_GetItem(hListView, &lvi);
                    std::wstring selectedText = szText;

                    SetWindowTextW(hEditSearch, selectedText.c_str());
                    SearchItems(selectedText);
                }
                else
                {
                    SetFocus(hEditSearch);
                }
                return 0;
            }
        }
    }
    else if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        LVHITTESTINFO hti;
        ZeroMemory(&hti, sizeof(hti));
        hti.pt = pt;
        ListView_SubItemHitTest(hListView, &hti);
        g_lastClickedSubItem = (hti.iSubItem >= 0) ? hti.iSubItem : 0;
    }
    return CallWindowProc(g_pOldListViewProc, hWnd, message, wParam, lParam);
}

void UpdateSplitterPosition(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    int statusBarHeight = DPIManager::ScaleY(24);
    int searchBarHeight = DPIManager::ScaleY(30);
    int controlTop = searchBarHeight;
    int controlHeight = rect.bottom - rect.top - statusBarHeight - searchBarHeight;

    int minWidth = DPIManager::ScaleX(100);
    int maxWidth = rect.right - rect.left - DPIManager::ScaleX(200);
    
    // 确保splitter位置在合理范围内
    if (g_splitterPos < minWidth || g_splitterPos > maxWidth) {
        // 如果splitter位置不合理，设置为默认值
        g_splitterPos = rect.right / 3;
    }

    SetWindowPos(hTreeView, nullptr, 0, controlTop, g_splitterPos, controlHeight, SWP_NOZORDER);
    SetWindowPos(hSplitter, nullptr, g_splitterPos, controlTop, DPIManager::ScaleX(4), controlHeight, SWP_NOZORDER);
    SetWindowPos(hTabCtrl, nullptr, g_splitterPos + DPIManager::ScaleX(4), controlTop, (rect.right - rect.left) - (g_splitterPos + DPIManager::ScaleX(4)), controlHeight, SWP_NOZORDER);

    RECT tabRect;
    GetClientRect(hTabCtrl, &tabRect);
    TabCtrl_AdjustRect(hTabCtrl, FALSE, &tabRect);
    SetWindowPos(hListView, nullptr, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, SWP_NOZORDER);
    SetWindowPos(hRichEdit, nullptr, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, SWP_NOZORDER);

    // 调整提示文本控件位置
    if (hInfoText) {
        SetWindowPos(hInfoText, nullptr, 0, rect.bottom - statusBarHeight - DPIManager::ScaleY(20), rect.right - rect.left, DPIManager::ScaleY(20), SWP_NOZORDER);
    }
}

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CONTEXTMENU)
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ShowRichEditContextMenu(hWnd, pt.x, pt.y);
        return 0;
    }
    else if (message == WM_KEYDOWN)
    {
        if (GetKeyState(VK_CONTROL) < 0)
        {
            if (wParam == 'C')
            {
                SendMessageW(hWnd, WM_COPY, 0, 0);
                return 0;
            }
            else if (wParam == 'F')
            {
                CHARRANGE cr;
                SendMessageW(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
                if (cr.cpMin != cr.cpMax)
                {
                    GETTEXTLENGTHEX gtle;
                    gtle.flags = GTL_NUMCHARS;
                    gtle.codepage = 1200;
                    LONG textLen = (LONG)SendMessageW(hWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
                    if (textLen > 0)
                    {
                        std::vector<WCHAR> buffer(textLen + 1);
                        GETTEXTEX gt;
                        gt.cb = (textLen + 1) * sizeof(WCHAR);
                        gt.flags = GT_DEFAULT;
                        gt.codepage = 1200;
                        gt.lpDefaultChar = NULL;
                        gt.lpUsedDefChar = NULL;
                        SendMessageW(hWnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer.data());
                        std::wstring text(buffer.data());
                        std::wstring selectedText = text.substr(cr.cpMin, cr.cpMax - cr.cpMin);
                        SetWindowTextW(hEditSearch, selectedText.c_str());
                        SearchItems(selectedText);
                    }
                }
                else
                {
                    SetFocus(hEditSearch);
                }
                return 0;
            }
        }
    }
    else if (message == WM_LBUTTONDBLCLK)
    {
        CallWindowProc(g_pOldRichEditProc, hWnd, message, wParam, lParam);
        
        CHARRANGE cr;
        SendMessageW(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
        
        GETTEXTLENGTHEX gtle;
        gtle.flags = GTL_NUMCHARS;
        gtle.codepage = 1200;
        LONG textLen = (LONG)SendMessageW(hWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
        
        if (textLen > 0 && cr.cpMin != cr.cpMax)
        {
            std::vector<WCHAR> buffer(textLen + 1);
            GETTEXTEX gt;
            gt.cb = (textLen + 1) * sizeof(WCHAR);
            gt.flags = GT_DEFAULT;
            gt.codepage = 1200;
            gt.lpDefaultChar = NULL;
            gt.lpUsedDefChar = NULL;
            SendMessageW(hWnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer.data());
            std::wstring text(buffer.data());
            
            while (cr.cpMax > cr.cpMin && 
                   (text[cr.cpMax - 1] == L' ' || text[cr.cpMax - 1] == L'\t' || text[cr.cpMax - 1] == L'\r' || text[cr.cpMax - 1] == L'\n'))
            {
                cr.cpMax--;
            }
            
            if (cr.cpMin != cr.cpMax)
            {
                SendMessageW(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
            }
        }
        return 0;
    }
    return CallWindowProc(g_pOldRichEditProc, hWnd, message, wParam, lParam);
}

void ShowRichEditContextMenu(HWND hWnd, int x, int y)
{
    if (!hRichEdit) return;

    HMENU hMenu = CreatePopupMenu();

    CHARRANGE cr;
    SendMessageW(hRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    bool hasSelection = (cr.cpMin != cr.cpMax);

    AppendMenuW(hMenu, hasSelection ? MF_ENABLED : MF_GRAYED, 1001, L"复制");
    AppendMenuW(hMenu, hasSelection ? MF_ENABLED : MF_GRAYED, 1003, L"搜索");

    POINT pt = { x, y };
    if (x == -1 && y == -1) {
        GetCursorPos(&pt);
    }

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd == 1001) {
        SendMessageW(hRichEdit, WM_COPY, 0, 0);
    }
    else if (cmd == 1003) {
        if (hasSelection) {
            GETTEXTLENGTHEX gtle;
            gtle.flags = GTL_NUMCHARS;
            gtle.codepage = 1200;
            LONG textLen = (LONG)SendMessageW(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
            if (textLen > 0) {
                std::vector<WCHAR> buffer(textLen + 1);
                GETTEXTEX gt;
                gt.cb = (textLen + 1) * sizeof(WCHAR);
                gt.flags = GT_DEFAULT;
                gt.codepage = 1200;
                gt.lpDefaultChar = NULL;
                gt.lpUsedDefChar = NULL;
                SendMessageW(hRichEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer.data());
                std::wstring text(buffer.data());
                std::wstring selectedText = text.substr(cr.cpMin, cr.cpMax - cr.cpMin);
                SetWindowTextW(hEditSearch, selectedText.c_str());
                SearchItems(selectedText);
            }
        }
    }
}

void CreateControls(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    int statusBarHeight = 24;
    int searchBarHeight = 30;
    int controlTop = searchBarHeight;
    int controlHeight = rect.bottom - rect.top - statusBarHeight - searchBarHeight;
    g_splitterPos = rect.right / 3;

    hEditSearch = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 5, 300, 24, hWnd, (HMENU)ID_EDIT_SEARCH, hInst, nullptr);

    g_pOldEditProc = (WNDPROC)SetWindowLongPtrW(hEditSearch, GWLP_WNDPROC, (LONG_PTR)SearchEditProc);

    hButtonSearch = CreateWindowExW(0, L"BUTTON", LanguageManager::GetInstance().GetString(L"button_search", L"搜索").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        320, 5, 80, 24, hWnd, (HMENU)ID_BUTTON_SEARCH, hInst, nullptr);
    SendMessageW(hButtonSearch, WM_SETFONT, (WPARAM)FontManager::GetHeaderViewFont(), TRUE);

    hButtonSearchHistory = CreateWindowExW(0, L"BUTTON", LanguageManager::GetInstance().GetString(L"button_search_history", L"▼").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        410, 5, 30, 24, hWnd, (HMENU)ID_BUTTON_SEARCH_HISTORY, hInst, nullptr);
    SendMessageW(hButtonSearchHistory, WM_SETFONT, (WPARAM)FontManager::GetHeaderViewFont(), TRUE);

    hTreeView = CreateWindowExW(0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_VSCROLL,
        0, controlTop, g_splitterPos, controlHeight, hWnd, (HMENU)ID_TREEVIEW, hInst, nullptr);

    hSplitter = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        g_splitterPos, controlTop, 4, controlHeight, hWnd, nullptr, hInst, nullptr);
    g_pOldSplitterProc = (WNDPROC)SetWindowLongPtrW(hSplitter, GWLP_WNDPROC, (LONG_PTR)SplitterProc);

    hTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        g_splitterPos + 4, controlTop, rect.right - g_splitterPos - 4, controlHeight, hWnd, (HMENU)ID_TABCTRL, hInst, nullptr);

    TCITEM tci;
    wcscpy_s(g_szTabText1, LanguageManager::GetInstance().GetString(L"tab_details", L"详细信息").c_str());
    wcscpy_s(g_szTabText2, LanguageManager::GetInstance().GetString(L"tab_header", L"头文件视图").c_str());
    tci.mask = TCIF_TEXT;
    tci.pszText = g_szTabText1;
    TabCtrl_InsertItem(hTabCtrl, 0, &tci);
    tci.pszText = g_szTabText2;
    TabCtrl_InsertItem(hTabCtrl, 1, &tci);

    RECT tabRect;
    GetClientRect(hTabCtrl, &tabRect);
    TabCtrl_AdjustRect(hTabCtrl, FALSE, &tabRect);

    hListView = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | WS_VSCROLL | WS_HSCROLL,
        tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top,
        hTabCtrl, (HMENU)ID_LISTVIEW, hInst, nullptr);

    g_pOldListViewProc = (WNDPROC)SetWindowLongPtrW(hListView, GWLP_WNDPROC, (LONG_PTR)ListViewProc);

    hRichEdit = CreateWindowExW(0, RICHEDIT_CLASS, L"",
        WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top,
        hTabCtrl, (HMENU)ID_RICHEDIT, hInst, nullptr);
    
    ShowWindow(hRichEdit, SW_HIDE);
    
    g_pOldRichEditProc = (WNDPROC)SetWindowLongPtrW(hRichEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
    
    FontManager::ApplyHeaderViewFont(hRichEdit);

    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    int statusParts[] = { -1 };
    hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, hWnd, (HMENU)ID_STATUSBAR, hInst, nullptr);
    SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)statusParts);

    // 创建提示文本控件
    hInfoText = CreateWindowExW(0, L"STATIC", LanguageManager::GetInstance().GetString(L"info_export_hint", L"如果符号信息查看不全请导出后查看").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 0, 0, hWnd, (HMENU)ID_INFO_TEXT, hInst, nullptr);
    SendMessageW(hInfoText, WM_SETFONT, (WPARAM)FontManager::GetHeaderViewFont(), TRUE);

    UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_ready", L"就绪 - 请打开一个 PDB 文件"));
}

void UpdateStatusBar(const std::wstring& text)
{
    if (hStatusBar)
    {
        SendMessageW(hStatusBar, WM_SETTEXT, 0, (LPARAM)text.c_str());
    }
}

void ShowListViewContextMenu(HWND hWnd, int x, int y)
{
    if (!hListView) return;

    int iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    bool hasSelection = (iItem != -1);

    if (!hasSelection) return;

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_ENABLED, 2001, L"复制");
    AppendMenuW(hMenu, MF_ENABLED, 2002, L"搜索");

    POINT pt = { x, y };
    if (x == -1 && y == -1) {
        GetCursorPos(&pt);
    }

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd == 2001 || cmd == 2002) {
        LVITEM lvi;
        lvi.iItem = iItem;
        lvi.iSubItem = g_lastClickedSubItem;
        lvi.mask = LVIF_TEXT;
        WCHAR szText[1024];
        lvi.pszText = szText;
        lvi.cchTextMax = 1024;
        ListView_GetItem(hListView, &lvi);
        std::wstring selectedText = szText;

        if (cmd == 2001) {
            if (OpenClipboard(NULL)) {
                EmptyClipboard();
                HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (selectedText.length() + 1) * sizeof(WCHAR));
                if (hGlobal) {
                    LPWSTR pData = (LPWSTR)GlobalLock(hGlobal);
                    if (pData) {
                        wcscpy_s(pData, selectedText.length() + 1, selectedText.c_str());
                        GlobalUnlock(hGlobal);
                        SetClipboardData(CF_UNICODETEXT, hGlobal);
                    }
                }
                CloseClipboard();
            }
        }
        else if (cmd == 2002) {
            SetWindowTextW(hEditSearch, selectedText.c_str());
            SearchItems(selectedText);
        }
    }
}

void UpdateTabViews()
{
    int tabIndex = TabCtrl_GetCurSel(hTabCtrl);
    
    if (tabIndex == 0)
    {
        ShowWindow(hListView, SW_SHOW);
        ShowWindow(hRichEdit, SW_HIDE);
        HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
        if (hSelected && g_pdbLoaded) {
            PopulateListView(hSelected);
        }
    }
    else
    {
        ShowWindow(hListView, SW_HIDE);
        ShowWindow(hRichEdit, SW_SHOW);
        FontManager::ApplyHeaderViewFont(hRichEdit);
        HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
        if (hSelected && g_pdbLoaded) {
            ShowHeaderView(hSelected);
        }
    }
}
