#pragma once

#include "PDBViewerGlobals.h"

class DragDropManager {
public:
    static void Initialize(HWND hWnd);
    static void Cleanup();
    static void HandleDropFiles(WPARAM wParam, HWND hWnd);

private:
    static HWND s_hWnd;
};
