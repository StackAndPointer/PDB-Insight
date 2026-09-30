#pragma once

#include "PDBData.h"
#include "ConfigManager.h"
#include <string>
#include <fstream>
#include <sstream>

class PDBHeaderGenerator {
public:
    static bool WriteTextFileUtf8(const std::wstring& filePath, const std::wstring& content);
    static bool GenerateClassHeader(const ClassInfo& classInfo, const std::wstring& filePath, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static bool GenerateAllHeaders(const ModuleInfo& moduleInfo, const std::wstring& directoryPath, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false);
    static bool GenerateClassHeader(const ClassInfo& classInfo, const std::wstring& filePath, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static bool GenerateAllHeaders(const ModuleInfo& moduleInfo, const std::wstring& directoryPath, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false);
    static bool GenerateEnumsHeader(const ModuleInfo& moduleInfo, const std::wstring& filePath, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false);
    static std::wstring GenerateClassDeclaration(const ClassInfo& classInfo, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateStructDeclaration(const ClassInfo& structInfo, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateUnionDeclaration(const ClassInfo& unionInfo, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateEnumDeclaration(const EnumInfo& enumInfo, NumberDisplayMode numberMode = NUMBER_HEX);
    static std::wstring GenerateClassDeclaration(const ClassInfo& classInfo, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateStructDeclaration(const ClassInfo& structInfo, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateUnionDeclaration(const ClassInfo& unionInfo, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX, bool expandBaseClasses = false, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring GenerateEnumDeclaration(const EnumInfo& enumInfo, const ExportSettings& settings, NumberDisplayMode numberMode = NUMBER_HEX);
    static std::wstring GeneratePureCStructDeclaration(const ClassInfo& classInfo, const ExportSettings& settings, NumberDisplayMode numberMode, const ModuleInfo* moduleInfo = nullptr);
    static std::wstring FormatNumber(ULONGLONG value, NumberDisplayMode mode);
    static std::wstring FormatOffset(LONG offset, NumberDisplayMode mode);
    static std::wstring FormatOffset(LONG offset, NumberDisplayMode mode, bool reconstructed = false);
    static LONG ResolveOffset(LONG offset, LONG baseOffset = 0);
    static std::wstring FlattenName(const std::wstring& name);
    static std::wstring ProcessTypeName(const std::wstring& name, const ExportSettings& settings);
    static std::wstring RenderDeclaration(const TypeRef& type, const std::wstring& name);
    static std::wstring RemoveVoidParams(const std::wstring& signature);
    static std::wstring AccessTypeToString(AccessType access);
    static void CollectAllMembersFromOffsetZero(const ClassInfo& classInfo,
                                                const ModuleInfo* moduleInfo,
                                                const std::wstring& sourceBaseName,
                                                std::vector<MemberVariableInfo>& out);

private:
    static std::wstring EscapeForHeader(const std::wstring& str);
    static const ClassInfo* FindClassInfo(const std::wstring& className, const ModuleInfo* moduleInfo);
    static std::wstring GetIndent(const ExportSettings& settings, int level);
};
