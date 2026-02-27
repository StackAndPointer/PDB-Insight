#pragma once

#include "PDBData.h"
#include <string>
#include <fstream>

class CacheManager {
public:
    static CacheManager& GetInstance();

    bool SaveCache(const ModuleInfo& moduleInfo, const std::wstring& pdbPath);
    bool LoadCache(ModuleInfo& moduleInfo, const std::wstring& cachePath, std::wstring& errorMsg);
    std::wstring GetPDBFilePathFromCache(const std::wstring& cachePath);
    std::wstring GetCacheFilePath(const std::wstring& pdbPath);
    bool CacheExists(const std::wstring& pdbPath);

private:
    CacheManager();
    ~CacheManager() = default;
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    bool WriteString(std::ofstream& file, const std::wstring& str);
    bool ReadString(std::ifstream& file, std::wstring& str);
    bool WriteModuleInfo(std::ofstream& file, const ModuleInfo& moduleInfo);
    bool ReadModuleInfo(std::ifstream& file, ModuleInfo& moduleInfo);
    bool WriteFunctionInfo(std::ofstream& file, const FunctionInfo& funcInfo);
    bool ReadFunctionInfo(std::ifstream& file, FunctionInfo& funcInfo);
    bool WriteClassInfo(std::ofstream& file, const ClassInfo& classInfo);
    bool ReadClassInfo(std::ifstream& file, ClassInfo& classInfo);
    bool WriteEnumInfo(std::ofstream& file, const EnumInfo& enumInfo);
    bool ReadEnumInfo(std::ifstream& file, EnumInfo& enumInfo);
    bool WriteGlobalVariableInfo(std::ofstream& file, const GlobalVariableInfo& varInfo);
    bool ReadGlobalVariableInfo(std::ifstream& file, GlobalVariableInfo& varInfo);
    bool WriteVirtualFunctionInfo(std::ofstream& file, const VirtualFunctionInfo& vfuncInfo);
    bool ReadVirtualFunctionInfo(std::ifstream& file, VirtualFunctionInfo& vfuncInfo);
};