#pragma once

#include "PDBViewerGlobals.h"

void PopulateTreeView();
HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, LPARAM lParam);
void OpenPDBFile(HWND hWnd);
void ClosePDBFile(HWND hWnd);
void RefreshCurrentSelection();
