#pragma once

#include "PDBViewerGlobals.h"

void ShowOffsetModeMenu(HWND hWnd, int x, int y);
void ShowSearchHistoryMenu(HWND hWnd, int x, int y);
void RebuildMenu(HWND hWnd);
void RefreshLanguage(HWND hWnd);
void AssociatePDBFiles(HWND hWnd);
void UnassociatePDBFiles(HWND hWnd);
void AutoAssociatePDBFiles();
