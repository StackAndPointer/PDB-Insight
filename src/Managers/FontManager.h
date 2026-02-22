#pragma once

#include "PDBViewerGlobals.h"

class FontManager {
public:
    static void Initialize();
    static void Cleanup();
    
    static HFONT GetHeaderViewFont();
    static HFONT GetDefaultFont();
    
    static int GetDPI();
    static int ScaleForDPI(int value);
    
    static void ApplyHeaderViewFont(HWND hRichEdit);
    
private:
    static HFONT s_headerViewFont;
    static HFONT s_defaultFont;
    static int s_dpi;
};
