#pragma once

#include "PDBViewerGlobals.h"

void ShowHeaderView(HTREEITEM hItem);
void ApplySyntaxHighlighting(HWND hRichEdit);
void SetRichEditRangeColor(HWND hRichEdit, LONG start, LONG end, COLORREF color);
void SetRichEditRangeBold(HWND hRichEdit, LONG start, LONG end);
