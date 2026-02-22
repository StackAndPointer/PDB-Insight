#pragma once

#include "PDBData.h"
#include <string>

class PDBExporter {
public:
    static bool ExportToCSV(const ModuleInfo& moduleInfo, const std::wstring& filePath, bool expandBaseClasses = false, const std::wstring& language = L"zh-CN");
    static bool ExportToXML(const ModuleInfo& moduleInfo, const std::wstring& filePath, bool expandBaseClasses = false);
    static bool ExportFunctionsToCSV(const std::vector<FunctionInfo>& functions, const std::wstring& filePath, const std::wstring& language = L"zh-CN");
    static bool ExportClassesToCSV(const std::vector<ClassInfo>& classes, const std::wstring& filePath, bool expandBaseClasses = false, const std::wstring& language = L"zh-CN", const ModuleInfo* moduleInfo = nullptr);
    static bool ExportGlobalVariablesToCSV(const std::vector<GlobalVariableInfo>& variables, const std::wstring& filePath, const std::wstring& language = L"zh-CN");

private:
    static std::wstring EscapeCSV(const std::wstring& str);
    static std::wstring EscapeXML(const std::wstring& str);
    static std::wstring GetCurrentDateTime();
    static const ClassInfo* FindClassInfo(const std::wstring& className, const ModuleInfo* moduleInfo);
};
