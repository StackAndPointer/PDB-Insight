#pragma once

#include "PDBViewerGlobals.h"

void CreateControls(HWND hWnd);
void LayoutMainWindow(HWND hWnd);
void UpdateSplitterPosition(HWND hWnd);
void SetTaskMode(bool active);
void ClearSearchBox();
void UpdateSearchClearButton();
void ApplyApplicationFonts();
void UpdateControlTooltips();
void CopyFocusedContent();
void CopyHeaderText();
void UpdateStatusBar(const std::wstring& text);
void ShowRichEditContextMenu(HWND hWnd, int x, int y);
void ShowListViewContextMenu(HWND hWnd, int x, int y);
void UpdateTabViews(bool refreshContent = true);

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
