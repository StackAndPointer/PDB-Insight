#pragma once

#include "PDBViewerGlobals.h"

struct ExportOptions {
    bool flattenNamespaces;
    bool removeVoidParams;
    bool generateTypeDefs;
    bool includeAddressComments;
    bool generateEnumValueComments;
};

class ExportEnhancer {
public:
    static std::wstring FlattenName(const std::wstring& name);
    static std::wstring RemoveVoidFromParams(const std::wstring& signature);
    static std::wstring GenerateIDACompatibleHeader(const ModuleInfo& moduleInfo, const ExportOptions& options);
    static ExportOptions GetDefaultOptions();
    static bool ExportEnhancedHeader(const ClassInfo& cls, const std::wstring& filePath, const ExportOptions& options, NumberDisplayMode numberMode, bool expandBaseClasses, const ModuleInfo* moduleInfo);
    static bool ExportAllEnhancedHeaders(const ModuleInfo& moduleInfo, const std::wstring& directoryPath, const ExportOptions& options, NumberDisplayMode numberMode, bool expandBaseClasses);
};
