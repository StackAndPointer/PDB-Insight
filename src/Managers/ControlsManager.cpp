#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "ListViewManager.h"
#include "HeaderViewManager.h"
#include "SearchManager.h"
#include "FontManager.h"
#include "DPIManager.h"
#include <memory>
#include <set>
#include <uxtheme.h>

#pragma comment(lib, "uxtheme.lib")

namespace {
WCHAR g_searchTooltip[128] = L"";
WCHAR g_clearTooltip[128] = L"";
WCHAR g_historyTooltip[128] = L"";

constexpr int kGap = 6;
constexpr int kTopHeight = 36;
constexpr int kSplitterWidth = 6;

int SplitterMinimum() {
    return DPIManager::ScaleX(180);
}

int SplitterMaximum(HWND hWnd) {
    RECT rect{};
    GetClientRect(hWnd, &rect);
    return max(SplitterMinimum(), rect.right - DPIManager::ScaleX(320));
}

void ClampSplitter(HWND hWnd) {
    g_splitterPos = max(SplitterMinimum(), min(g_splitterPos, SplitterMaximum(hWnd)));
}

void MoveSplitter(HWND hWnd, int delta) {
    g_splitterPos += delta;
    ClampSplitter(hWnd);
    LayoutMainWindow(GetParent(hWnd));
}

void CopySelectedListCell() {
    int item = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (item < 0) return;

    WCHAR buffer[4096] = L"";
    LVITEMW lvi{};
    lvi.iItem = item;
    lvi.iSubItem = g_lastClickedSubItem;
    lvi.mask = LVIF_TEXT;
    lvi.pszText = buffer;
    lvi.cchTextMax = _countof(buffer);
    ListView_GetItem(hListView, &lvi);

    std::wstring selectedText = buffer;
    if (!OpenClipboard(g_hMainWindow)) return;
    EmptyClipboard();
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (selectedText.size() + 1) * sizeof(WCHAR));
    if (memory) {
        auto data = static_cast<WCHAR*>(GlobalLock(memory));
        if (data) {
            wcscpy_s(data, selectedText.size() + 1, selectedText.c_str());
            GlobalUnlock(memory);
            SetClipboardData(CF_UNICODETEXT, memory);
        } else {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
}

void SearchSelectedListCell() {
    int item = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (item < 0) {
        SetFocus(hEditSearch);
        return;
    }

    WCHAR buffer[4096] = L"";
    LVITEMW lvi{};
    lvi.iItem = item;
    lvi.iSubItem = g_lastClickedSubItem;
    lvi.mask = LVIF_TEXT;
    lvi.pszText = buffer;
    lvi.cchTextMax = _countof(buffer);
    ListView_GetItem(hListView, &lvi);
    SetWindowTextW(hEditSearch, buffer);
    SearchItems(buffer);
}

void AddTooltip(HWND tool, const WCHAR* text, UINT id) {
    TOOLINFOW info{};
    info.cbSize = sizeof(info);
    info.hwnd = tool;
    info.uId = id;
    info.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    info.lpszText = const_cast<WCHAR*>(text);
    GetClientRect(tool, &info.rect);
    SendMessageW(hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&info));
}

void UpdateTooltipText(HWND tool, UINT id, WCHAR* buffer, size_t capacity, const std::wstring& text) {
    wcscpy_s(buffer, capacity, text.c_str());
    TOOLINFOW info{};
    info.cbSize = sizeof(info);
    info.hwnd = tool;
    info.uId = id;
    info.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    info.lpszText = buffer;
    GetClientRect(tool, &info.rect);
    SendMessageW(hTooltip, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&info));
}
}

LRESULT CALLBACK ClearButtonProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = CallWindowProcW(g_pOldClearButtonProc, hWnd, message, wParam, lParam);
    if (message == BM_CLICK || message == WM_LBUTTONUP ||
        message == WM_LBUTTONDOWN ||
        (message == WM_KEYUP && (wParam == VK_SPACE || wParam == VK_RETURN))) {
        HWND edit = GetDlgItem(GetParent(hWnd), ID_EDIT_SEARCH);
        if (edit) SetWindowTextW(edit, L"");
        ClearSearchBox();
    }
    return result;
}

LRESULT CALLBACK SplitterProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return TRUE;
    case WM_SETFOCUS:
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;
    case WM_LBUTTONDOWN:
        g_splitterDragging = true;
        SetFocus(hWnd);
        SetCapture(hWnd);
        return 0;
    case WM_LBUTTONUP:
        if (g_splitterDragging) {
            g_splitterDragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_CAPTURECHANGED:
        g_splitterDragging = false;
        return 0;
    case WM_MOUSEMOVE:
        if (g_splitterDragging) {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(hWnd, &pt);
            ScreenToClient(GetParent(hWnd), &pt);
            g_splitterPos = pt.x - kSplitterWidth / 2;
            ClampSplitter(GetParent(hWnd));
            LayoutMainWindow(GetParent(hWnd));
        }
        return 0;
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
    case WM_KEYDOWN:
        if (wParam == VK_LEFT) {
            MoveSplitter(hWnd, -DPIManager::ScaleX(16));
            return 0;
        }
        if (wParam == VK_RIGHT) {
            MoveSplitter(hWnd, DPIManager::ScaleX(16));
            return 0;
        }
        if (wParam == VK_HOME) {
            g_splitterPos = SplitterMinimum();
            LayoutMainWindow(GetParent(hWnd));
            return 0;
        }
        if (wParam == VK_END) {
            g_splitterPos = SplitterMaximum(GetParent(hWnd));
            LayoutMainWindow(GetParent(hWnd));
            return 0;
        }
        break;
    case WM_LBUTTONDBLCLK:
        RECT rect{};
        GetClientRect(GetParent(hWnd), &rect);
        g_splitterPos = rect.right / 3;
        LayoutMainWindow(GetParent(hWnd));
        return 0;
    }
    return CallWindowProcW(g_pOldSplitterProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0) {
        if (wParam == 'C') {
            CopySelectedListCell();
            return 0;
        }
        if (wParam == 'F') {
            SearchSelectedListCell();
            return 0;
        }
    }
    if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) {
        LVHITTESTINFO hit{};
        hit.pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ListView_SubItemHitTest(hListView, &hit);
        g_lastClickedSubItem = hit.iSubItem >= 0 ? hit.iSubItem : 0;
    }
    if (message == WM_CONTEXTMENU) {
        ShowListViewContextMenu(GetParent(hWnd), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }
    return CallWindowProcW(g_pOldListViewProc, hWnd, message, wParam, lParam);
}

namespace {
bool g_richEditSelecting = false;
LONG g_richEditSelectionAnchor = 0;

LONG RichEditCharFromPoint(HWND hWnd, LPARAM lParam) {
    POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    LRESULT index = SendMessageW(hWnd, EM_CHARFROMPOS, 0, reinterpret_cast<LPARAM>(&point));
    LONG length = GetWindowTextLengthW(hWnd);
    return max(0L, min(static_cast<LONG>(index), length));
}
}

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0) {
        if (wParam == 'C') {
            SendMessageW(hWnd, WM_COPY, 0, 0);
            return 0;
        }
        if (wParam == 'F') {
            CHARRANGE selection{};
            SendMessageW(hWnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
            if (selection.cpMin != selection.cpMax) {
                const int length = GetWindowTextLengthW(hWnd);
                std::wstring text(static_cast<size_t>(length) + 1, L'\0');
                GetWindowTextW(hWnd, &text[0], length + 1);
                text.resize(static_cast<size_t>(length));
                std::wstring selected =
                    text.substr(selection.cpMin, selection.cpMax - selection.cpMin);
                SetWindowTextW(hEditSearch, selected.c_str());
                SearchItems(selected);
            } else {
                SetFocus(hEditSearch);
            }
            return 0;
        }
    }
    if (message == WM_LBUTTONDOWN) {
        SetFocus(hWnd);
        LRESULT result = CallWindowProcW(g_pOldRichEditProc, hWnd, message, wParam, lParam);
        g_richEditSelectionAnchor = RichEditCharFromPoint(hWnd, lParam);
        g_richEditSelecting = true;
        if (GetCapture() != hWnd) SetCapture(hWnd);
        SendMessageW(hWnd, EM_SETSEL, g_richEditSelectionAnchor, g_richEditSelectionAnchor);
        return result;
    }
    if (message == WM_MOUSEMOVE && g_richEditSelecting) {
        LRESULT result = CallWindowProcW(g_pOldRichEditProc, hWnd, message, wParam, lParam);
        LONG current = RichEditCharFromPoint(hWnd, lParam);
        LONG begin = min(g_richEditSelectionAnchor, current);
        LONG end = max(g_richEditSelectionAnchor, current);
        SendMessageW(hWnd, EM_SETSEL, begin, end);
        return result;
    }
    if (message == WM_LBUTTONUP || message == WM_CAPTURECHANGED || message == WM_CANCELMODE) {
        LRESULT result = CallWindowProcW(g_pOldRichEditProc, hWnd, message, wParam, lParam);
        g_richEditSelecting = false;
        if (message == WM_LBUTTONUP && GetCapture() == hWnd) ReleaseCapture();
        return result;
    }
    if (message == WM_RBUTTONDOWN) {
        POINT screenPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ClientToScreen(hWnd, &screenPoint);
        ShowRichEditContextMenu(GetParent(hWnd), screenPoint.x, screenPoint.y);
        return 0;
    }
    if (message == WM_CONTEXTMENU) {
        ShowRichEditContextMenu(GetParent(hWnd), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }
    return CallWindowProcW(g_pOldRichEditProc, hWnd, message, wParam, lParam);
}

void ShowRichEditContextMenu(HWND hWnd, int x, int y) {
    if (!hRichEdit) return;
    CHARRANGE selection{};
    SendMessageW(hRichEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
    bool hasSelection = selection.cpMin != selection.cpMax;

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, hasSelection ? MF_ENABLED : MF_GRAYED, 1001, LANG_STR(L"menu_copy").c_str());
    AppendMenuW(menu, hasSelection ? MF_ENABLED : MF_GRAYED, 1003, LANG_STR(L"menu_search").c_str());

    POINT pt{x, y};
    if (x == -1 && y == -1) GetCursorPos(&pt);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(menu);

    if (command == 1001) {
        SendMessageW(hRichEdit, WM_COPY, 0, 0);
    } else if (command == 1003 && hasSelection) {
        int length = GetWindowTextLengthW(hRichEdit);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        GetWindowTextW(hRichEdit, &text[0], length + 1);
        text.resize(length);
        std::wstring selected = text.substr(selection.cpMin, selection.cpMax - selection.cpMin);
        SetWindowTextW(hEditSearch, selected.c_str());
        SearchItems(selected);
    }
}

void ShowListViewContextMenu(HWND hWnd, int x, int y) {
    if (!hListView || ListView_GetNextItem(hListView, -1, LVNI_SELECTED) < 0) return;

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_ENABLED, 2001, LANG_STR(L"menu_copy").c_str());
    AppendMenuW(menu, MF_ENABLED, 2002, LANG_STR(L"menu_search").c_str());

    POINT pt{x, y};
    if (x == -1 && y == -1) GetCursorPos(&pt);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(menu);

    if (command == 2001) CopySelectedListCell();
    if (command == 2002) SearchSelectedListCell();
}

void CreateControls(HWND hWnd) {
    g_hMainWindow = hWnd;

    hTooltip = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, 0, 0, 0, 0, hWnd, nullptr, hInst, nullptr);

    hEditSearch = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_WANTRETURN,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_EDIT_SEARCH)), hInst, nullptr);
    g_pOldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hEditSearch, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SearchEditProc)));

    hButtonClearSearch = CreateWindowExW(0, L"BUTTON", L"×",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_BUTTON_CLEAR_SEARCH)), hInst, nullptr);
    g_pOldClearButtonProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        hButtonClearSearch, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ClearButtonProc)));

    hButtonSearch = CreateWindowExW(0, L"BUTTON", LANG_STR(L"button_search").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_BUTTON_SEARCH)), hInst, nullptr);

    hButtonSearchHistory = CreateWindowExW(0, L"BUTTON", L"▼",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_BUTTON_SEARCH_HISTORY)), hInst, nullptr);

    hProgressTask = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        WS_CHILD | PBS_SMOOTH,
        0, 0, 0, 0, hWnd, nullptr, hInst, nullptr);
    SendMessageW(hProgressTask, PBM_SETRANGE32, 0, 100);

    hButtonCancelTask = CreateWindowExW(0, L"BUTTON", LANG_STR(L"button_cancel_task").c_str(),
        WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_BUTTON_CANCEL_TASK)), hInst, nullptr);

    hTreeView = CreateWindowExW(0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_VSCROLL,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_TREEVIEW)), hInst, nullptr);

    hSplitter = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | SS_NOTIFY,
        0, 0, 0, 0, hWnd, nullptr, hInst, nullptr);
    g_pOldSplitterProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hSplitter, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SplitterProc)));

    hTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_TABCTRL)), hInst, nullptr);

    wcscpy_s(g_szTabText1, LANG_STR(L"tab_details").c_str());
    wcscpy_s(g_szTabText2, LANG_STR(L"tab_header").c_str());
    TCITEM item{};
    item.mask = TCIF_TEXT;
    item.pszText = g_szTabText1;
    TabCtrl_InsertItem(hTabCtrl, 0, &item);
    item.pszText = g_szTabText2;
    TabCtrl_InsertItem(hTabCtrl, 1, &item);

    hListView = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_VSCROLL | WS_HSCROLL,
        0, 0, 0, 0, hTabCtrl, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_LISTVIEW)), hInst, nullptr);
    g_pOldListViewProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hListView, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ListViewProc)));

    hRichEdit = CreateWindowExW(WS_EX_CLIENTEDGE, RICHEDIT_CLASSW, L"",
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
        0, 0, 0, 0, hTabCtrl, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_RICHEDIT)), hInst, nullptr);
    g_pOldRichEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hRichEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(RichEditProc)));
    ShowWindow(hRichEdit, SW_HIDE);
    hButtonCopyHeader = CreateWindowExW(0, L"BUTTON", LANG_STR(L"menu_copy").c_str(),
        WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_BUTTON_COPY_HEADER)), hInst, nullptr);
    ShowWindow(hButtonCopyHeader, SW_HIDE);

    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(hTreeView, L"Explorer", nullptr);
    SetWindowTheme(hListView, L"Explorer", nullptr);
    SetWindowTheme(hTabCtrl, L"Explorer", nullptr);

    hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(ID_STATUSBAR)), hInst, nullptr);

    ApplyApplicationFonts();
    FontManager::ApplyHeaderViewFont(hRichEdit);
    UpdateControlTooltips();
    AddTooltip(hEditSearch, g_searchTooltip, ID_EDIT_SEARCH);
    AddTooltip(hButtonClearSearch, g_clearTooltip, ID_BUTTON_CLEAR_SEARCH);
    AddTooltip(hButtonSearchHistory, g_historyTooltip, ID_BUTTON_SEARCH_HISTORY);
    SendMessageW(hEditSearch, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(LANG_STR(L"search_placeholder").c_str()));
    LayoutMainWindow(hWnd);
    UpdateStatusBar(LANG_STR(L"status_ready"));
}

void ApplyApplicationFonts() {
    HFONT uiFont = FontManager::GetDefaultFont();
    HWND controls[] = {
        hEditSearch, hButtonClearSearch, hButtonSearch, hButtonSearchHistory,
        hButtonCopyHeader,
        hButtonCancelTask, hTreeView, hTabCtrl, hListView, hStatusBar
    };
    for (HWND control : controls) {
        if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
    }
    FontManager::ApplyHeaderViewFont(hRichEdit);
}

void UpdateControlTooltips() {
    if (!hTooltip) return;
    UpdateTooltipText(hEditSearch, ID_EDIT_SEARCH, g_searchTooltip, _countof(g_searchTooltip), LANG_STR(L"search_tooltip"));
    UpdateTooltipText(hButtonClearSearch, ID_BUTTON_CLEAR_SEARCH, g_clearTooltip, _countof(g_clearTooltip), LANG_STR(L"clear_search"));
    UpdateTooltipText(hButtonSearchHistory, ID_BUTTON_SEARCH_HISTORY, g_historyTooltip, _countof(g_historyTooltip), LANG_STR(L"search_history"));
}

void SetTaskMode(bool active) {
    ShowWindow(hProgressTask, active ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonCancelTask, active ? SW_SHOW : SW_HIDE);
    ShowWindow(hEditSearch, active ? SW_HIDE : SW_SHOW);
    ShowWindow(hButtonSearch, active ? SW_HIDE : SW_SHOW);
    ShowWindow(hButtonSearchHistory, active ? SW_HIDE : SW_SHOW);
    ShowWindow(hButtonClearSearch, active ? SW_HIDE : SW_SHOW);
    if (active) SendMessageW(hProgressTask, PBM_SETPOS, 0, 0);
    LayoutMainWindow(g_hMainWindow);
}

void ClearSearchBox() {
    SendMessageW(hEditSearch, EM_SETSEL, 0, -1);
    SendMessageW(hEditSearch, WM_CLEAR, 0, 0);
    SetWindowTextW(hEditSearch, L"");
    SetDlgItemTextW(g_hMainWindow, ID_EDIT_SEARCH, L"");
    SendMessageW(hEditSearch, EM_SETSEL, 0, 0);
    SearchItems(L"", false);
}

void UpdateSearchClearButton() {
    if (!hButtonClearSearch) return;
    ShowWindow(hButtonClearSearch, IsWindowVisible(hProgressTask) ? SW_HIDE : SW_SHOW);
}

void UpdateStatusBar(const std::wstring& text) {
    if (hStatusBar) SendMessageW(hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
}

void LayoutMainWindow(HWND hWnd) {
    if (!hWnd || !hTreeView || !hTabCtrl || !hStatusBar) return;

    RECT rect{};
    GetClientRect(hWnd, &rect);
    SendMessageW(hStatusBar, WM_SIZE, 0, 0);
    RECT statusRect{};
    GetWindowRect(hStatusBar, &statusRect);
    int statusHeight = statusRect.bottom - statusRect.top;
    int margin = DPIManager::ScaleX(8);
    int gap = DPIManager::ScaleX(kGap);
    int topHeight = DPIManager::ScaleY(kTopHeight);
    int contentBottom = max(topHeight, rect.bottom - statusHeight);

    bool taskActive = IsWindowVisible(hProgressTask);
    int clearWidth = DPIManager::ScaleX(32);
    int searchWidth = DPIManager::ScaleX(82);
    int historyWidth = DPIManager::ScaleX(34);
    int cancelWidth = DPIManager::ScaleX(88);
    int y = DPIManager::ScaleY(6);
    int controlHeight = DPIManager::ScaleY(24);

    if (taskActive) {
        int progressWidth = max(DPIManager::ScaleX(180), rect.right - margin * 2 - cancelWidth - gap);
        SetWindowPos(hProgressTask, nullptr, margin, y, progressWidth, controlHeight, SWP_NOZORDER);
        SetWindowPos(hButtonCancelTask, nullptr, margin + progressWidth + gap, y, cancelWidth, controlHeight, SWP_NOZORDER);
    } else {
        int fixedWidth = clearWidth + searchWidth + historyWidth + gap * 3;
        int editWidth = max(DPIManager::ScaleX(220), rect.right - margin * 2 - fixedWidth);
        int x = margin;
        SetWindowPos(hEditSearch, nullptr, x, y, editWidth, controlHeight, SWP_NOZORDER);
        x += editWidth + gap;
        SetWindowPos(hButtonClearSearch, nullptr, x, y, clearWidth, controlHeight, SWP_NOZORDER);
        x += clearWidth + gap;
        SetWindowPos(hButtonSearch, nullptr, x, y, searchWidth, controlHeight, SWP_NOZORDER);
        x += searchWidth + gap;
        SetWindowPos(hButtonSearchHistory, nullptr, x, y, historyWidth, controlHeight, SWP_NOZORDER);
    }

    ClampSplitter(hWnd);
    int contentHeight = contentBottom - topHeight;
    SetWindowPos(hTreeView, nullptr, 0, topHeight, g_splitterPos, contentHeight, SWP_NOZORDER);
    SetWindowPos(hSplitter, nullptr, g_splitterPos, topHeight, kSplitterWidth, contentHeight, SWP_NOZORDER);
    SetWindowPos(hTabCtrl, nullptr, g_splitterPos + kSplitterWidth, topHeight,
        max(0, rect.right - g_splitterPos - kSplitterWidth), contentHeight, SWP_NOZORDER);

    RECT tabRect{};
    GetClientRect(hTabCtrl, &tabRect);
    TabCtrl_AdjustRect(hTabCtrl, FALSE, &tabRect);
    SetWindowPos(hListView, nullptr, tabRect.left, tabRect.top,
        max(0, tabRect.right - tabRect.left), max(0, tabRect.bottom - tabRect.top), SWP_NOZORDER);
    SetWindowPos(hRichEdit, nullptr, tabRect.left, tabRect.top,
        max(0, tabRect.right - tabRect.left), max(0, tabRect.bottom - tabRect.top), SWP_NOZORDER);
    RECT tabClient{};
    GetClientRect(hTabCtrl, &tabClient);
    POINT tabOrigin{tabClient.left, tabClient.top};
    ClientToScreen(hTabCtrl, &tabOrigin);
    ScreenToClient(hWnd, &tabOrigin);
    const int copyButtonWidth = DPIManager::ScaleX(76);
    SetWindowPos(hButtonCopyHeader, nullptr,
        tabOrigin.x + max(0, tabClient.right - copyButtonWidth - DPIManager::ScaleX(4)),
        tabOrigin.y + DPIManager::ScaleY(3), copyButtonWidth, DPIManager::ScaleY(24), SWP_NOZORDER);

    ResizeListViewColumns();
    UpdateTabViews(false);
}

void UpdateSplitterPosition(HWND hWnd) {
    LayoutMainWindow(hWnd);
}

void UpdateTabViews(bool refreshContent) {
    if (!hTabCtrl || !hListView || !hRichEdit) return;
    int tabIndex = TabCtrl_GetCurSel(hTabCtrl);
    ShowWindow(hListView, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hRichEdit, tabIndex == 0 ? SW_HIDE : SW_SHOW);
    if (hButtonCopyHeader) ShowWindow(hButtonCopyHeader, tabIndex == 0 ? SW_HIDE : SW_SHOW);
    if (!refreshContent) return;

    HTREEITEM selected = TreeView_GetSelection(hTreeView);
    if (!selected || !g_pdbLoaded) return;
    if (tabIndex == 0) PopulateListView(selected);
    else ShowHeaderView(selected);
}

void CopyHeaderText() {
    if (!hRichEdit) return;

    CHARRANGE selection{};
    SendMessageW(hRichEdit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
    const bool hadSelection = selection.cpMin != selection.cpMax;
    if (!hadSelection) {
        SendMessageW(hRichEdit, EM_SETSEL, 0, -1);
    }
    SendMessageW(hRichEdit, WM_COPY, 0, 0);
    if (!hadSelection) {
        SendMessageW(hRichEdit, EM_SETSEL, 0, 0);
    }
}

void CopyFocusedContent() {
    HWND focus = GetFocus();
    if (focus == hRichEdit || focus == hEditSearch) {
        SendMessageW(focus, WM_COPY, 0, 0);
    } else if (focus == hListView) {
        CopySelectedListCell();
    } else if (focus == hTreeView) {
        HTREEITEM selected = TreeView_GetSelection(hTreeView);
        if (!selected) return;
        WCHAR text[4096] = L"";
        TVITEM item{};
        item.mask = TVIF_TEXT;
        item.hItem = selected;
        item.pszText = text;
        item.cchTextMax = _countof(text);
        if (TreeView_GetItem(hTreeView, &item)) {
            std::wstring value = text;
            if (OpenClipboard(g_hMainWindow)) {
                EmptyClipboard();
                HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (value.size() + 1) * sizeof(WCHAR));
                if (memory) {
                    auto data = static_cast<WCHAR*>(GlobalLock(memory));
                    if (data) {
                        wcscpy_s(data, value.size() + 1, value.c_str());
                        GlobalUnlock(memory);
                        SetClipboardData(CF_UNICODETEXT, memory);
                    }
                }
                CloseClipboard();
            }
        }
    }
}
