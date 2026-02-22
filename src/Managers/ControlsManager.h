#pragma once

#include "PDBViewerGlobals.h"

void CreateControls(HWND hWnd);
void UpdateStatusBar(const std::wstring& text);
void UpdateTabViews();
void UpdateSplitterPosition(HWND hWnd);
void ShowRichEditContextMenu(HWND hWnd, int x, int y);
void ShowListViewContextMenu(HWND hWnd, int x, int y);
LRESULT CALLBACK RichEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
