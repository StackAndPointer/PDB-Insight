#pragma once

#include "PDBViewerGlobals.h"
#include <string>
#include <vector>

class FontManager {
public:
    static void Initialize();
    static void Cleanup();
    static void UpdateDPI(int dpi);
    static void ApplyFontSettings(const std::wstring& uiFontName,
                                  const std::wstring& codeFontName);

    static const std::vector<std::wstring>& GetSystemFonts();

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
    static std::wstring s_uiFontName;
    static std::wstring s_codeFontName;
};
