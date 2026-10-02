#pragma once

#include "PDBViewerGlobals.h"

constexpr UINT WM_APP_UPDATE_SEARCH_UI = WM_APP + 42;

constexpr LPARAM kSearchResultNodeParamBase = 70000;

void SearchItems(const std::wstring& text, bool addToHistory = true);
bool HandleSearchTreeNotification(const NMTREEVIEW* notification);
bool HandleSearchTreeSelection(HTREEITEM item);
void AutoLoadMoreSearchResults();
LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
