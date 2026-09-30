#pragma once

#include "PDBViewerGlobals.h"

enum class ThemeMode {
    Light,
    Dark
};

struct ThemePalette {
    COLORREF window = RGB(0, 0, 0);
    COLORREF surface = RGB(0, 0, 0);
    COLORREF border = RGB(0, 0, 0);
    COLORREF text = RGB(0, 0, 0);
    COLORREF mutedText = RGB(0, 0, 0);
    COLORREF keyword = RGB(0, 0, 0);
    COLORREF type = RGB(0, 0, 0);
    COLORREF function = RGB(0, 0, 0);
    COLORREF qualified = RGB(0, 0, 0);
    COLORREF comment = RGB(0, 0, 0);
    COLORREF literal = RGB(0, 0, 0);
    COLORREF defaultSyntax = RGB(0, 0, 0);
};

class ThemeManager {
public:
    static ThemeManager& GetInstance();

    ThemeMode GetMode() const { return m_mode; }
    void SetMode(ThemeMode mode);
    void Apply(HWND hWnd);

    const ThemePalette& GetPalette() const;

private:
    ThemeManager() = default;

    ThemeMode m_mode = ThemeMode::Light;
};
