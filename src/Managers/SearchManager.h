#pragma once

#include "PDBViewerGlobals.h"

constexpr UINT WM_APP_UPDATE_SEARCH_UI = WM_APP + 42;

void SearchItems(const std::wstring& text, bool addToHistory = true);
LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
