#pragma once

#include "PDBViewerGlobals.h"

class FontManager {
public:
    static void Initialize();
    static void Cleanup();
    static void UpdateDPI(int dpi);

    static HFONT GetHeaderViewFont();
    static HFONT GetCodeFont();
    static HFONT GetDefaultFont();

    static int GetDPI();
    static int ScaleForDPI(int value);

    static void ApplyHeaderViewFont(HWND hRichEdit);

private:
    static HFONT CreateUiFont();
    static HFONT CreateCodeFont();
    static HFONT s_codeFont;
    static HFONT s_defaultFont;
    static int s_dpi;
};
