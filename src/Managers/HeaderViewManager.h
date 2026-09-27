#pragma once

#include "PDBViewerGlobals.h"
#include <vector>

void ShowHeaderView(HTREEITEM hItem);
std::vector<SyntaxSpan> BuildSyntaxSpans(const std::wstring& sourceText);
void ApplySyntaxHighlighting(HWND hRichEdit);
void ApplySyntaxHighlighting(HWND hRichEdit, const std::wstring& text);
void ApplySyntaxHighlighting(HWND hRichEdit, const std::wstring& text,
                            const std::vector<SyntaxSpan>& spans);
void SetRichEditRangeColor(HWND hRichEdit, LONG start, LONG end, COLORREF color);
void SetRichEditRangeBold(HWND hRichEdit, LONG start, LONG end);
