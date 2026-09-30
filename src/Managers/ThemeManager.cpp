#include "ThemeManager.h"
#include "ControlsManager.h"
#include "HeaderViewManager.h"
#include "FontManager.h"
#include <uxtheme.h>

#pragma comment(lib, "uxtheme.lib")

namespace {
const ThemePalette kLightPalette{};

const ThemePalette kDarkPalette = [] {
    ThemePalette palette;
    palette.window = RGB(30, 30, 30);
    palette.surface = RGB(37, 37, 38);
    palette.border = RGB(62, 62, 66);
    palette.text = RGB(226, 226, 226);
    palette.mutedText = RGB(160, 160, 160);
    palette.keyword = RGB(86, 156, 214);
    palette.type = RGB(78, 201, 176);
    palette.function = RGB(220, 220, 170);
    palette.qualified = RGB(156, 220, 254);
    palette.comment = RGB(106, 153, 85);
    palette.literal = RGB(206, 145, 120);
    palette.defaultSyntax = RGB(220, 220, 220);
    return palette;
}();
}  // namespace

ThemeManager& ThemeManager::GetInstance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::SetMode(ThemeMode mode) {
    m_mode = mode;
}

const ThemePalette& ThemeManager::GetPalette() const {
    return m_mode == ThemeMode::Dark ? kDarkPalette : kLightPalette;
}

void ThemeManager::Apply(HWND hWnd) {
    const ThemePalette& palette = GetPalette();
    const bool dark = m_mode == ThemeMode::Dark;

    SetWindowTheme(hTreeView, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    SetWindowTheme(hListView, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    SetWindowTheme(hTabCtrl, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    SetWindowTheme(hEditSearch, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);

    if (hTreeView) {
        TreeView_SetBkColor(hTreeView, palette.surface);
        TreeView_SetTextColor(hTreeView, palette.text);
        TreeView_SetLineColor(hTreeView, palette.border);
    }
    if (hListView) {
        ListView_SetBkColor(hListView, palette.surface);
        ListView_SetTextBkColor(hListView, palette.surface);
        ListView_SetTextColor(hListView, palette.text);
    }
    if (hRichEdit) {
        SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, palette.surface);
        ApplySyntaxHighlighting(hRichEdit);
    }

    HBRUSH background = CreateSolidBrush(palette.window);
    if (background) {
        SetClassLongPtrW(hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(background));
    }
    InvalidateRect(hWnd, nullptr, TRUE);
    DrawMenuBar(hWnd);
}
