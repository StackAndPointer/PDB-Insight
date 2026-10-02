#pragma once

#include "PDBData.h"
#include "dia2.h"
#include <atomic>
#include <functional>
#include <string>

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
        m_progressCallback = std::move(callback);
    }

    void SetCancellationCallback(std::function<bool()> callback) {
        m_cancellationCallback = std::move(callback);
    }

    // Optional diagnostic hook used by CLI batch runs. Receives
    // "<stage>=<milliseconds>" fragments; never invoked unless set.
    void SetStageTimingCallback(std::function<void(const std::wstring&)> callback) {
        m_stageTimingCallback = std::move(callback);
    }

    bool WasCancelled() const { return m_cancelled; }

    static std::wstring AccessTypeToString(AccessType access);
    static std::wstring CallingConventionToString(CallingConvention cc);
    static std::wstring GenerateFunctionSignature(const FunctionInfo& funcInfo);

private:
    bool Initialize();
    void Cleanup();
    bool OpenSession(const std::wstring& pdbPath, IDiaDataSource** dataSource,
                     IDiaSession** session, IDiaSymbol** globalScope,
                     std::wstring* errorMessage) const;
    bool IsCancelled();
    bool ReportProgress(int percent, const std::wstring& message);
    void ReportStageTiming(const wchar_t* stage, ULONGLONG milliseconds);

    void ParseFunctions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void RenderFunctionSignatures(ModuleInfo& moduleInfo);
    void ParseUdtSymbols(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseUdtSymbolsSingleSession(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseEnums(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseGlobalVariables(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo);
    void ParseClassDetails(IDiaSymbol* pClass, ClassInfo& classInfo);
    void ParseMemberVariables(IDiaSymbol* owner, std::vector<MemberVariableInfo>& members, int depth = 0);
    void ReconstructMemberOffsetsIfNeeded(std::vector<MemberVariableInfo>& members);
    void ParseFunctionDetails(IDiaSymbol* pFunction, FunctionInfo& funcInfo);
    void ParseParameters(IDiaSymbol* pFunction, std::vector<ParameterInfo>& params);
    TypeRef BuildTypeRef(IDiaSymbol* pType, int depth = 0);
    ULONGLONG InferTypeAlignment(IDiaSymbol* pType, ULONGLONG typeSize) const;
    bool IsAnonymousTypeName(const std::wstring& name) const;
    std::wstring GetSymbolName(IDiaSymbol* pSymbol);
    std::wstring GetUndecoratedName(IDiaSymbol* pSymbol);
    std::wstring GetTypeName(IDiaSymbol* pType);
    CallingConvention GetCallingConvention(DWORD cc);
    AccessType GetAccessType(DWORD access);

    IDiaDataSource* m_pDataSource;
    IDiaSession* m_pSession;
    IDiaSymbol* m_pGlobal;
    std::wstring m_pdbPath;
    std::wstring m_lastError;
    std::function<void(int, const std::wstring&)> m_progressCallback;
    std::function<bool()> m_cancellationCallback;
    std::function<void(const std::wstring&)> m_stageTimingCallback;
    bool m_cancelled = false;
    bool m_comInitialized = false;
};
