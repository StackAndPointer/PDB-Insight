#include "FontManager.h"

HFONT FontManager::s_headerViewFont = nullptr;
HFONT FontManager::s_defaultFont = nullptr;
int FontManager::s_dpi = 96;

void FontManager::Initialize() {
    HDC hDC = GetDC(nullptr);
    if (hDC) {
        s_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(nullptr, hDC);
    }
    
    int fontSize = 10;
    if (s_dpi >= 144) {
        fontSize = 12;
    } else if (s_dpi >= 120) {
        fontSize = 11;
    }
    
    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(fontSize, s_dpi, 72);
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    wcscpy_s(lf.lfFaceName, L"SimSun");
    lf.lfCharSet = GB2312_CHARSET;
    s_headerViewFont = CreateFontIndirectW(&lf);
    
    if (!s_headerViewFont) {
        wcscpy_s(lf.lfFaceName, L"宋体");
        lf.lfCharSet = GB2312_CHARSET;
        s_headerViewFont = CreateFontIndirectW(&lf);
        
        if (!s_headerViewFont) {
            wcscpy_s(lf.lfFaceName, L"Consolas");
            lf.lfCharSet = DEFAULT_CHARSET;
            s_headerViewFont = CreateFontIndirectW(&lf);
            
            if (!s_headerViewFont) {
                wcscpy_s(lf.lfFaceName, L"Courier New");
                lf.lfCharSet = DEFAULT_CHARSET;
                s_headerViewFont = CreateFontIndirectW(&lf);
                
                if (!s_headerViewFont) {
                    NONCLIENTMETRICSW ncm = {};
                    ncm.cbSize = sizeof(ncm);
                    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
                    s_headerViewFont = CreateFontIndirectW(&ncm.lfMessageFont);
                }
            }
        }
    }
    
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    s_defaultFont = CreateFontIndirectW(&ncm.lfMessageFont);
}

void FontManager::Cleanup() {
    if (s_headerViewFont) {
        DeleteObject(s_headerViewFont);
        s_headerViewFont = nullptr;
    }
    if (s_defaultFont) {
        DeleteObject(s_defaultFont);
        s_defaultFont = nullptr;
    }
}

HFONT FontManager::GetHeaderViewFont() {
    return s_headerViewFont;
}

HFONT FontManager::GetDefaultFont() {
    return s_defaultFont;
}

int FontManager::GetDPI() {
    return s_dpi;
}

int FontManager::ScaleForDPI(int value) {
    return MulDiv(value, s_dpi, 96);
}

void FontManager::ApplyHeaderViewFont(HWND hRichEdit) {
    if (hRichEdit && s_headerViewFont) {
        SendMessageW(hRichEdit, WM_SETFONT, (WPARAM)s_headerViewFont, TRUE);
        InvalidateRect(hRichEdit, nullptr, TRUE);
    }
}
