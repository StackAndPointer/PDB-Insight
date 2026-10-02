#include "FontManager.h"
#include <set>

HFONT FontManager::s_codeFont = nullptr;
HFONT FontManager::s_defaultFont = nullptr;
int FontManager::s_dpi = 96;
std::wstring FontManager::s_uiFontName;
std::wstring FontManager::s_codeFontName;

namespace {
constexpr int kFontSizePoints = 10;

bool IsFontNameUsable(const std::wstring& faceName) {
    if (faceName.empty()) return false;
    HDC hdc = GetDC(nullptr);
    if (!hdc) return false;
    bool found = false;
    LOGFONTW filter = {};
    filter.lfCharSet = DEFAULT_CHARSET;
    wcsncpy_s(filter.lfFaceName, faceName.c_str(), _TRUNCATE);
    EnumFontFamiliesExW(hdc, &filter,
        [](const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM lParam) -> int {
            *reinterpret_cast<bool*>(lParam) = true;
            return 0;
        }, reinterpret_cast<LPARAM>(&found), 0);
    ReleaseDC(nullptr, hdc);
    return found;
}

HFONT CreateFontByName(const std::wstring& faceName, DWORD pitchAndFamily,
                       const std::wstring& fallbackFaceName, int fallbackPitch,
                       int fallbackFamily) {
    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(kFontSizePoints, FontManager::GetDPI(), 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = static_cast<BYTE>(pitchAndFamily);
    if (!faceName.empty()) {
        wcsncpy_s(lf.lfFaceName, faceName.c_str(), _TRUNCATE);
    } else {
        wcsncpy_s(lf.lfFaceName, fallbackFaceName.c_str(), _TRUNCATE);
        lf.lfPitchAndFamily = static_cast<BYTE>(fallbackPitch | fallbackFamily);
    }
    return CreateFontIndirectW(&lf);
}
}  // namespace

std::vector<std::wstring> FontManager::EnumerateSystemFonts() {
    std::set<std::wstring> uniqueNames;
    HDC hdc = GetDC(nullptr);
    if (!hdc) return {};

    LOGFONTW filter = {};
    filter.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExW(hdc, &filter,
        [](const LOGFONTW* logFont, const TEXTMETRICW*, DWORD, LPARAM lParam) -> int {
            auto* names = reinterpret_cast<std::set<std::wstring>*>(lParam);
            if (logFont && logFont->lfFaceName[0] != L'@') {
                names->insert(logFont->lfFaceName);
            }
            return 1;
        }, reinterpret_cast<LPARAM>(&uniqueNames), 0);
    ReleaseDC(nullptr, hdc);

    return std::vector<std::wstring>(uniqueNames.begin(), uniqueNames.end());
}

HFONT FontManager::CreateUiFont() {
    if (!s_uiFontName.empty()) {
        HFONT font = CreateFontByName(s_uiFontName, DEFAULT_PITCH | FF_DONTCARE,
                                      L"Segoe UI", DEFAULT_PITCH, FF_DONTCARE);
        if (font) return font;
    }

    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        ncm.lfMessageFont.lfHeight = -MulDiv(kFontSizePoints, s_dpi, 72);
        return CreateFontIndirectW(&ncm.lfMessageFont);
    }

    return CreateFontByName({}, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI",
                            DEFAULT_PITCH, FF_DONTCARE);
}

HFONT FontManager::CreateCodeFont() {
    if (!s_codeFontName.empty()) {
        HFONT font = CreateFontByName(s_codeFontName, FIXED_PITCH | FF_MODERN,
                                      L"Consolas", FIXED_PITCH, FF_MODERN);
        if (font) return font;
    }

    const wchar_t* faces[] = { L"Cascadia Mono", L"Consolas", L"Courier New" };
    for (const wchar_t* face : faces) {
        HFONT font = CreateFontByName(face, FIXED_PITCH | FF_MODERN,
                                      L"Consolas", FIXED_PITCH, FF_MODERN);
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

void FontManager::ApplyFontSettings(const std::wstring& uiFontName,
                                    const std::wstring& codeFontName) {
    s_uiFontName = IsFontNameUsable(uiFontName) ? uiFontName : std::wstring();
    s_codeFontName = IsFontNameUsable(codeFontName) ? codeFontName : std::wstring();
    Cleanup();
    s_defaultFont = CreateUiFont();
    s_codeFont = CreateCodeFont();
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
