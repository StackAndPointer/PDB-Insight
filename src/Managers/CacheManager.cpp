#include "CacheManager.h"
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

CacheManager& CacheManager::GetInstance() {
    static CacheManager instance;
    return instance;
}

CacheManager::CacheManager() {
}

std::wstring CacheManager::GetCacheFilePath(const std::wstring& pdbPath) {
    fs::path pdbPathObj(pdbPath);
    std::wstring cachePath = pdbPathObj.parent_path().wstring() + L"\\" + pdbPathObj.stem().wstring() + L".pdbbc";
    return cachePath;
}

bool CacheManager::CacheExists(const std::wstring& pdbPath) {
    std::wstring cachePath = GetCacheFilePath(pdbPath);
    return fs::exists(cachePath);
}

bool CacheManager::WriteString(std::ofstream& file, const std::wstring& str) {
    size_t length = str.length();
    file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    file.write(reinterpret_cast<const char*>(str.c_str()), length * sizeof(wchar_t));
    return file.good();
}

bool CacheManager::ReadString(std::ifstream& file, std::wstring& str) {
    size_t length;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (!file.good()) return false;
    
    str.resize(length);
    file.read(reinterpret_cast<char*>(&str[0]), length * sizeof(wchar_t));
    return file.good();
}

bool CacheManager::WriteModuleInfo(std::ofstream& file, const ModuleInfo& moduleInfo) {
    if (!WriteString(file, moduleInfo.name)) return false;
    if (!WriteString(file, moduleInfo.pdbFileName)) return false;
    
    size_t count = moduleInfo.functions.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& func : moduleInfo.functions) {
        if (!WriteFunctionInfo(file, func)) return false;
    }
    
    count = moduleInfo.classes.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& cls : moduleInfo.classes) {
        if (!WriteClassInfo(file, cls)) return false;
    }
    
    count = moduleInfo.structs.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& str : moduleInfo.structs) {
        if (!WriteClassInfo(file, str)) return false;
    }
    
    count = moduleInfo.unions.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& uni : moduleInfo.unions) {
        if (!WriteClassInfo(file, uni)) return false;
    }
    
    count = moduleInfo.enums.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& enm : moduleInfo.enums) {
        if (!WriteEnumInfo(file, enm)) return false;
    }
    
    count = moduleInfo.globalVariables.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& var : moduleInfo.globalVariables) {
        if (!WriteGlobalVariableInfo(file, var)) return false;
    }
    
    return file.good();
}

bool CacheManager::ReadModuleInfo(std::ifstream& file, ModuleInfo& moduleInfo) {
    if (!ReadString(file, moduleInfo.name)) return false;
    if (!ReadString(file, moduleInfo.pdbFileName)) return false;
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.functions.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadFunctionInfo(file, moduleInfo.functions[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.classes.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadClassInfo(file, moduleInfo.classes[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.structs.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadClassInfo(file, moduleInfo.structs[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.unions.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadClassInfo(file, moduleInfo.unions[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.enums.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadEnumInfo(file, moduleInfo.enums[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    moduleInfo.globalVariables.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadGlobalVariableInfo(file, moduleInfo.globalVariables[i])) return false;
    }
    
    return file.good();
}

bool CacheManager::WriteFunctionInfo(std::ofstream& file, const FunctionInfo& funcInfo) {
    if (!WriteString(file, funcInfo.name)) return false;
    if (!WriteString(file, funcInfo.undecoratedName)) return false;
    if (!WriteString(file, funcInfo.returnType)) return false;
    file.write(reinterpret_cast<const char*>(&funcInfo.callingConvention), sizeof(funcInfo.callingConvention));
    
    size_t count = funcInfo.parameters.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& param : funcInfo.parameters) {
        if (!WriteString(file, param.name)) return false;
        if (!WriteString(file, param.type)) return false;
        file.write(reinterpret_cast<const char*>(&param.hasDefaultValue), sizeof(param.hasDefaultValue));
    }
    
    file.write(reinterpret_cast<const char*>(&funcInfo.rva), sizeof(funcInfo.rva));
    file.write(reinterpret_cast<const char*>(&funcInfo.virtualAddress), sizeof(funcInfo.virtualAddress));
    file.write(reinterpret_cast<const char*>(&funcInfo.size), sizeof(funcInfo.size));
    file.write(reinterpret_cast<const char*>(&funcInfo.isStatic), sizeof(funcInfo.isStatic));
    file.write(reinterpret_cast<const char*>(&funcInfo.isVirtual), sizeof(funcInfo.isVirtual));
    file.write(reinterpret_cast<const char*>(&funcInfo.isMemberFunction), sizeof(funcInfo.isMemberFunction));
    if (!WriteString(file, funcInfo.className)) return false;
    
    return file.good();
}

bool CacheManager::ReadFunctionInfo(std::ifstream& file, FunctionInfo& funcInfo) {
    if (!ReadString(file, funcInfo.name)) return false;
    if (!ReadString(file, funcInfo.undecoratedName)) return false;
    if (!ReadString(file, funcInfo.returnType)) return false;
    file.read(reinterpret_cast<char*>(&funcInfo.callingConvention), sizeof(funcInfo.callingConvention));
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    funcInfo.parameters.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, funcInfo.parameters[i].name)) return false;
        if (!ReadString(file, funcInfo.parameters[i].type)) return false;
        file.read(reinterpret_cast<char*>(&funcInfo.parameters[i].hasDefaultValue), sizeof(funcInfo.parameters[i].hasDefaultValue));
    }
    
    file.read(reinterpret_cast<char*>(&funcInfo.rva), sizeof(funcInfo.rva));
    file.read(reinterpret_cast<char*>(&funcInfo.virtualAddress), sizeof(funcInfo.virtualAddress));
    file.read(reinterpret_cast<char*>(&funcInfo.size), sizeof(funcInfo.size));
    file.read(reinterpret_cast<char*>(&funcInfo.isStatic), sizeof(funcInfo.isStatic));
    file.read(reinterpret_cast<char*>(&funcInfo.isVirtual), sizeof(funcInfo.isVirtual));
    file.read(reinterpret_cast<char*>(&funcInfo.isMemberFunction), sizeof(funcInfo.isMemberFunction));
    if (!ReadString(file, funcInfo.className)) return false;
    
    return file.good();
}

bool CacheManager::WriteClassInfo(std::ofstream& file, const ClassInfo& classInfo) {
    if (!WriteString(file, classInfo.name)) return false;
    file.write(reinterpret_cast<const char*>(&classInfo.size), sizeof(classInfo.size));
    file.write(reinterpret_cast<const char*>(&classInfo.alignment), sizeof(classInfo.alignment));
    
    size_t count = classInfo.baseClasses.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& base : classInfo.baseClasses) {
        if (!WriteString(file, base.name)) return false;
        file.write(reinterpret_cast<const char*>(&base.inheritanceType), sizeof(base.inheritanceType));
        file.write(reinterpret_cast<const char*>(&base.offset), sizeof(base.offset));
        file.write(reinterpret_cast<const char*>(&base.access), sizeof(base.access));
    }
    
    count = classInfo.members.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& member : classInfo.members) {
        if (!WriteString(file, member.name)) return false;
        if (!WriteString(file, member.type)) return false;
        file.write(reinterpret_cast<const char*>(&member.offset), sizeof(member.offset));
        file.write(reinterpret_cast<const char*>(&member.access), sizeof(member.access));
        file.write(reinterpret_cast<const char*>(&member.bitPosition), sizeof(member.bitPosition));
        file.write(reinterpret_cast<const char*>(&member.bitSize), sizeof(member.bitSize));
    }
    
    count = classInfo.memberFunctions.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& func : classInfo.memberFunctions) {
        if (!WriteString(file, func)) return false;
    }
    
    count = classInfo.virtualFunctions.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& vfunc : classInfo.virtualFunctions) {
        if (!WriteVirtualFunctionInfo(file, vfunc)) return false;
    }
    
    file.write(reinterpret_cast<const char*>(&classInfo.isStruct), sizeof(classInfo.isStruct));
    file.write(reinterpret_cast<const char*>(&classInfo.isUnion), sizeof(classInfo.isUnion));
    
    return file.good();
}

bool CacheManager::ReadClassInfo(std::ifstream& file, ClassInfo& classInfo) {
    if (!ReadString(file, classInfo.name)) return false;
    file.read(reinterpret_cast<char*>(&classInfo.size), sizeof(classInfo.size));
    file.read(reinterpret_cast<char*>(&classInfo.alignment), sizeof(classInfo.alignment));
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    classInfo.baseClasses.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, classInfo.baseClasses[i].name)) return false;
        file.read(reinterpret_cast<char*>(&classInfo.baseClasses[i].inheritanceType), sizeof(classInfo.baseClasses[i].inheritanceType));
        file.read(reinterpret_cast<char*>(&classInfo.baseClasses[i].offset), sizeof(classInfo.baseClasses[i].offset));
        file.read(reinterpret_cast<char*>(&classInfo.baseClasses[i].access), sizeof(classInfo.baseClasses[i].access));
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    classInfo.members.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, classInfo.members[i].name)) return false;
        if (!ReadString(file, classInfo.members[i].type)) return false;
        file.read(reinterpret_cast<char*>(&classInfo.members[i].offset), sizeof(classInfo.members[i].offset));
        file.read(reinterpret_cast<char*>(&classInfo.members[i].access), sizeof(classInfo.members[i].access));
        file.read(reinterpret_cast<char*>(&classInfo.members[i].bitPosition), sizeof(classInfo.members[i].bitPosition));
        file.read(reinterpret_cast<char*>(&classInfo.members[i].bitSize), sizeof(classInfo.members[i].bitSize));
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    classInfo.memberFunctions.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, classInfo.memberFunctions[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    classInfo.virtualFunctions.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadVirtualFunctionInfo(file, classInfo.virtualFunctions[i])) return false;
    }
    
    file.read(reinterpret_cast<char*>(&classInfo.isStruct), sizeof(classInfo.isStruct));
    file.read(reinterpret_cast<char*>(&classInfo.isUnion), sizeof(classInfo.isUnion));
    
    return file.good();
}

bool CacheManager::WriteEnumInfo(std::ofstream& file, const EnumInfo& enumInfo) {
    if (!WriteString(file, enumInfo.name)) return false;
    if (!WriteString(file, enumInfo.underlyingType)) return false;
    
    size_t count = enumInfo.values.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& value : enumInfo.values) {
        if (!WriteString(file, value.name)) return false;
        file.write(reinterpret_cast<const char*>(&value.value), sizeof(value.value));
    }
    
    return file.good();
}

bool CacheManager::ReadEnumInfo(std::ifstream& file, EnumInfo& enumInfo) {
    if (!ReadString(file, enumInfo.name)) return false;
    if (!ReadString(file, enumInfo.underlyingType)) return false;
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    enumInfo.values.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, enumInfo.values[i].name)) return false;
        file.read(reinterpret_cast<char*>(&enumInfo.values[i].value), sizeof(enumInfo.values[i].value));
    }
    
    return file.good();
}

bool CacheManager::WriteGlobalVariableInfo(std::ofstream& file, const GlobalVariableInfo& varInfo) {
    if (!WriteString(file, varInfo.name)) return false;
    if (!WriteString(file, varInfo.type)) return false;
    file.write(reinterpret_cast<const char*>(&varInfo.rva), sizeof(varInfo.rva));
    file.write(reinterpret_cast<const char*>(&varInfo.virtualAddress), sizeof(varInfo.virtualAddress));
    file.write(reinterpret_cast<const char*>(&varInfo.size), sizeof(varInfo.size));
    
    return file.good();
}

bool CacheManager::ReadGlobalVariableInfo(std::ifstream& file, GlobalVariableInfo& varInfo) {
    if (!ReadString(file, varInfo.name)) return false;
    if (!ReadString(file, varInfo.type)) return false;
    file.read(reinterpret_cast<char*>(&varInfo.rva), sizeof(varInfo.rva));
    file.read(reinterpret_cast<char*>(&varInfo.virtualAddress), sizeof(varInfo.virtualAddress));
    file.read(reinterpret_cast<char*>(&varInfo.size), sizeof(varInfo.size));
    
    return file.good();
}

bool CacheManager::WriteVirtualFunctionInfo(std::ofstream& file, const VirtualFunctionInfo& vfuncInfo) {
    if (!WriteString(file, vfuncInfo.name)) return false;
    if (!WriteString(file, vfuncInfo.returnType)) return false;
    
    size_t count = vfuncInfo.parameters.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& param : vfuncInfo.parameters) {
        if (!WriteString(file, param.name)) return false;
        if (!WriteString(file, param.type)) return false;
        file.write(reinterpret_cast<const char*>(&param.hasDefaultValue), sizeof(param.hasDefaultValue));
    }
    
    file.write(reinterpret_cast<const char*>(&vfuncInfo.rva), sizeof(vfuncInfo.rva));
    file.write(reinterpret_cast<const char*>(&vfuncInfo.virtualAddress), sizeof(vfuncInfo.virtualAddress));
    file.write(reinterpret_cast<const char*>(&vfuncInfo.vtableIndex), sizeof(vfuncInfo.vtableIndex));
    
    return file.good();
}

bool CacheManager::ReadVirtualFunctionInfo(std::ifstream& file, VirtualFunctionInfo& vfuncInfo) {
    if (!ReadString(file, vfuncInfo.name)) return false;
    if (!ReadString(file, vfuncInfo.returnType)) return false;
    
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file.good()) return false;
    vfuncInfo.parameters.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (!ReadString(file, vfuncInfo.parameters[i].name)) return false;
        if (!ReadString(file, vfuncInfo.parameters[i].type)) return false;
        file.read(reinterpret_cast<char*>(&vfuncInfo.parameters[i].hasDefaultValue), sizeof(vfuncInfo.parameters[i].hasDefaultValue));
    }
    
    file.read(reinterpret_cast<char*>(&vfuncInfo.rva), sizeof(vfuncInfo.rva));
    file.read(reinterpret_cast<char*>(&vfuncInfo.virtualAddress), sizeof(vfuncInfo.virtualAddress));
    file.read(reinterpret_cast<char*>(&vfuncInfo.vtableIndex), sizeof(vfuncInfo.vtableIndex));
    
    return file.good();
}

bool CacheManager::SaveCache(const ModuleInfo& moduleInfo, const std::wstring& pdbPath) {
    std::wstring cachePath = GetCacheFilePath(pdbPath);
    std::ofstream file(cachePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 写入文件头
    char header[] = "PDBBC";
    file.write(header, 5);
    if (!file.good()) {
        file.close();
        return false;
    }
    
    // 写入版本号
    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    if (!file.good()) {
        file.close();
        return false;
    }
    
    // 写入模块信息
    if (!WriteModuleInfo(file, moduleInfo)) {
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

bool CacheManager::LoadCache(ModuleInfo& moduleInfo, const std::wstring& cachePath, std::wstring& errorMsg) {
    std::ifstream file(cachePath, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = L"无法打开文件";
        return false;
    }
    
    // 读取文件头
    char header[6] = {0};
    file.read(header, 5);
    if (!file.good()) {
        errorMsg = L"读取文件头失败";
        file.close();
        return false;
    }
    if (strcmp(header, "PDBBC") != 0) {
        errorMsg = L"文件头不正确";
        file.close();
        return false;
    }
    
    // 读取版本号
    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!file.good()) {
        errorMsg = L"读取版本号失败";
        file.close();
        return false;
    }
    if (version != 1) {
        errorMsg = L"版本号不匹配";
        file.close();
        return false;
    }
    
    // 读取模块信息
    if (!ReadModuleInfo(file, moduleInfo)) {
        errorMsg = L"读取模块信息失败";
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

std::wstring CacheManager::GetPDBFilePathFromCache(const std::wstring& cachePath) {
    fs::path cachePathObj(cachePath);
    std::wstring pdbPath = cachePathObj.parent_path().wstring() + L"\\" + cachePathObj.stem().wstring() + L".pdb";
    return pdbPath;
}