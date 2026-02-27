#pragma once

#include "PDBData.h"
#include "dia2.h"
#include <string>
#include <functional>

class PDBParser {
public:
    PDBParser();
    ~PDBParser();

    bool LoadPDB(const std::wstring& pdbPath);
    void Unload();
    bool IsLoaded() const { return m_pSession != nullptr; }

    ModuleInfo ParseModule();

    std::wstring GetLastError() const { return m_lastError; }

    void SetProgressCallback(std::function<void(int, const std::wstring&)> callback) {
        m_progressCallback = callback;
    }

    static std::wstring AccessTypeToString(AccessType access);
    static std::wstring CallingConventionToString(CallingConvention cc);
    static std::wstring GenerateFunctionSignature(const FunctionInfo& funcInfo);

private:
    bool Initialize();
    void Cleanup();

    void ParseFunctions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseClasses(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseStructs(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseUnions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseEnums(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseGlobalVariables(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseClassDetails(IDiaSymbol* pClass, ClassInfo& classInfo);
    void ParseFunctionDetails(IDiaSymbol* pFunction, FunctionInfo& funcInfo);
    void ParseParameters(IDiaSymbol* pFunction, std::vector<ParameterInfo>& params);
    void ParseVirtualFunctions(IDiaSymbol* pClass, ClassInfo& classInfo);
    std::wstring GetSymbolName(IDiaSymbol* pSymbol);
    std::wstring GetUndecoratedName(IDiaSymbol* pSymbol);
    std::wstring GetTypeName(IDiaSymbol* pType);
    CallingConvention GetCallingConvention(DWORD cc);
    AccessType GetAccessType(DWORD access);

    IDiaDataSource* m_pDataSource;
    IDiaSession* m_pSession;
    IDiaSymbol* m_pGlobal;
    std::wstring m_lastError;
    std::function<void(int, const std::wstring&)> m_progressCallback;
};
