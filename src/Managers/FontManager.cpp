#include "FontManager.h"

HFONT FontManager::s_codeFont = nullptr;
HFONT FontManager::s_defaultFont = nullptr;
int FontManager::s_dpi = 96;

HFONT FontManager::CreateUiFont() {
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        ncm.lfMessageFont.lfHeight = -MulDiv(10, s_dpi, 72);
        return CreateFontIndirectW(&ncm.lfMessageFont);
    }

    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(10, s_dpi, 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    return CreateFontIndirectW(&lf);
}

HFONT FontManager::CreateCodeFont() {
    const wchar_t* faces[] = { L"Cascadia Mono", L"Consolas", L"Courier New" };
    for (const wchar_t* face : faces) {
        LOGFONTW lf = {};
        lf.lfHeight = -MulDiv(10, s_dpi, 72);
        lf.lfWeight = FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        wcscpy_s(lf.lfFaceName, face);
        HFONT font = CreateFontIndirectW(&lf);
        if (font) return font;
    }
    return CreateUiFont();
}

void FontManager::Initialize() {
    HDC hDC = GetDC(nullptr);
    if (hDC) {
        s_dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(nullptr, hDC);
    }
    s_defaultFont = CreateUiFont();
    s_codeFont = CreateCodeFont();
}

void FontManager::Cleanup() {
    if (s_codeFont) {
        DeleteObject(s_codeFont);
        s_codeFont = nullptr;
    }
    if (s_defaultFont) {
        DeleteObject(s_defaultFont);
        s_defaultFont = nullptr;
    }
}

void FontManager::UpdateDPI(int dpi) {
    if (dpi <= 0 || dpi == s_dpi) return;
    s_dpi = dpi;
    Cleanup();
    s_defaultFont = CreateUiFont();
    s_codeFont = CreateCodeFont();
}

HFONT FontManager::GetHeaderViewFont() {
    return s_codeFont;
}

HFONT FontManager::GetCodeFont() {
    return s_codeFont;
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
    if (hRichEdit && s_codeFont) {
        SendMessageW(hRichEdit, WM_SETFONT, reinterpret_cast<WPARAM>(s_codeFont), TRUE);
        InvalidateRect(hRichEdit, nullptr, TRUE);
    }
}
