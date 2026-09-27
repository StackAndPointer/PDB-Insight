#include "HeaderViewManager.h"
#include <algorithm>
#include <unordered_set>

std::vector<SyntaxSpan> BuildSyntaxSpans(const std::wstring& sourceText) {
    std::vector<SyntaxSpan> spans;
    if (sourceText.empty()) return spans;

    static const std::unordered_set<std::wstring> keywords = {
        L"class", L"struct", L"union", L"public", L"protected", L"private",
        L"const", L"static", L"virtual", L"inline", L"explicit",
        L"if", L"else", L"for", L"while", L"do", L"switch", L"case", L"default",
        L"break", L"continue", L"return", L"true", L"false", L"NULL", L"nullptr",
        L"typedef", L"enum", L"template", L"typename", L"namespace", L"using",
        L"new", L"delete", L"this", L"friend", L"operator", L"sizeof", L"typeid"
    };

    static const std::unordered_set<std::wstring> basicTypes = {
        L"void", L"int", L"char", L"bool", L"float", L"double", L"long", L"short",
        L"unsigned", L"signed", L"wchar_t", L"char16_t", L"char32_t",
        L"int8_t", L"int16_t", L"int32_t", L"int64_t", L"uint8_t", L"uint16_t",
        L"uint32_t", L"uint64_t", L"size_t", L"ptrdiff_t", L"intptr_t", L"uintptr_t",
        L"DWORD", L"WORD", L"BYTE", L"LONG", L"ULONG", L"BOOL", L"HRESULT",
        L"LPVOID", L"LPCVOID", L"LPSTR", L"LPCSTR", L"LPWSTR", L"LPCWSTR",
        L"HANDLE", L"HWND", L"HINSTANCE", L"HMODULE", L"HDWP", L"HRGN"
    };

    std::unordered_set<std::wstring> typeNames = basicTypes;
    if (g_pdbLoaded) {
        auto addType = [&typeNames](const std::wstring& name) {
            if (name.empty()) return;
            typeNames.insert(name);
            typeNames.insert(PDBHeaderGenerator::FlattenName(name));
            const size_t templatePos = name.find(L'<');
            const std::wstring withoutTemplate = templatePos == std::wstring::npos
                ? name : name.substr(0, templatePos);
            typeNames.insert(withoutTemplate);
            const size_t namespacePos = withoutTemplate.rfind(L"::");
            if (namespacePos != std::wstring::npos) {
                typeNames.insert(withoutTemplate.substr(namespacePos + 2));
            }
            if (templatePos != std::wstring::npos) typeNames.insert(name.substr(0, templatePos));
        };
        for (const auto& cls : g_moduleInfo.classes) addType(cls.name);
        for (const auto& str : g_moduleInfo.structs) addType(str.name);
        for (const auto& uni : g_moduleInfo.unions) addType(uni.name);
        for (const auto& enm : g_moduleInfo.enums) addType(enm.name);
    }

    const auto isIdentStart = [](WCHAR c) {
        return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
    };
    const auto isIdentChar = [&isIdentStart](WCHAR c) {
        return isIdentStart(c) || (c >= L'0' && c <= L'9');
    };

    size_t pos = 0;
    while (pos < sourceText.size()) {
        if (sourceText[pos] == L'/' && pos + 1 < sourceText.size()) {
            size_t end = pos + 2;
            if (sourceText[pos + 1] == L'/') {
                while (end < sourceText.size() && sourceText[end] != L'\r' && sourceText[end] != L'\n') ++end;
                spans.push_back({pos, end, SyntaxKind::Comment});
                pos = end;
                continue;
            }
            if (sourceText[pos + 1] == L'*') {
                while (end + 1 < sourceText.size() &&
                       !(sourceText[end] == L'*' && sourceText[end + 1] == L'/')) {
                    ++end;
                }
                end = (std::min)(end + 2, sourceText.size());
                spans.push_back({pos, end, SyntaxKind::Comment});
                pos = end;
                continue;
            }
        }

        if (sourceText[pos] == L'"') {
            size_t end = pos + 1;
            while (end < sourceText.size()) {
                if (sourceText[end] == L'\\' && end + 1 < sourceText.size()) {
                    end += 2;
                } else if (sourceText[end++] == L'"') {
                    break;
                }
            }
            spans.push_back({pos, end, SyntaxKind::Literal});
            pos = end;
            continue;
        }

        if (sourceText[pos] == L'\'' && pos + 1 < sourceText.size()) {
            size_t end = pos + 1;
            while (end < sourceText.size()) {
                if (sourceText[end] == L'\\' && end + 1 < sourceText.size()) {
                    end += 2;
                } else if (sourceText[end++] == L'\'') {
                    break;
                }
            }
            spans.push_back({pos, end, SyntaxKind::Literal});
            pos = end;
            continue;
        }

        if ((sourceText[pos] >= L'0' && sourceText[pos] <= L'9') ||
            (sourceText[pos] == L'.' && pos + 1 < sourceText.size() &&
             sourceText[pos + 1] >= L'0' && sourceText[pos + 1] <= L'9')) {
            size_t end = pos + 1;
            while (end < sourceText.size()) {
                const WCHAR c = sourceText[end];
                if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'z') ||
                    (c >= L'A' && c <= L'Z') || c == L'.' || c == L'\'' || c == L'x') {
                    ++end;
                } else {
                    break;
                }
            }
            spans.push_back({pos, end, SyntaxKind::Literal});
            pos = end;
            continue;
        }

        if (isIdentStart(sourceText[pos])) {
            size_t end = pos;
            while (end < sourceText.size() && isIdentChar(sourceText[end])) ++end;
            const std::wstring word = sourceText.substr(pos, end - pos);
            SyntaxKind kind = SyntaxKind::Identifier;
            if (keywords.find(word) != keywords.end()) kind = SyntaxKind::Keyword;
            else if (typeNames.find(word) != typeNames.end()) kind = SyntaxKind::Type;

            if (kind != SyntaxKind::Identifier && end < sourceText.size() && sourceText[end] == L'<') {
                size_t templateEnd = end + 1;
                int depth = 1;
                while (templateEnd < sourceText.size() && depth > 0) {
                    if (sourceText[templateEnd] == L'<') ++depth;
                    else if (sourceText[templateEnd] == L'>') --depth;
                    ++templateEnd;
                }
                if (depth == 0) end = templateEnd;
            }

            if (kind != SyntaxKind::Identifier) spans.push_back({pos, end, kind});
            pos = end;
            continue;
        }
        ++pos;
    }
    return spans;
}

static std::wstring ReadRichEditText(HWND hRichEdit) {
    GETTEXTLENGTHEX lengthQuery{};
    lengthQuery.flags = GTL_NUMCHARS;
    lengthQuery.codepage = 1200;
    const LONG length = static_cast<LONG>(SendMessageW(
        hRichEdit, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&lengthQuery), 0));
    if (length <= 0) return std::wstring();

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    GETTEXTEX getText{};
    getText.cb = static_cast<DWORD>((text.size()) * sizeof(WCHAR));
    getText.flags = GT_DEFAULT;
    getText.codepage = 1200;
    const LONG copied = static_cast<LONG>(SendMessageW(
        hRichEdit, EM_GETTEXTEX, reinterpret_cast<WPARAM>(&getText), reinterpret_cast<LPARAM>(text.data())));
    if (copied <= 0) return std::wstring();
    text.resize(static_cast<size_t>(copied));
    return text;
}

void ApplySyntaxHighlighting(HWND hRichEdit, const std::wstring& text,
                             const std::vector<SyntaxSpan>& spans) {
    if (!hRichEdit || text.empty()) return;

    const std::wstring actualText = ReadRichEditText(hRichEdit);
    if (actualText.empty()) return;
    const bool textMatchesControl = actualText == text;
    const std::vector<SyntaxSpan> actualSpans = textMatchesControl
        ? spans : BuildSyntaxSpans(actualText);

    const COLORREF COLOR_KEYWORD = RGB(0, 0, 255);
    const COLORREF COLOR_COMMENT = RGB(0, 128, 0);
    const COLORREF COLOR_STRING = RGB(163, 21, 21);
    const COLORREF COLOR_TYPE = RGB(43, 145, 175);
    const COLORREF COLOR_DEFAULT = RGB(0, 0, 0);
    CHARRANGE previousSelection{};
    SendMessageW(hRichEdit, EM_EXGETSEL, 0,
                 reinterpret_cast<LPARAM>(&previousSelection));

    SendMessageW(hRichEdit, WM_SETREDRAW, FALSE, 0);
    CHARFORMAT2 cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = COLOR_DEFAULT;
    cf.dwEffects = 0;
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));

    for (const auto& span : actualSpans) {
        if (span.begin >= span.end || span.end > actualText.size()) continue;
        COLORREF color = COLOR_DEFAULT;
        switch (span.kind) {
        case SyntaxKind::Keyword: color = COLOR_KEYWORD; break;
        case SyntaxKind::Type: color = COLOR_TYPE; break;
        case SyntaxKind::Comment: color = COLOR_COMMENT; break;
        case SyntaxKind::Literal: color = COLOR_STRING; break;
        default: break;
        }
        SetRichEditRangeColor(hRichEdit, static_cast<LONG>(span.begin),
                              static_cast<LONG>(span.end), color);
    }

    const LONG textLength = static_cast<LONG>(actualText.size());
    previousSelection.cpMin = max(0L, min(previousSelection.cpMin, textLength));
    previousSelection.cpMax = max(previousSelection.cpMin, min(previousSelection.cpMax, textLength));
    SendMessageW(hRichEdit, EM_EXSETSEL, 0,
                 reinterpret_cast<LPARAM>(&previousSelection));
    SendMessageW(hRichEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hRichEdit, nullptr, TRUE);
}
