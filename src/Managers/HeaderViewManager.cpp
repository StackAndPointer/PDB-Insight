#include "HeaderViewManager.h"
#include "FontManager.h"
#include "TreeNodeHelper.h"
#include <set>
#include <unordered_set>

void ShowHeaderView(HTREEITEM hItem)
{
    if (!hRichEdit || !hItem || !g_pdbLoaded) return;
    
    std::wstring content;
    TVITEM tvi;
    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(hTreeView, &tvi);
    DWORD param = (DWORD)tvi.lParam;

    if (param == 0)
    {
        std::wostringstream ss;
        ss << L"// Module Information\r\n\r\n";
        ss << L"// PDB File: " << g_moduleInfo.pdbFileName << L"\r\n";
        if (!g_moduleInfo.name.empty())
        {
            ss << L"// Module Name: " << g_moduleInfo.name << L"\r\n";
        }
        ss << L"\r\n// Statistics:\r\n";
        ss << L"// Functions: " << g_moduleInfo.functions.size() << L"\r\n";
        ss << L"// Classes: " << g_moduleInfo.classes.size() << L"\r\n";
        ss << L"// Structs: " << g_moduleInfo.structs.size() << L"\r\n";
        ss << L"// Unions: " << g_moduleInfo.unions.size() << L"\r\n";
        ss << L"// Enums: " << g_moduleInfo.enums.size() << L"\r\n";
        ss << L"// Global Variables: " << g_moduleInfo.globalVariables.size() << L"\r\n";
        content = ss.str();
    }
    else if (param == 1)
    {
        std::wostringstream ss;
        ss << L"// Functions List\r\n\r\n";
        ss << L"// Total Functions: " << g_moduleInfo.functions.size() << L"\r\n\r\n";
        
        size_t staticCount = 0, virtualCount = 0, memberCount = 0;
        for (const auto& func : g_moduleInfo.functions)
        {
            if (func.isStatic) staticCount++;
            if (func.isVirtual) virtualCount++;
            if (func.isMemberFunction) memberCount++;
        }
        ss << L"// Static Functions: " << staticCount << L"\r\n";
        ss << L"// Virtual Functions: " << virtualCount << L"\r\n";
        ss << L"// Member Functions: " << memberCount << L"\r\n";
        ss << L"// Global Functions: " << (g_moduleInfo.functions.size() - memberCount) << L"\r\n";
        content = ss.str();
    }
    else if (param == 2)
    {
        std::wostringstream ss;
        ss << L"// Classes List\r\n\r\n";
        ss << L"// Total Classes: " << g_moduleInfo.classes.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0, totalVirtuals = 0;
        for (const auto& cls : g_moduleInfo.classes)
        {
            totalMembers += cls.members.size();
            totalVirtuals += cls.virtualFunctions.size();
        }
        ss << L"// Total Members: " << totalMembers << L"\r\n";
        ss << L"// Total Virtual Functions: " << totalVirtuals << L"\r\n";
        content = ss.str();
    }
    else if (param == 3)
    {
        std::wostringstream ss;
        ss << L"// Structs List\r\n\r\n";
        ss << L"// Total Structs: " << g_moduleInfo.structs.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0;
        size_t totalSize = 0;
        for (const auto& str : g_moduleInfo.structs)
        {
            totalMembers += str.members.size();
            totalSize += str.size;
        }
        ss << L"// Total Members: " << totalMembers << L"\r\n";
        ss << L"// Total Size: " << totalSize << L" bytes\r\n";
        content = ss.str();
    }
    else if (param == 4)
    {
        std::wostringstream ss;
        ss << L"// Unions List\r\n\r\n";
        ss << L"// Total Unions: " << g_moduleInfo.unions.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0;
        size_t totalSize = 0;
        for (const auto& uni : g_moduleInfo.unions)
        {
            totalMembers += uni.members.size();
            totalSize += uni.size;
        }
        ss << L"// Total Members: " << totalMembers << L"\r\n";
        ss << L"// Total Size: " << totalSize << L" bytes\r\n";
        content = ss.str();
    }
    else if (param == 5)
    {
        std::wostringstream ss;
        ss << L"// Enums List\r\n\r\n";
        ss << L"// Total Enums: " << g_moduleInfo.enums.size() << L"\r\n\r\n";
        
        size_t totalValues = 0;
        for (const auto& enm : g_moduleInfo.enums)
        {
            totalValues += enm.values.size();
        }
        ss << L"// Total Enum Values: " << totalValues << L"\r\n";
        content = ss.str();
    }
    else if (param == 6)
    {
        std::wostringstream ss;
        ss << L"// Global Variables List\r\n\r\n";
        ss << L"// Total Global Variables: " << g_moduleInfo.globalVariables.size() << L"\r\n\r\n";
        
        size_t totalSize = 0;
        for (const auto& var : g_moduleInfo.globalVariables)
        {
            totalSize += var.size;
        }
        ss << L"// Total Size: " << totalSize << L" bytes\r\n";
        content = ss.str();
    }
    else if (TreeNodeParamHelper::IsFunctionNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.functions.size())
        {
            const auto& func = g_moduleInfo.functions[idx];
            std::wostringstream ss;
            
            ss << L"// Function Declaration\r\n\r\n";
            ss << PDBParser::GenerateFunctionSignature(func) << L";\r\n\r\n";
            
            ss << L"// Additional Information:\r\n";
            if (func.rva != 0)
            {
                ss << L"// RVA: 0x" << std::hex << func.rva << L"\r\n";
            }
            if (func.virtualAddress != 0)
            {
                ss << L"// Virtual Address: 0x" << std::hex << func.virtualAddress << L"\r\n";
            }
            if (func.size != 0)
            {
                ss << L"// Size: " << std::dec << func.size << L" bytes\r\n";
            }
            if (func.isStatic)
            {
                ss << L"// Static: Yes\r\n";
            }
            if (func.isVirtual)
            {
                ss << L"// Virtual: Yes\r\n";
            }
            if (func.isMemberFunction && !func.className.empty())
            {
                ss << L"// Class: " << func.className << L"\r\n";
            }
            
            content = ss.str();
        }
    }
    else if (TreeNodeParamHelper::IsClassNode(param) || TreeNodeParamHelper::IsStructNode(param) || TreeNodeParamHelper::IsUnionNode(param))
    {
        const ClassInfo* pClass = nullptr;
        
        if (TreeNodeParamHelper::IsClassNode(param))
        {
            size_t idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.classes.size())
            {
                pClass = &g_moduleInfo.classes[idx];
            }
        }
        else if (TreeNodeParamHelper::IsStructNode(param))
        {
            size_t idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.structs.size())
            {
                pClass = &g_moduleInfo.structs[idx];
            }
        }
        else if (TreeNodeParamHelper::IsUnionNode(param))
        {
            size_t idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.unions.size())
            {
                pClass = &g_moduleInfo.unions[idx];
            }
        }

        if (pClass) {
            std::wostringstream ss;
            ss << L"// Auto-generated class/struct definition\r\n\r\n";
            
            ExportSettings settings = ConfigManager::GetInstance().GetExportSettings();
            settings.flattenNamespaces = false;
            
            if (pClass->isUnion) {
                ss << PDBHeaderGenerator::GenerateUnionDeclaration(*pClass, settings, g_numberMode, g_expandBaseClasses, &g_moduleInfo);
            } else if (pClass->isStruct) {
                ss << PDBHeaderGenerator::GenerateStructDeclaration(*pClass, settings, g_numberMode, g_expandBaseClasses, &g_moduleInfo);
            } else {
                ss << PDBHeaderGenerator::GenerateClassDeclaration(*pClass, settings, g_numberMode, g_expandBaseClasses, &g_moduleInfo);
            }
            
            if (!pClass->virtualFunctions.empty()) {
                ss << L"\r\n// Virtual Function Table:\r\n";
                ss << L"//=======================================\r\n";
                for (size_t i = 0; i < pClass->virtualFunctions.size(); ++i) {
                    const auto& vfunc = pClass->virtualFunctions[i];
                    ss << L"// [" << vfunc.vtableIndex << L"] " << vfunc.returnType << L" " << vfunc.name << L"(";
                    for (size_t j = 0; j < vfunc.parameters.size(); ++j) {
                        if (j > 0) ss << L", ";
                        ss << vfunc.parameters[j].type;
                        if (!vfunc.parameters[j].name.empty()) {
                            ss << L" " << vfunc.parameters[j].name;
                        }
                    }
                    ss << L")";
                    if (vfunc.rva != 0) {
                        ss << L" // RVA: 0x" << std::hex << vfunc.rva;
                    }
                    if (vfunc.virtualAddress != 0) {
                        if (vfunc.rva != 0) ss << L", ";
                        ss << L" VA: 0x" << std::hex << vfunc.virtualAddress;
                    }
                    ss << L"\r\n";
                }
                ss << L"//=======================================\r\n";
            }
            
            ss << L"\r\n// Additional Information:\r\n";
            ss << L"// Size: " << PDBHeaderGenerator::FormatNumber(pClass->size, g_numberMode) << L" bytes\r\n";
            ss << L"// Member count: " << pClass->members.size() << L"\r\n";
            ss << L"// Virtual function count: " << pClass->virtualFunctions.size() << L"\r\n";
            
            if (!pClass->baseClasses.empty()) {
                ss << L"// Base classes: ";
                for (size_t i = 0; i < pClass->baseClasses.size(); ++i) {
                    if (i > 0) ss << L", ";
                    ss << pClass->baseClasses[i].name;
                }
                ss << L"\r\n";
            }
            
            content = ss.str();
        }
    }
    else if (TreeNodeParamHelper::IsEnumNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.enums.size())
        {
            const auto& enm = g_moduleInfo.enums[idx];
            std::wostringstream ss;
            ss << L"// Auto-generated enum definition\r\n\r\n";
            ss << PDBHeaderGenerator::GenerateEnumDeclaration(enm, g_numberMode);
            ss << L"\r\n// Additional Information:\r\n";
            ss << L"// Underlying type: " << (enm.underlyingType.empty() ? L"int" : enm.underlyingType) << L"\r\n";
            ss << L"// Value count: " << enm.values.size() << L"\r\n";
            content = ss.str();
        }
    }
    else if (TreeNodeParamHelper::IsGlobalVarNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.globalVariables.size())
        {
            const auto& var = g_moduleInfo.globalVariables[idx];
            std::wostringstream ss;
            ss << L"// Global Variable Declaration\r\n\r\n";
            ss << PDBHeaderGenerator::RenderDeclaration(var.typeRef, var.name) << L";\r\n\r\n";
            ss << L"// Additional Information:\r\n";
            if (var.rva != 0)
            {
                ss << L"// RVA: 0x" << std::hex << var.rva << L"\r\n";
            }
            if (var.virtualAddress != 0)
            {
                ss << L"// Virtual Address: 0x" << std::hex << var.virtualAddress << L"\r\n";
            }
            if (var.size != 0)
            {
                ss << L"// Size: " << std::dec << var.size << L" bytes\r\n";
            }
            content = ss.str();
        }
    }

    SetWindowTextW(hRichEdit, content.c_str());
    const auto spans = BuildSyntaxSpans(content);
    ApplySyntaxHighlighting(hRichEdit, content, spans);
}

void SetRichEditRangeColor(HWND hRichEdit, LONG start, LONG end, COLORREF color)
{
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;

    CHARRANGE range{start, end};
    SendMessageW(hRichEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void SetRichEditRangeBold(HWND hRichEdit, LONG start, LONG end)
{
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD;
    cf.dwEffects = CFE_BOLD;

    SendMessageW(hRichEdit, EM_SETSEL, start, end);
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

void ApplySyntaxHighlighting(HWND hRichEdit) {
    if (!hRichEdit) return;
    GETTEXTLENGTHEX gtle;
    gtle.flags = GTL_NUMCHARS;
    gtle.codepage = 1200;
    LONG textLen = (LONG)SendMessageW(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
    if (textLen <= 0) return;

    std::vector<WCHAR> buffer(textLen + 1);
    GETTEXTEX gt;
    gt.cb = (textLen + 1) * sizeof(WCHAR);
    gt.flags = GT_DEFAULT;
    gt.codepage = 1200;
    gt.lpDefaultChar = NULL;
    gt.lpUsedDefChar = NULL;
    SendMessageW(hRichEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer.data());
    const std::wstring text(buffer.data());
    const auto spans = BuildSyntaxSpans(text);
    ApplySyntaxHighlighting(hRichEdit, text, spans);
}

void ApplySyntaxHighlighting(HWND hRichEdit, const std::wstring& sourceText) {
    if (!hRichEdit) return;
    const std::wstring& text = sourceText;
    if (text.empty()) return;

    const COLORREF COLOR_KEYWORD = RGB(0, 0, 255);
    const COLORREF COLOR_COMMENT = RGB(0, 128, 0);
    const COLORREF COLOR_STRING = RGB(163, 21, 21);
    const COLORREF COLOR_TYPE = RGB(43, 145, 175);
    const COLORREF COLOR_DEFAULT = RGB(0, 0, 0);

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
        L"unsigned", L"signed", L"const", L"static", L"virtual", L"inline", L"explicit",
        L"wchar_t", L"char16_t", L"char32_t", L"int8_t", L"int16_t", L"int32_t", L"int64_t",
        L"uint8_t", L"uint16_t", L"uint32_t", L"uint64_t", L"size_t", L"ptrdiff_t",
        L"intptr_t", L"uintptr_t", L"DWORD", L"WORD", L"BYTE", L"LONG", L"ULONG",
        L"BOOL", L"HRESULT", L"LPVOID", L"LPCVOID", L"LPSTR", L"LPCSTR", L"LPWSTR",
        L"LPCWSTR", L"HANDLE", L"HWND", L"HINSTANCE", L"HMODULE", L"HDWP", L"HRGN"
    };

    std::unordered_set<std::wstring> typeNames = basicTypes;
    if (g_pdbLoaded) {
        for (const auto& cls : g_moduleInfo.classes) {
            typeNames.insert(cls.name);
            typeNames.insert(PDBHeaderGenerator::FlattenName(cls.name));
        }
        for (const auto& str : g_moduleInfo.structs) {
            typeNames.insert(str.name);
            typeNames.insert(PDBHeaderGenerator::FlattenName(str.name));
        }
        for (const auto& uni : g_moduleInfo.unions) {
            typeNames.insert(uni.name);
            typeNames.insert(PDBHeaderGenerator::FlattenName(uni.name));
        }
        for (const auto& enm : g_moduleInfo.enums) {
            typeNames.insert(enm.name);
            typeNames.insert(PDBHeaderGenerator::FlattenName(enm.name));
        }
    }
    std::vector<std::wstring> typeNamesSnapshot(typeNames.begin(), typeNames.end());
    for (const auto& type : typeNamesSnapshot) {
        const size_t templatePos = type.find(L'<');
        if (templatePos != std::wstring::npos) typeNames.insert(type.substr(0, templatePos));
    }

    SendMessageW(hRichEdit, WM_SETREDRAW, FALSE, 0);

    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = COLOR_DEFAULT;
    cf.dwEffects = 0;
    SendMessageW(hRichEdit, EM_SETSEL, 0, -1);
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    std::vector<std::pair<LONG, LONG>> comments;
    std::vector<std::pair<LONG, LONG>> strings;
    std::vector<std::pair<LONG, LONG>> keywordsFound;
    std::vector<std::pair<LONG, LONG>> typesFound;

    LONG pos = 0;
    LONG len = (LONG)text.length();
    while (pos < len) {
        if (text[pos] == L'/' && pos + 1 < len) {
            if (text[pos + 1] == L'/') {
                LONG commentEnd = pos;
                while (commentEnd < len && text[commentEnd] != L'\r' && text[commentEnd] != L'\n') {
                    commentEnd++;
                }
                comments.push_back({pos, commentEnd});
                pos = commentEnd;
                continue;
            } else if (text[pos + 1] == L'*') {
                LONG commentEnd = pos + 2;
                while (commentEnd < len) {
                    if (text[commentEnd] == L'*' && commentEnd + 1 < len && text[commentEnd + 1] == L'/') {
                        commentEnd += 2;
                        break;
                    }
                    commentEnd++;
                }
                comments.push_back({pos, commentEnd});
                pos = commentEnd;
                continue;
            }
        }

        if (text[pos] == L'"') {
            LONG stringEnd = pos + 1;
            while (stringEnd < len) {
                if (text[stringEnd] == L'\\' && stringEnd + 1 < len) {
                    stringEnd += 2;
                } else if (text[stringEnd] == L'"') {
                    stringEnd++;
                    break;
                } else {
                    stringEnd++;
                }
            }
            strings.push_back({pos, stringEnd});
            pos = stringEnd;
            continue;
        }

        if ((text[pos] >= L'a' && text[pos] <= L'z') || 
            (text[pos] >= L'A' && text[pos] <= L'Z') || text[pos] == L'_') {
            LONG wordEnd = pos;
            while (wordEnd < len && 
                   ((text[wordEnd] >= L'a' && text[wordEnd] <= L'z') || 
                    (text[wordEnd] >= L'A' && text[wordEnd] <= L'Z') || 
                    (text[wordEnd] >= L'0' && text[wordEnd] <= L'9') || 
                    text[wordEnd] == L'_' || text[wordEnd] == L':')) {
                wordEnd++;
            }
            std::wstring word = text.substr(pos, wordEnd - pos);

            if (keywords.find(word) != keywords.end()) {
                keywordsFound.push_back({pos, wordEnd});
            } else if (typeNames.find(word) != typeNames.end()) {
                typesFound.push_back({pos, wordEnd});
            }
            pos = wordEnd;
            continue;
        }

        pos++;
    }

    for (const auto& range : comments) {
        SetRichEditRangeColor(hRichEdit, range.first, range.second, COLOR_COMMENT);
    }

    for (const auto& range : strings) {
        SetRichEditRangeColor(hRichEdit, range.first, range.second, COLOR_STRING);
    }

    for (const auto& range : keywordsFound) {
        SetRichEditRangeColor(hRichEdit, range.first, range.second, COLOR_KEYWORD);
    }

    for (const auto& range : typesFound) {
        SetRichEditRangeColor(hRichEdit, range.first, range.second, COLOR_TYPE);
    }

    SendMessageW(hRichEdit, EM_SETSEL, 0, 0);
    SendMessageW(hRichEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hRichEdit, nullptr, TRUE);
}
