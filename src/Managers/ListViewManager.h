#pragma once

#include "PDBViewerGlobals.h"

void PopulateListView(HTREEITEM hItem);
void HandleListViewGetDispInfo(NMLVDISPINFOW* info);
void AddListViewColumn(int index, const std::wstring& text, int width);
void ResizeListViewColumns();
