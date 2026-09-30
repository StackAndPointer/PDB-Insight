#include "HeaderViewManager.h"
#include "FontManager.h"
#include "TreeNodeHelper.h"
#include <set>

namespace {
std::wstring T(const wchar_t* key, const wchar_t* fallback) {
    return LanguageManager::GetInstance().GetString(key, fallback);
}
}

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
        ss << T(L"comment_module_information", L"// Module Information") + L"\r\n\r\n";
        ss << T(L"comment_pdb_file", L"// PDB File: ") << g_moduleInfo.pdbFileName << L"\r\n";
        if (!g_moduleInfo.name.empty())
        {
            ss << T(L"comment_module_name", L"// Module Name: ") << g_moduleInfo.name << L"\r\n";
        }
        ss << L"\r\n" + T(L"comment_statistics", L"// Statistics:") + L"\r\n";
        ss << T(L"tree_functions", L"Functions") + L": " << g_moduleInfo.functions.size() << L"\r\n";
        ss << T(L"tree_classes", L"Classes") + L": " << g_moduleInfo.classes.size() << L"\r\n";
        ss << T(L"tree_structs", L"Structs") + L": " << g_moduleInfo.structs.size() << L"\r\n";
        ss << T(L"tree_unions", L"Unions") + L": " << g_moduleInfo.unions.size() << L"\r\n";
        ss << T(L"tree_enums", L"Enum") + L": " << g_moduleInfo.enums.size() << L"\r\n";
        ss << T(L"tree_global_variables", L"Global Variables") + L": " << g_moduleInfo.globalVariables.size() << L"\r\n";
        content = ss.str();
    }
    else if (param == 1)
    {
        std::wostringstream ss;
        ss << T(L"comment_functions_list", L"// Functions List") + L"\r\n\r\n";
        ss << T(L"comment_total_functions", L"// Total Functions: ") << g_moduleInfo.functions.size() << L"\r\n\r\n";
        
        size_t staticCount = 0, virtualCount = 0, memberCount = 0;
        for (const auto& func : g_moduleInfo.functions)
        {
            if (func.isStatic) staticCount++;
            if (func.isVirtual) virtualCount++;
            if (func.isMemberFunction) memberCount++;
        }
        ss << T(L"comment_static_functions", L"// Static Functions: ") << staticCount << L"\r\n";
        ss << T(L"comment_virtual_functions", L"// Virtual Functions: ") << virtualCount << L"\r\n";
        ss << T(L"comment_member_functions", L"// Member Functions: ") << memberCount << L"\r\n";
        ss << T(L"comment_global_functions", L"// Global Functions: ") << (g_moduleInfo.functions.size() - memberCount) << L"\r\n";
        content = ss.str();
    }
    else if (param == 2)
    {
        std::wostringstream ss;
        ss << T(L"comment_classes_list", L"// Classes List") + L"\r\n\r\n";
        ss << T(L"comment_total_classes", L"// Total Classes: ") << g_moduleInfo.classes.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0, totalVirtuals = 0;
        for (const auto& cls : g_moduleInfo.classes)
        {
            totalMembers += cls.members.size();
            totalVirtuals += cls.virtualFunctions.size();
        }
        ss << T(L"comment_total_members", L"// Total Members: ") << totalMembers << L"\r\n";
        ss << T(L"comment_total_virtual_functions", L"// Total Virtual Functions: ") << totalVirtuals << L"\r\n";
        content = ss.str();
    }
    else if (param == 3)
    {
        std::wostringstream ss;
        ss << T(L"comment_structs_list", L"// Structs List") + L"\r\n\r\n";
        ss << T(L"comment_total_structs", L"// Total Structs: ") << g_moduleInfo.structs.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0;
        size_t totalSize = 0;
        for (const auto& str : g_moduleInfo.structs)
        {
            totalMembers += str.members.size();
            totalSize += str.size;
        }
        ss << T(L"comment_total_members", L"// Total Members: ") << totalMembers << L"\r\n";
        ss << T(L"comment_total_size", L"// Total Size: ") << totalSize << T(L"comment_bytes", L" bytes") + L"\r\n";
        content = ss.str();
    }
    else if (param == 4)
    {
        std::wostringstream ss;
        ss << T(L"comment_unions_list", L"// Unions List") + L"\r\n\r\n";
        ss << T(L"tree_unions", L"Unions") + L": " << g_moduleInfo.unions.size() << L"\r\n\r\n";
        
        size_t totalMembers = 0;
        size_t totalSize = 0;
        for (const auto& uni : g_moduleInfo.unions)
        {
            totalMembers += uni.members.size();
            totalSize += uni.size;
        }
        ss << T(L"comment_total_members", L"// Total Members: ") << totalMembers << L"\r\n";
        ss << T(L"comment_total_size", L"// Total Size: ") << totalSize << T(L"comment_bytes", L" bytes") + L"\r\n";
        content = ss.str();
    }
    else if (param == 5)
    {
        std::wostringstream ss;
        ss << T(L"comment_enums_list", L"// Enums List") + L"\r\n\r\n";
        ss << T(L"comment_total_enums", L"// Total Enums: ") << g_moduleInfo.enums.size() << L"\r\n\r\n";
        
        size_t totalValues = 0;
        for (const auto& enm : g_moduleInfo.enums)
        {
            totalValues += enm.values.size();
        }
        ss << T(L"comment_total_enum_values", L"// Total Enum Values: ") << totalValues << L"\r\n";
        content = ss.str();
    }
    else if (param == 6)
    {
        std::wostringstream ss;
        ss << T(L"comment_global_variables_list", L"// Global Variables List") + L"\r\n\r\n";
        ss << T(L"comment_total_global_variables", L"// Total Global Variables: ") << g_moduleInfo.globalVariables.size() << L"\r\n\r\n";
        
        size_t totalSize = 0;
        for (const auto& var : g_moduleInfo.globalVariables)
        {
            totalSize += var.size;
        }
        ss << T(L"comment_total_size", L"// Total Size: ") << totalSize << T(L"comment_bytes", L" bytes") + L"\r\n";
        content = ss.str();
    }
    else if (TreeNodeParamHelper::IsFunctionNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.functions.size())
        {
            const auto& func = g_moduleInfo.functions[idx];
            std::wostringstream ss;
            
            ss << T(L"comment_function_decl", L"// Function Declaration") + L"\r\n\r\n";
            ss << PDBParser::GenerateFunctionSignature(func) << L";\r\n\r\n";
            
            ss << T(L"comment_additional_info", L"// Additional Information:") + L"\r\n";
            if (func.rva != 0)
            {
                ss << T(L"comment_rva", L"// RVA: ") << L"0x" << std::hex << func.rva << L"\r\n";
            }
            if (func.virtualAddress != 0)
            {
                ss << T(L"comment_virtual_address", L"// Virtual Address: ") << L"0x" << std::hex << func.virtualAddress << L"\r\n";
            }
            if (func.size != 0)
            {
                ss << T(L"comment_size", L"// Size: ") << std::dec << func.size << T(L"comment_bytes", L" bytes") + L"\r\n";
            }
            if (func.isStatic)
            {
                ss << T(L"comment_static_yes", L"// Static: Yes") + L"\r\n";
            }
            if (func.isVirtual)
            {
                ss << T(L"comment_virtual_yes", L"// Virtual: Yes") + L"\r\n";
            }
            if (func.isMemberFunction && !func.className.empty())
            {
                ss << T(L"comment_class", L"// Class: ") << func.className << L"\r\n";
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
            ss << T(L"comment_auto_generated_class", L"// Auto-generated class/struct definition") + L"\r\n\r\n";
            
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
                ss << L"\r\n" + T(L"comment_virtual_function_table", L"// Virtual Function Table:") + L"\r\n";
                ss << T(L"comment_rule", L"//=======================================") + L"\r\n";
                for (size_t i = 0; i < pClass->virtualFunctions.size(); ++i) {
                    const auto& vfunc = pClass->virtualFunctions[i];
                    ss << T(L"comment_vtable_slot_prefix", L"// [slot ") << vfunc.vtableIndex << L"] "
                       << vfunc.returnType << L" " << vfunc.name << L"(";
                    for (size_t j = 0; j < vfunc.parameters.size(); ++j) {
                        if (j > 0) ss << L", ";
                        ss << vfunc.parameters[j].type;
                        if (!vfunc.parameters[j].name.empty()) {
                            ss << L" " << vfunc.parameters[j].name;
                        }
                    }
                    ss << L")";
                    if (vfunc.rva != 0) {
                        ss << L" " + T(L"comment_rva", L"// RVA: ") + L"0x" << std::hex << vfunc.rva;
                    }
                    if (vfunc.virtualAddress != 0) {
                        if (vfunc.rva != 0) ss << L", ";
                        ss << L" " + T(L"comment_va", L"VA: ") << L"0x" << std::hex << vfunc.virtualAddress;
                    }
                    ss << L"\r\n";
                }
                ss << T(L"comment_rule", L"//=======================================") + L"\r\n";
            }
            
            ss << L"\r\n" + T(L"comment_additional_info", L"// Additional Information:") + L"\r\n";
            ss << T(L"comment_size", L"// Size: ") << PDBHeaderGenerator::FormatNumber(pClass->size, g_numberMode) << T(L"comment_bytes", L" bytes") + L"\r\n";
            ss << T(L"comment_member_count", L"// Member count: ") << pClass->members.size() << L"\r\n";
            ss << T(L"comment_virtual_function_count", L"// Virtual function count: ") << pClass->virtualFunctions.size() << L"\r\n";
            
            if (!pClass->baseClasses.empty()) {
                ss << T(L"comment_base_classes", L"// Base classes: ");
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
            ss << T(L"comment_auto_generated_enum", L"// Auto-generated enum definition") + L"\r\n\r\n";
            ss << PDBHeaderGenerator::GenerateEnumDeclaration(enm, g_numberMode);
            ss << L"\r\n" + T(L"comment_additional_info", L"// Additional Information:") + L"\r\n";
            ss << T(L"comment_underlying_type", L"// Underlying type: ") << (enm.underlyingType.empty() ? L"int" : enm.underlyingType) << L"\r\n";
            ss << T(L"comment_value_count", L"// Value count: ") << enm.values.size() << L"\r\n";
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
            ss << T(L"comment_global_variable_decl", L"// Global Variable Declaration") + L"\r\n\r\n";
            ss << PDBHeaderGenerator::RenderDeclaration(var.typeRef, var.name) << L";\r\n\r\n";
            ss << T(L"comment_additional_info", L"// Additional Information:") + L"\r\n";
            if (var.rva != 0)
            {
                ss << T(L"comment_rva", L"// RVA: ") << L"0x" << std::hex << var.rva << L"\r\n";
            }
            if (var.virtualAddress != 0)
            {
                ss << T(L"comment_virtual_address", L"// Virtual Address: ") << L"0x" << std::hex << var.virtualAddress << L"\r\n";
            }
            if (var.size != 0)
            {
                ss << T(L"comment_size", L"// Size: ") << std::dec << var.size << T(L"comment_bytes", L" bytes") + L"\r\n";
            }
            content = ss.str();
        }
    }

    SetWindowTextW(hRichEdit, content.c_str());
    const auto spans = BuildSyntaxSpans(content);
    ApplySyntaxHighlighting(hRichEdit, content, spans);
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
    GETTEXTLENGTHEX gtle{};
    gtle.flags = GTL_NUMCHARS;
    gtle.codepage = 1200;
    LONG textLen = (LONG)SendMessageW(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
    if (textLen <= 0) return;

    std::vector<WCHAR> buffer(static_cast<size_t>(textLen) + 1);
    GETTEXTEX gt{};
    gt.cb = static_cast<DWORD>(buffer.size() * sizeof(WCHAR));
    gt.flags = GT_DEFAULT;
    gt.codepage = 1200;
    SendMessageW(hRichEdit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer.data());
    ApplySyntaxHighlighting(hRichEdit, std::wstring(buffer.data()));
}

void ApplySyntaxHighlighting(HWND hRichEdit, const std::wstring& sourceText)
{
    if (!hRichEdit) return;
    if (sourceText.empty())
    {
        SendMessageW(hRichEdit, WM_SETREDRAW, FALSE, 0);
        CHARFORMAT2 cf{};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR | CFM_BOLD;
        cf.crTextColor = RGB(0, 0, 0);
        cf.dwEffects = 0;
        SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
        SendMessageW(hRichEdit, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hRichEdit, nullptr, TRUE);
        return;
    }
    ApplySyntaxHighlighting(hRichEdit, sourceText, BuildSyntaxSpans(sourceText));
}
