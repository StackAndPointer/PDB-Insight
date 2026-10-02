#pragma once

#include "PDBViewerGlobals.h"
#include "TreeNodeHelper.h"

void PopulateTreeView();
void ExpandLazyTreeItem(HTREEITEM item);
void AutoExtendTreeCategory();
void ResetPagedTreeCategory();
HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, LPARAM lParam);
std::wstring GetSearchResultItemText(TreeCategory category, size_t index);
LPARAM GetSearchResultItemParam(TreeCategory category, size_t index);
void OpenPDBFile(HWND hWnd);
void OpenDllFile(HWND hWnd);
void ClosePDBFile(HWND hWnd);
void RefreshCurrentSelection();
