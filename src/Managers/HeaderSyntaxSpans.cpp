#include "HeaderViewManager.h"
#include <algorithm>
#include <unordered_set>

namespace {

bool IsIdentStart(WCHAR c) {
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
}

bool IsIdentChar(WCHAR c) {
    return IsIdentStart(c) || (c >= L'0' && c <= L'9');
}

bool IsDigit(WCHAR c) {
    return c >= L'0' && c <= L'9';
}

bool IsHexDigit(WCHAR c) {
    return IsDigit(c) || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

bool IsBinaryDigit(WCHAR c) {
    return c == L'0' || c == L'1';
}

bool IsNumericSuffixChar(WCHAR c) {
    switch (c) {
    case L'u': case L'U':
    case L'l': case L'L':
    case L'f': case L'F':
    case L'z': case L'Z':
        return true;
    default:
        return false;
    }
}

size_t ConsumeDigits(const std::wstring& text, size_t pos,
                     bool (*predicate)(WCHAR)) {
    while (pos < text.size() && predicate(text[pos])) ++pos;
    return pos;
}

size_t ConsumeNumericSuffix(const std::wstring& text, size_t pos) {
    while (pos < text.size() && IsNumericSuffixChar(text[pos])) ++pos;
    return pos;
}

size_t ConsumeExponent(const std::wstring& text, size_t pos) {
    if (pos >= text.size() || (text[pos] != L'e' && text[pos] != L'E')) {
        return pos;
    }
    ++pos;
    if (pos < text.size() && (text[pos] == L'+' || text[pos] == L'-')) ++pos;
    if (pos >= text.size() || !IsDigit(text[pos])) return pos;
    return ConsumeDigits(text, pos, IsDigit);
}

size_t ConsumeNumericLiteral(const std::wstring& text, size_t pos) {
    const bool startsWithDot = text[pos] == L'.';
    if (startsWithDot) {
        pos = ConsumeDigits(text, pos + 1, IsDigit);
    } else if (pos + 1 < text.size() && text[pos] == L'0' &&
               (text[pos + 1] == L'x' || text[pos + 1] == L'X')) {
        const size_t digitsBegin = pos + 2;
        const size_t end = ConsumeDigits(text, digitsBegin, IsHexDigit);
        if (end == digitsBegin) return end;
        return ConsumeNumericSuffix(text, end);
    } else if (pos + 1 < text.size() && text[pos] == L'0' &&
               (text[pos + 1] == L'b' || text[pos + 1] == L'B')) {
        const size_t digitsBegin = pos + 2;
        const size_t end = ConsumeDigits(text, digitsBegin, IsBinaryDigit);
        if (end == digitsBegin) return end;
        return ConsumeNumericSuffix(text, end);
    } else {
        pos = ConsumeDigits(text, pos, IsDigit);
    }

    if (pos < text.size() && text[pos] == L'.') {
        pos = ConsumeDigits(text, pos + 1, IsDigit);
    }
    const size_t exponentEnd = ConsumeExponent(text, pos);
    if (exponentEnd == pos && (startsWithDot || (pos > 0 && text[pos - 1] == L'.'))) {
        return pos;
    }
    pos = exponentEnd;
    pos = ConsumeNumericSuffix(text, pos);
    return pos;
}

bool IsNumericLiteralBoundary(const std::wstring& text, size_t pos) {
    return pos >= text.size() || (!IsIdentChar(text[pos]) && text[pos] != L'.');
}

struct LexedIdentifier {
    size_t begin = 0;
    size_t end = 0;
    std::wstring text;
};

std::vector<LexedIdentifier> LexIdentifiers(const std::wstring& sourceText,
                                            const std::vector<SyntaxSpan>& ignored) {
    std::vector<LexedIdentifier> identifiers;
    size_t ignoredIndex = 0;
    size_t pos = 0;
    while (pos < sourceText.size()) {
        while (ignoredIndex < ignored.size() && ignored[ignoredIndex].end <= pos) {
            ++ignoredIndex;
        }
        if (ignoredIndex < ignored.size() && ignored[ignoredIndex].begin <= pos) {
            pos = ignored[ignoredIndex].end;
            continue;
        }
        if (!IsIdentStart(sourceText[pos])) {
            ++pos;
            continue;
        }
        size_t end = pos;
        while (end < sourceText.size() && IsIdentChar(sourceText[end])) ++end;
        identifiers.push_back({pos, end, sourceText.substr(pos, end - pos)});
        pos = end;
    }
    return identifiers;
}

size_t SkipWhitespace(const std::wstring& text, size_t pos) {
    while (pos < text.size() && (text[pos] == L' ' || text[pos] == L'\t' ||
                                 text[pos] == L'\r' || text[pos] == L'\n')) {
        ++pos;
    }
    return pos;
}

bool IsQualifiedSeparator(const std::wstring& text, size_t pos) {
    return pos + 1 < text.size() && text[pos] == L':' && text[pos + 1] == L':';
}

bool IsFunctionSyntaxCandidate(const std::wstring& sourceText,
                               const std::vector<LexedIdentifier>& identifiers,
                               size_t index,
                               const std::unordered_set<std::wstring>& keywords) {
    const auto& identifier = identifiers[index];
    if (identifier.text.empty() || keywords.find(identifier.text) != keywords.end()) {
        return false;
    }
    const size_t after = SkipWhitespace(sourceText, identifier.end);
    if (after >= sourceText.size() || sourceText[after] != L'(') return false;
    return true;
}

std::vector<SyntaxKind> ClassifyIdentifiers(
    const std::wstring& sourceText,
    const std::vector<LexedIdentifier>& identifiers,
    const std::unordered_set<std::wstring>& keywords,
    const std::unordered_set<std::wstring>& typeNames) {
    std::vector<SyntaxKind> kinds(identifiers.size(), SyntaxKind::Identifier);

    for (size_t i = 0; i < identifiers.size(); ++i) {
        if (keywords.find(identifiers[i].text) != keywords.end()) {
            kinds[i] = SyntaxKind::Keyword;
        } else if (typeNames.find(identifiers[i].text) != typeNames.end()) {
            kinds[i] = SyntaxKind::Type;
        }
    }

    for (size_t i = 0; i + 1 < identifiers.size(); ++i) {
        if (identifiers[i].end + 2 != identifiers[i + 1].begin ||
            !IsQualifiedSeparator(sourceText, identifiers[i].end)) {
            continue;
        }
        size_t chainEnd = i;
        while (chainEnd + 1 < identifiers.size() &&
               identifiers[chainEnd].end + 2 == identifiers[chainEnd + 1].begin &&
               IsQualifiedSeparator(sourceText, identifiers[chainEnd].end)) {
            ++chainEnd;
        }
        if (kinds[chainEnd] == SyntaxKind::Type) {
            for (size_t j = i; j < chainEnd; ++j) {
                if (kinds[j] == SyntaxKind::Identifier) {
                    kinds[j] = SyntaxKind::QualifiedIdentifier;
                }
            }
            i = chainEnd;
            continue;
        }
        for (size_t j = i; j <= chainEnd; ++j) {
            if (kinds[j] == SyntaxKind::Identifier) {
                kinds[j] = SyntaxKind::QualifiedIdentifier;
            }
        }
        i = chainEnd;
    }
    for (size_t i = 0; i < identifiers.size(); ++i) {
        const bool nameCandidate = kinds[i] == SyntaxKind::Identifier ||
            kinds[i] == SyntaxKind::QualifiedIdentifier;
        if (nameCandidate &&
            IsFunctionSyntaxCandidate(sourceText, identifiers, i, keywords)) {
            kinds[i] = SyntaxKind::Function;
        }
    }
    return kinds;
}

}  // namespace

std::vector<SyntaxSpan> BuildSyntaxSpans(const std::wstring& sourceText) {
    std::vector<SyntaxSpan> spans;
    if (sourceText.empty()) return spans;

    static const std::unordered_set<std::wstring> keywords = {
        L"class", L"struct", L"union", L"public", L"protected", L"private",
        L"const", L"static", L"virtual", L"inline", L"explicit",
        L"if", L"else", L"for", L"while", L"do", L"switch", L"case", L"default",
        L"break", L"continue", L"return", L"true", L"false", L"NULL", L"nullptr",
        L"typedef", L"enum", L"template", L"typename", L"namespace", L"using",
        L"new", L"delete", L"this", L"friend", L"operator", L"sizeof", L"typeid",
        L"__cdecl", L"__stdcall", L"__fastcall", L"__vectorcall", L"__thiscall"
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

    const auto isIdentStart = IsIdentStart;
    const auto isIdentChar = IsIdentChar;

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

        if ((IsDigit(sourceText[pos])) ||
            (sourceText[pos] == L'.' && pos + 1 < sourceText.size() &&
             IsDigit(sourceText[pos + 1]))) {
            size_t end = ConsumeNumericLiteral(sourceText, pos);
            if (!IsNumericLiteralBoundary(sourceText, end)) {
                ++pos;
                continue;
            }
            spans.push_back({pos, end, SyntaxKind::Literal});
            pos = end;
            continue;
        }

        ++pos;
    }

    std::vector<SyntaxSpan> literalSpans = spans;
    const std::vector<LexedIdentifier> identifiers = LexIdentifiers(sourceText, spans);
    for (auto it = literalSpans.begin(); it != literalSpans.end();) {
        const size_t begin = it->begin;
        const size_t end = it->end;
        const bool embedded = std::find_if(identifiers.begin(), identifiers.end(),
            [begin, end](const LexedIdentifier& identifier) {
                return identifier.begin < end && begin < identifier.end;
            }) != identifiers.end();
        if (!embedded) {
            ++it;
            continue;
        }
        it = literalSpans.erase(it);
    }
    spans.swap(literalSpans);
    const std::vector<SyntaxKind> kinds = ClassifyIdentifiers(
        sourceText, identifiers, keywords, typeNames);
    for (size_t i = 0; i < identifiers.size(); ++i) {
        if (kinds[i] == SyntaxKind::Identifier) continue;
        size_t end = identifiers[i].end;
        if ((kinds[i] == SyntaxKind::Type || kinds[i] == SyntaxKind::Keyword) &&
            end < sourceText.size() && sourceText[end] == L'<') {
            size_t templateEnd = end + 1;
            int depth = 1;
            while (templateEnd < sourceText.size() && depth > 0) {
                if (sourceText[templateEnd] == L'<') ++depth;
                else if (sourceText[templateEnd] == L'>') --depth;
                ++templateEnd;
            }
            if (depth == 0) end = templateEnd;
        }
        spans.push_back({identifiers[i].begin, end, kinds[i]});
    }
    std::sort(spans.begin(), spans.end(),
              [](const SyntaxSpan& lhs, const SyntaxSpan& rhs) {
                  if (lhs.begin != rhs.begin) return lhs.begin < rhs.begin;
                  return lhs.end < rhs.end;
              });
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

void SetRichEditRangeColor(HWND hRichEdit, LONG start, LONG end, COLORREF color) {
    CHARFORMAT2 cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    CHARRANGE range{start, end};
    SendMessageW(hRichEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION,
                 reinterpret_cast<LPARAM>(&cf));
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
    const COLORREF COLOR_FUNCTION = RGB(121, 94, 38);
    const COLORREF COLOR_QUALIFIED = RGB(38, 127, 153);
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
        case SyntaxKind::Function: color = COLOR_FUNCTION; break;
        case SyntaxKind::QualifiedIdentifier: color = COLOR_QUALIFIED; break;
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
