#include "PDBHeaderGenerator.h"
#include "PDBParser.h"
#include <algorithm>
#include <set>
#include <functional>

namespace {
std::wstring RenderDeclarationImpl(const TypeRef& type, const std::wstring& name) {
    std::wstring prefix = type.isConst ? L"const " : L"";
    if (type.isVolatile) prefix += L"volatile ";

    switch (type.kind) {
    case TypeRefKind::Pointer:
    case TypeRefKind::Reference: {
        std::wstring declarator;
        if (type.kind == TypeRefKind::Reference) {
            declarator = type.isRValueReference ? L"&&" : L"&";
        } else {
            declarator = L"*";
            if (type.isConst) declarator += L" const";
            if (type.isVolatile) declarator += L" volatile";
        }
        declarator += name;
        if (type.child && (type.child->kind == TypeRefKind::Array ||
                           type.child->kind == TypeRefKind::Function)) {
            declarator = L"(" + declarator + L")";
        }
        return type.child ? RenderDeclarationImpl(*type.child, declarator) : prefix + declarator;
    }
    case TypeRefKind::Array: {
        std::wstring suffix = L"[";
        if (type.hasKnownArrayCount) suffix += std::to_wstring(type.arrayCount);
        suffix += L"]";
        return type.child ? RenderDeclarationImpl(*type.child, name + suffix)
                          : prefix + name + suffix;
    }
    case TypeRefKind::Function: {
        std::wstring parameters = L"(";
        for (size_t i = 0; i < type.functionParameters.size(); ++i) {
            if (i > 0) parameters += L", ";
            parameters += RenderDeclarationImpl(type.functionParameters[i], L"");
        }
        if (type.isVariadic) {
            parameters += type.functionParameters.empty() ? L"..." : L", ...";
        }
        parameters += L")";
        return type.child ? RenderDeclarationImpl(*type.child, name + parameters)
                          : prefix + name + parameters;
    }
    case TypeRefKind::Named:
    default: {
        std::wstring baseType = prefix + type.name;
        if (baseType.empty()) return name;
        return name.empty() ? baseType : baseType + L" " + name;
    }
    }
}

void AppendAccessLabel(std::wostringstream& ss, AccessType access, int indentLevel) {
    const std::wstring label = PDBHeaderGenerator::AccessTypeToString(access);
    if (label.empty()) return;
    ss << std::wstring(indentLevel * 4, L' ') << label << L":\r\n";
}

void AppendMemberDeclaration(std::wostringstream& ss, const MemberVariableInfo& member,
                             NumberDisplayMode numberMode, int indentLevel,
                             bool includeOffset, const ExportSettings& settings,
                             LONG offsetBase = 0, bool offsetKnown = true);

void AppendMemberCollection(std::wostringstream& ss,
                            const std::vector<MemberVariableInfo>& members,
                            NumberDisplayMode numberMode, int indentLevel,
                            bool includeOffset, AccessType defaultAccess,
                            bool forceAccessLabels, const ExportSettings& settings,
                            LONG offsetBase = 0, bool offsetKnown = true) {
    AccessType currentAccess = PDB_ACCESS_UNKNOWN;
    for (const auto& member : members) {
        const bool needsLabel = member.access != PDB_ACCESS_UNKNOWN &&
            (forceAccessLabels || member.access != defaultAccess);
        if (needsLabel && member.access != currentAccess) {
            AppendAccessLabel(ss, member.access, indentLevel);
            currentAccess = member.access;
        }
        AppendMemberDeclaration(ss, member, numberMode, indentLevel + 1, includeOffset,
                                settings, offsetBase, offsetKnown);
    }
}

void AppendMemberDeclaration(std::wostringstream& ss, const MemberVariableInfo& member,
                             NumberDisplayMode numberMode, int indentLevel,
                             bool includeOffset, const ExportSettings& settings,
                             LONG offsetBase, bool offsetKnown) {
    const std::wstring indent(indentLevel * 4, L' ');
    const bool absoluteOffsetKnown = offsetKnown && member.offset >= 0;
    const LONG absoluteOffset = absoluteOffsetKnown
        ? PDBHeaderGenerator::ResolveOffset(member.offset, offsetBase)
        : -1;
    const bool hasAbsoluteOffset = absoluteOffset >= 0;
    if (member.anonymousType && settings.expandAnonymousAggregates) {
        const ClassInfo& aggregate = *member.anonymousType;
        const AccessType defaultAccess = aggregate.isStruct || aggregate.isUnion
            ? PDB_ACCESS_PUBLIC : PDB_ACCESS_PRIVATE;
        ss << indent << (aggregate.isUnion ? L"union" : L"struct") << L"\r\n";
        ss << indent << L"{\r\n";
        AppendMemberCollection(ss, aggregate.members, numberMode, indentLevel,
                               includeOffset, defaultAccess,
                               !aggregate.isStruct && !aggregate.isUnion,
                               settings, absoluteOffset, hasAbsoluteOffset);
        ss << indent << L"};";
        if (includeOffset && hasAbsoluteOffset) {
            ss << L" // " << PDBHeaderGenerator::FormatOffset(absoluteOffset, numberMode);
        }
        ss << L"\r\n";
        return;
    }

    if (member.anonymousType) {
        ss << indent << L"// Anonymous struct/union member (expansion disabled)";
        if (includeOffset && !member.isStatic && hasAbsoluteOffset) {
            ss << L" // " << PDBHeaderGenerator::FormatOffset(absoluteOffset, numberMode);
        }
        ss << L"\r\n";
        return;
    }

    ss << indent;
    if (member.isStatic) ss << L"static ";
    ss << PDBHeaderGenerator::RenderDeclaration(member.typeRef, member.name);
    if (member.isBitfield && member.bitSize > 0) ss << L" : " << member.bitSize;
    ss << L";";
    if (includeOffset && !member.isStatic && hasAbsoluteOffset) {
        ss << L" // " << PDBHeaderGenerator::FormatOffset(absoluteOffset, numberMode);
    }
    ss << L"\r\n";
}

std::wstring BuildBaseList(const ClassInfo& classInfo, const ExportSettings& settings,
                           bool includeAccess) {
    std::wstring result;
    for (size_t i = 0; i < classInfo.baseClasses.size(); ++i) {
        if (i > 0) result += L", ";
        const auto& base = classInfo.baseClasses[i];
        const std::wstring baseName = settings.flattenNamespaces
            ? PDBHeaderGenerator::FlattenName(base.name) : base.name;
        if (includeAccess) {
            const std::wstring access = PDBHeaderGenerator::AccessTypeToString(base.access);
            if (!access.empty()) result += access + L" ";
        }
        if (base.inheritanceType == PDB_INHERITANCE_VIRTUAL) result += L"virtual ";
        result += baseName;
    }
    return result;
}
}

bool PDBHeaderGenerator::GenerateClassHeader(const ClassInfo& classInfo,
                                              const std::wstring& filePath,
                                              NumberDisplayMode numberMode,
                                              bool expandBaseClasses,
                                              const ModuleInfo* moduleInfo) {
    ExportSettings settings;
    return GenerateClassHeader(classInfo, filePath, settings, numberMode, expandBaseClasses, moduleInfo);
}

bool PDBHeaderGenerator::GenerateAllHeaders(const ModuleInfo& moduleInfo,
                                             const std::wstring& directoryPath,
                                             NumberDisplayMode numberMode,
                                             bool expandBaseClasses) {
    ExportSettings settings;
    return GenerateAllHeaders(moduleInfo, directoryPath, settings, numberMode, expandBaseClasses);
}

bool PDBHeaderGenerator::GenerateClassHeader(const ClassInfo& classInfo,
                                              const std::wstring& filePath,
                                              const ExportSettings& settings,
                                              NumberDisplayMode numberMode,
                                              bool expandBaseClasses,
                                              const ModuleInfo* moduleInfo) {
    std::wofstream file(filePath);
    if (!file.is_open()) return false;

    file << L"// Auto-generated header file\r\n";
    file << L"// Generated by PDB Insight\r\n\r\n";
    file << L"#pragma once\r\n\r\n";

    if (classInfo.isUnion) {
        file << GenerateUnionDeclaration(classInfo, settings, numberMode, expandBaseClasses, moduleInfo);
    } else if (classInfo.isStruct) {
        file << GenerateStructDeclaration(classInfo, settings, numberMode, expandBaseClasses, moduleInfo);
    } else {
        file << GenerateClassDeclaration(classInfo, settings, numberMode, expandBaseClasses, moduleInfo);
    }
    return true;
}

bool PDBHeaderGenerator::GenerateAllHeaders(const ModuleInfo& moduleInfo,
                                             const std::wstring& directoryPath,
                                             const ExportSettings& settings,
                                             NumberDisplayMode numberMode,
                                             bool expandBaseClasses) {
    bool success = true;
    auto exportGroup = [&](const std::vector<ClassInfo>& types) {
        for (const auto& type : types) {
            const std::wstring displayName = settings.flattenNamespaces
                ? FlattenName(type.name) : type.name;
            const std::wstring filePath = directoryPath + L"\\" + EscapeForHeader(displayName) + L".h";
            success = GenerateClassHeader(type, filePath, settings, numberMode,
                                          expandBaseClasses, &moduleInfo) && success;
        }
    };
    exportGroup(moduleInfo.classes);
    exportGroup(moduleInfo.structs);
    exportGroup(moduleInfo.unions);
    return success;
}

std::wstring PDBHeaderGenerator::GenerateClassDeclaration(const ClassInfo& classInfo,
                                                           NumberDisplayMode numberMode,
                                                           bool expandBaseClasses,
                                                           const ModuleInfo* moduleInfo) {
    ExportSettings settings;
    return GenerateClassDeclaration(classInfo, settings, numberMode, expandBaseClasses, moduleInfo);
}

std::wstring PDBHeaderGenerator::GenerateStructDeclaration(const ClassInfo& structInfo,
                                                            NumberDisplayMode numberMode,
                                                            bool expandBaseClasses,
                                                            const ModuleInfo* moduleInfo) {
    ExportSettings settings;
    return GenerateStructDeclaration(structInfo, settings, numberMode, expandBaseClasses, moduleInfo);
}

std::wstring PDBHeaderGenerator::GenerateUnionDeclaration(const ClassInfo& unionInfo,
                                                           NumberDisplayMode numberMode,
                                                           bool expandBaseClasses,
                                                           const ModuleInfo* moduleInfo) {
    ExportSettings settings;
    return GenerateUnionDeclaration(unionInfo, settings, numberMode, expandBaseClasses, moduleInfo);
}

std::wstring PDBHeaderGenerator::GenerateEnumDeclaration(const EnumInfo& enumInfo,
                                                          NumberDisplayMode numberMode) {
    ExportSettings settings;
    return GenerateEnumDeclaration(enumInfo, settings, numberMode);
}

std::wstring PDBHeaderGenerator::GeneratePureCStructDeclaration(
    const ClassInfo& classInfo, const ExportSettings& settings,
    NumberDisplayMode numberMode, const ModuleInfo* moduleInfo) {
    std::wostringstream ss;
    const std::wstring className = settings.flattenNamespaces
        ? FlattenName(classInfo.name) : EscapeForHeader(classInfo.name);
    std::vector<MemberVariableInfo> allMembers;

    if (moduleInfo) {
        std::set<std::pair<const ClassInfo*, LONG>> activeClasses;
        std::function<void(const ClassInfo&, LONG, bool, int)> collectMembers =
            [&](const ClassInfo& current, LONG currentOffset, bool offsetKnown, int depth) {
                if (depth > 32 || !activeClasses.insert({&current, currentOffset}).second) return;

                // Virtual bases do not have a fixed offset in the complete object.
                // They cannot be represented safely in a flattened C declaration.
                for (const auto& base : current.baseClasses) {
                    if (base.inheritanceType == PDB_INHERITANCE_VIRTUAL) continue;
                    const ClassInfo* baseClass = FindClassInfo(base.name, moduleInfo);
                    if (!baseClass) continue;
                    const LONG baseOrigin = offsetKnown
                        ? PDBHeaderGenerator::ResolveOffset(base.offset, currentOffset)
                        : -1;
                    collectMembers(*baseClass, baseOrigin, baseOrigin >= 0, depth + 1);
                }

                for (const auto& member : current.members) {
                    // Static members do not contribute to the C object layout.
                    if (member.isStatic) continue;
                    MemberVariableInfo expanded = member;
                    expanded.offset = offsetKnown
                        ? PDBHeaderGenerator::ResolveOffset(expanded.offset, currentOffset)
                        : -1;
                    allMembers.push_back(std::move(expanded));
                }

                activeClasses.erase({&current, currentOffset});
            };
        collectMembers(classInfo, 0, true, 0);
    } else {
        for (const auto& member : classInfo.members) {
            if (!member.isStatic) allMembers.push_back(member);
        }
    }

    ss << (classInfo.isUnion ? L"union " : L"struct ") << className << L"\r\n{\r\n";
    AppendMemberCollection(ss, allMembers, numberMode, 0, true, PDB_ACCESS_PUBLIC, false,
                           settings);
    ss << L"};\r\n";
    return ss.str();
}

bool PDBHeaderGenerator::GenerateEnumsHeader(const ModuleInfo& moduleInfo,
                                              const std::wstring& filePath,
                                              const ExportSettings& settings,
                                              NumberDisplayMode numberMode,
                                              bool expandBaseClasses) {
    std::wofstream file(filePath);
    if (!file.is_open()) return false;

    file << L"// Auto-generated header file with all classes/structs/unions\r\n";
    file << L"// Generated by PDB Insight\r\n\r\n";
    file << L"#pragma once\r\n\r\n";

    std::set<std::wstring> exportedNames;
    if (settings.includeEnumsInEnumsH) {
        for (const auto& enumInfo : moduleInfo.enums) {
            const std::wstring enumName = settings.flattenNamespaces
                ? FlattenName(enumInfo.name) : EscapeForHeader(enumInfo.name);
            if (exportedNames.insert(enumName).second) {
                file << GenerateEnumDeclaration(enumInfo, settings, numberMode) << L"\r\n\r\n";
            }
        }
    }

    auto exportTypes = [&](const std::vector<ClassInfo>& types) {
        for (const auto& type : types) {
            const std::wstring typeName = settings.flattenNamespaces
                ? FlattenName(type.name) : EscapeForHeader(type.name);
            if (!exportedNames.insert(typeName).second) continue;
            if (settings.idaCompatible) {
                file << GeneratePureCStructDeclaration(type, settings, numberMode, &moduleInfo);
            } else if (type.isUnion) {
                file << GenerateUnionDeclaration(type, settings, numberMode, expandBaseClasses, &moduleInfo);
            } else if (type.isStruct) {
                file << GenerateStructDeclaration(type, settings, numberMode, expandBaseClasses, &moduleInfo);
            } else {
                file << GenerateClassDeclaration(type, settings, numberMode, expandBaseClasses, &moduleInfo);
            }
            file << L"\r\n\r\n";
        }
    };
    exportTypes(moduleInfo.classes);
    exportTypes(moduleInfo.structs);
    exportTypes(moduleInfo.unions);
    return true;
}

std::wstring PDBHeaderGenerator::FormatNumber(ULONGLONG value, NumberDisplayMode mode) {
    std::wostringstream ss;
    switch (mode) {
    case NUMBER_HEX:
        ss << L"0x" << std::hex << value;
        break;
    case NUMBER_DEC:
        ss << std::dec << value;
        break;
    case NUMBER_BOTH:
        ss << L"0x" << std::hex << value << L" (" << std::dec << value << L")";
        break;
    }
    return ss.str();
}

LONG PDBHeaderGenerator::ResolveOffset(LONG offset, LONG baseOffset) {
    if (offset < 0 || baseOffset < 0) return -1;
    const LONGLONG resolved = static_cast<LONGLONG>(offset) + baseOffset;
    if (resolved < 0 || resolved > MAXLONG) return -1;
    return static_cast<LONG>(resolved);
}

std::wstring PDBHeaderGenerator::FormatOffset(LONG offset, NumberDisplayMode mode) {
    if (offset < 0) return L"N/A";
    return FormatNumber(static_cast<ULONGLONG>(offset), mode);
}

void PDBHeaderGenerator::CollectAllMembersFromOffsetZero(
    const ClassInfo& classInfo, const ModuleInfo* moduleInfo,
    std::vector<MemberVariableInfo>& out) {
    std::set<std::pair<const ClassInfo*, LONG>> active;
    std::function<void(const ClassInfo&, LONG, int)> collect =
        [&](const ClassInfo& current, LONG currentOffset, int depth) {
            if (depth > 32 || !active.insert({&current, currentOffset}).second) return;
            for (const auto& base : current.baseClasses) {
                if (base.inheritanceType == PDB_INHERITANCE_VIRTUAL) continue;
                const ClassInfo* baseClass = moduleInfo
                    ? FindClassInfo(base.name, moduleInfo) : nullptr;
                if (!baseClass || base.offset < 0) continue;
                collect(*baseClass, ResolveOffset(base.offset, currentOffset), depth + 1);
            }
            for (const auto& member : current.members) {
                if (member.isStatic) continue;
                MemberVariableInfo expanded = member;
                expanded.offset = ResolveOffset(member.offset, currentOffset);
                out.push_back(std::move(expanded));
            }
            active.erase({&current, currentOffset});
        };
    collect(classInfo, 0, 0);
}

const ClassInfo* PDBHeaderGenerator::FindClassInfo(const std::wstring& className,
                                                    const ModuleInfo* moduleInfo) {
    if (!moduleInfo) return nullptr;
    for (const auto& cls : moduleInfo->classes) if (cls.name == className) return &cls;
    for (const auto& str : moduleInfo->structs) if (str.name == className) return &str;
    for (const auto& uni : moduleInfo->unions) if (uni.name == className) return &uni;
    return nullptr;
}

std::wstring PDBHeaderGenerator::GenerateClassDeclaration(
    const ClassInfo& classInfo, const ExportSettings& settings,
    NumberDisplayMode numberMode, bool expandBaseClasses, const ModuleInfo* moduleInfo) {
    std::wostringstream ss;
    const std::wstring className = settings.flattenNamespaces
        ? FlattenName(classInfo.name) : classInfo.name;
    ss << L"class " << className;
    if (!classInfo.baseClasses.empty()) ss << L" : " << BuildBaseList(classInfo, settings, true);
    ss << L"\r\n{\r\n";

    if (expandBaseClasses && moduleInfo) {
        std::vector<MemberVariableInfo> expandedMembers;
        CollectAllMembersFromOffsetZero(classInfo, moduleInfo, expandedMembers);
        AppendMemberCollection(ss, expandedMembers, numberMode, 0, true,
                               PDB_ACCESS_PRIVATE, true, settings);
    } else {
        AppendMemberCollection(ss, classInfo.members, numberMode, 0, true,
                               PDB_ACCESS_PRIVATE, true, settings);
    }
    ss << L"};\r\n";
    return ss.str();
}

std::wstring PDBHeaderGenerator::GenerateStructDeclaration(
    const ClassInfo& structInfo, const ExportSettings& settings,
    NumberDisplayMode numberMode, bool expandBaseClasses, const ModuleInfo* moduleInfo) {
    std::wostringstream ss;
    const std::wstring structName = settings.flattenNamespaces
        ? FlattenName(structInfo.name) : structInfo.name;
    ss << L"struct " << structName;
    if (!structInfo.baseClasses.empty()) ss << L" : " << BuildBaseList(structInfo, settings, false);
    ss << L"\r\n{\r\n";

    if (expandBaseClasses && moduleInfo) {
        std::vector<MemberVariableInfo> expandedMembers;
        CollectAllMembersFromOffsetZero(structInfo, moduleInfo, expandedMembers);
        AppendMemberCollection(ss, expandedMembers, numberMode, 0, true,
                               PDB_ACCESS_PUBLIC, false, settings);
    } else {
        AppendMemberCollection(ss, structInfo.members, numberMode, 0, true,
                               PDB_ACCESS_PUBLIC, false, settings);
    }
    ss << L"};\r\n";
    return ss.str();
}

std::wstring PDBHeaderGenerator::GenerateUnionDeclaration(
    const ClassInfo& unionInfo, const ExportSettings& settings,
    NumberDisplayMode numberMode, bool expandBaseClasses, const ModuleInfo* moduleInfo) {
    UNREFERENCED_PARAMETER(expandBaseClasses);
    UNREFERENCED_PARAMETER(moduleInfo);
    std::wostringstream ss;
    const std::wstring unionName = settings.flattenNamespaces
        ? FlattenName(unionInfo.name) : unionInfo.name;
    ss << L"union " << unionName << L"\r\n{\r\n";
    AppendMemberCollection(ss, unionInfo.members, numberMode, 0, true,
                           PDB_ACCESS_PUBLIC, false, settings);
    ss << L"};\r\n";
    return ss.str();
}

std::wstring PDBHeaderGenerator::AccessTypeToString(AccessType access) {
    switch (access) {
    case PDB_ACCESS_PUBLIC: return L"public";
    case PDB_ACCESS_PROTECTED: return L"protected";
    case PDB_ACCESS_PRIVATE: return L"private";
    default: return L"";
    }
}

std::wstring PDBHeaderGenerator::EscapeForHeader(const std::wstring& str) {
    std::wstring result;
    for (wchar_t c : str) {
        if (c == L' ' || c == L':' || c == L'-' || c == L'<' || c == L'>' ||
            c == L'(' || c == L')' || c == L',') {
            result += L'_';
        } else if (c == L'*') {
            result += L"_ptr";
        } else {
            result += c;
        }
    }
    return result;
}

std::wstring PDBHeaderGenerator::FlattenName(const std::wstring& name) {
    std::wstring result;
    for (wchar_t c : name) {
        if (c == L':' || c == L'<' || c == L'>' || c == L',' || c == L' ' ||
            c == L'(' || c == L')' || c == L'&' || c == L'*' || c == L'-') {
            result += L'_';
        } else {
            result += c;
        }
    }
    return result;
}

std::wstring PDBHeaderGenerator::RenderDeclaration(const TypeRef& type, const std::wstring& name) {
    return RenderDeclarationImpl(type, name);
}

std::wstring PDBHeaderGenerator::ProcessTypeName(const std::wstring& name,
                                                 const ExportSettings& settings) {
    std::wstring result = settings.flattenNamespaces ? FlattenName(name) : name;
    return settings.removeVoidParams ? RemoveVoidParams(result) : result;
}

std::wstring PDBHeaderGenerator::RemoveVoidParams(const std::wstring& signature) {
    std::wstring result = signature;
    size_t pos = 0;
    while ((pos = result.find(L"(void)", pos)) != std::wstring::npos) {
        result.replace(pos, 6, L"()");
        pos += 2;
    }
    return result;
}

std::wstring PDBHeaderGenerator::GenerateEnumDeclaration(const EnumInfo& enumInfo,
                                                          const ExportSettings& settings,
                                                          NumberDisplayMode numberMode) {
    std::wostringstream ss;
    const std::wstring enumName = settings.flattenNamespaces
        ? FlattenName(enumInfo.name) : enumInfo.name;
    ss << L"enum " << enumName;
    if (!enumInfo.underlyingType.empty()) ss << L" : " << enumInfo.underlyingType;
    ss << L"\r\n{\r\n";
    for (size_t i = 0; i < enumInfo.values.size(); ++i) {
        ss << L"    " << enumInfo.values[i].name << L" = "
           << FormatNumber(static_cast<ULONGLONG>(enumInfo.values[i].value), numberMode);
        if (i + 1 < enumInfo.values.size()) ss << L",";
        ss << L"\r\n";
    }
    ss << L"};\r\n";
    return ss.str();
}
