#pragma once

#include "PDBViewerGlobals.h"

void SearchItems(const std::wstring& text, bool addToHistory = true);
LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
