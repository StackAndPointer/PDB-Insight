#pragma once

#include "resource.h"
#include <windows.h>
#include <string>

extern HWND hTreeView;
extern HWND hTabCtrl;
extern HWND hListView;
extern HWND hRichEdit;
extern HWND hStatusBar;
extern HWND hEditSearch;
extern HWND hButtonSearch;

extern WNDPROC g_pOldEditProc;

LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
