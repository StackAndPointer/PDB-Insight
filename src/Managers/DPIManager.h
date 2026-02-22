#pragma once

#include "PDBViewerGlobals.h"

class DPIManager {
public:
    static void Initialize();
    static void Cleanup();
    
    static int GetDPI();
    static int ScaleX(int value);
    static int ScaleY(int value);
    static int UnscaleX(int value);
    static int UnscaleY(int value);
    
    static void SetProcessDPIAware();
    
    static void AdjustWindowRectForDPI(LPRECT rect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
    
private:
    static int s_dpi;
    static float s_scaleX;
    static float s_scaleY;
};
