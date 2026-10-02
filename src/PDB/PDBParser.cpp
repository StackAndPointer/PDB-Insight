#include "PDBParser.h"
#include "PDBHeaderGenerator.h"
#include "diaCreate.h"
#include "cvConst.h"
#include <comdef.h>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <shlwapi.h>
#include <thread>
#include <unordered_set>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace {
std::mutex s_timingMutex;
}

PDBParser::PDBParser()
    : m_pDataSource(nullptr)
    , m_pSession(nullptr)
    , m_pGlobal(nullptr)
{
    Initialize();
}

PDBParser::~PDBParser() {
    if (m_comInitialized) {
        Cleanup();
        CoUninitialize();
        m_comInitialized = false;
    }
}

bool PDBParser::Initialize() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (hr == RPC_E_CHANGED_MODE) {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    if (hr == S_FALSE) {
        return true;
    }
    if (FAILED(hr)) {
        m_lastError = L"Failed to initialize COM";
        return false;
    }
    m_comInitialized = true;
    return true;
}

void PDBParser::Cleanup() {
    if (m_pGlobal) {
        m_pGlobal->Release();
        m_pGlobal = nullptr;
    }
    if (m_pSession) {
        m_pSession->Release();
        m_pSession = nullptr;
    }
    if (m_pDataSource) {
        m_pDataSource->Release();
        m_pDataSource = nullptr;
    }
    m_pdbPath.clear();
}

void PDBParser::Unload() {
    Cleanup();
}

bool PDBParser::IsCancelled() {
    if (m_cancellationCallback && m_cancellationCallback()) {
        m_cancelled = true;
    }
    return m_cancelled;
}

bool PDBParser::ReportProgress(int percent, const std::wstring& message) {
    if (IsCancelled()) return false;
    if (m_progressCallback) m_progressCallback(percent, message);
    return !IsCancelled();
}

void PDBParser::ReportStageTiming(const wchar_t* stage, ULONGLONG milliseconds) {
    if (!m_stageTimingCallback || !stage) return;
    std::wstring fragment = stage;
    fragment += L"=";
    fragment += std::to_wstring(milliseconds);
    m_stageTimingCallback(fragment);
}

bool PDBParser::OpenSession(const std::wstring& pdbPath, IDiaDataSource** dataSource,
                            IDiaSession** session, IDiaSymbol** globalScope,
                            std::wstring* errorMessage) const {
    if (!dataSource || !session || !globalScope || !errorMessage) return false;

    *dataSource = nullptr;
    *session = nullptr;
    *globalScope = nullptr;
    errorMessage->clear();

    HRESULT hr = NoRegCoCreate(L"msdia140.dll", _uuidof(DiaSource),
        _uuidof(IDiaDataSource), reinterpret_cast<void**>(dataSource));
    if (FAILED(hr)) {
        *errorMessage = L"Failed to create DIA data source:\n";
        *errorMessage += GetDebugInfo();
        return false;
    }

    hr = (*dataSource)->loadDataFromPdb(pdbPath.c_str());
    if (FAILED(hr)) {
        *errorMessage = L"Failed to load PDB file";
        return false;
    }

    hr = (*dataSource)->openSession(session);
    if (FAILED(hr)) {
        *errorMessage = L"Failed to open session";
        return false;
    }

    hr = (*session)->get_globalScope(globalScope);
    if (FAILED(hr)) {
        *errorMessage = L"Failed to get global scope";
        return false;
    }

    return true;
}

bool PDBParser::LoadPDB(const std::wstring& pdbPath) {
    Cleanup();

    if (IsCancelled()) return false;

    IDiaDataSource* dataSource = nullptr;
    IDiaSession* session = nullptr;
    IDiaSymbol* globalScope = nullptr;
    std::wstring errorMessage;
    if (!OpenSession(pdbPath, &dataSource, &session, &globalScope, &errorMessage)) {
        if (dataSource) dataSource->Release();
        if (session) session->Release();
        if (globalScope) globalScope->Release();
        if (IsCancelled()) return false;
        m_lastError = std::move(errorMessage);
        return false;
    }

    m_pDataSource = dataSource;
    m_pSession = session;
    m_pGlobal = globalScope;
    m_pdbPath = pdbPath;
    m_cancelled = false;
    return true;
}

ModuleInfo PDBParser::ParseModule() {
    ModuleInfo moduleInfo;

    if (!m_pGlobal) {
        return moduleInfo;
    }

    BSTR bstrName = nullptr;
    m_pGlobal->get_name(&bstrName);
    if (bstrName) {
        moduleInfo.name = bstrName;
        SysFreeString(bstrName);
    }

    constexpr ULONGLONG kParallelParseThresholdBytes = 8ULL * 1024 * 1024;
    ULONGLONG pdbSize = 0;
    if (!m_pdbPath.empty()) {
        WIN32_FILE_ATTRIBUTE_DATA fileInfo{};
        if (GetFileAttributesExW(m_pdbPath.c_str(), GetFileExInfoStandard, &fileInfo)) {
            pdbSize = (static_cast<ULONGLONG>(fileInfo.nFileSizeHigh) << 32) |
                fileInfo.nFileSizeLow;
        }
    }

    if (m_pdbPath.empty() || pdbSize < kParallelParseThresholdBytes) {
        ULONGLONG t0 = GetTickCount64();
        if (!ReportProgress(0, L"Parsing functions...")) return moduleInfo;
        ParseFunctions(m_pGlobal, moduleInfo);
        ULONGLONG t1 = GetTickCount64();
        ReportStageTiming(L"functions", t1 - t0);
        if (!ReportProgress(20, L"Parsing global variables...")) return moduleInfo;
        ParseGlobalVariables(m_pGlobal, moduleInfo);
        ULONGLONG t2 = GetTickCount64();
        ReportStageTiming(L"globals", t2 - t1);
        if (!ReportProgress(40, L"Parsing classes...")) return moduleInfo;
        ParseUdtSymbols(m_pGlobal, moduleInfo);
        ULONGLONG t3 = GetTickCount64();
        ReportStageTiming(L"udt", t3 - t2);
        ParseEnums(m_pGlobal, moduleInfo);
        ReportStageTiming(L"enums", GetTickCount64() - t3);
        RenderFunctionSignatures(moduleInfo);
        if (!ReportProgress(100, L"Complete")) return moduleInfo;
        return moduleInfo;
    }

    enum class ParseStage {
        Functions,
        GlobalVariables,
        UdtSymbols,
        Enums
    };

    struct StageResult {
        ModuleInfo module;
        std::wstring error;
        bool cancelled = false;
    };

    const std::wstring pdbPath = m_pdbPath;
    std::vector<StageResult> results(4);
    std::vector<std::thread> workers;
    workers.reserve(4);
    std::atomic<int> completed{0};

    if (!ReportProgress(0, L"Parsing functions...")) return moduleInfo;

    // Start every stage at once. The light stages finish well before UDT, and
    // overlapping them with UDT hides their cost; serialising the two parallel
    // pools measured slower because DIA access is internally serialised anyway.
    for (size_t index = 0; index < 4; ++index) {
        workers.emplace_back([this, index, &pdbPath, &results, &completed]() {
            HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool comInitialized = SUCCEEDED(comResult);
            if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                results[index].error = L"Failed to initialize COM in parser worker";
                completed.fetch_add(1, std::memory_order_relaxed);
                return;
            }

            ParseStage stage = static_cast<ParseStage>(index);
            StageResult& result = results[index];

            IDiaDataSource* dataSource = nullptr;
            IDiaSession* session = nullptr;
            IDiaSymbol* globalScope = nullptr;
            if (!OpenSession(pdbPath, &dataSource, &session, &globalScope, &result.error)) {
                if (dataSource) dataSource->Release();
                if (session) session->Release();
                if (globalScope) globalScope->Release();
                if (comInitialized) CoUninitialize();
                completed.fetch_add(1, std::memory_order_relaxed);
                return;
            }

            const ULONGLONG stageStart = GetTickCount64();
            ULONGLONG functionDiaStart = 0;
            switch (stage) {
                case ParseStage::Functions:
                    functionDiaStart = GetTickCount64();
                    ParseFunctions(globalScope, result.module);
                    if (m_stageTimingCallback) {
                        std::lock_guard<std::mutex> lock(s_timingMutex);
                        m_stageTimingCallback(L"functions-dia=" +
                            std::to_wstring(GetTickCount64() - functionDiaStart) +
                            L"ms/" + std::to_wstring(result.module.functions.size()) +
                            L"fns");
                    }
                    break;
                case ParseStage::GlobalVariables:
                    ParseGlobalVariables(globalScope, result.module);
                    break;
                case ParseStage::UdtSymbols:
                    ParseUdtSymbols(globalScope, result.module);
                    break;
                case ParseStage::Enums:
                    ParseEnums(globalScope, result.module);
                    break;
            }
            const wchar_t* stageName =
                stage == ParseStage::Functions ? L"functions" :
                stage == ParseStage::GlobalVariables ? L"globals" :
                stage == ParseStage::UdtSymbols ? L"udt" : L"enums";
            {
                std::lock_guard<std::mutex> lock(s_timingMutex);
                ReportStageTiming(stageName, GetTickCount64() - stageStart);
            }

            result.cancelled = IsCancelled();
            globalScope->Release();
            session->Release();
            dataSource->Release();
            completed.fetch_add(1, std::memory_order_relaxed);
            if (comInitialized) CoUninitialize();
        });
    }

    constexpr UINT_PTR kProgressTimerId = 0x50444249;
    const HWND progressOwner = GetForegroundWindow();
    const bool useTimer = progressOwner && IsWindow(progressOwner);
    if (useTimer) SetTimer(progressOwner, kProgressTimerId, 150, nullptr);

    const int totalStages = static_cast<int>(workers.size());
    while (completed.load(std::memory_order_relaxed) < totalStages) {
        const int now = completed.load(std::memory_order_relaxed);
        const int percent = (std::min)(99, (now * 100) / totalStages);
        const wchar_t* message = L"Parsing functions...";
        if (now == 1) message = L"Parsing global variables...";
        else if (now == 2) message = L"Parsing classes...";
        else if (now >= 3) message = L"Finalizing...";
        if (!ReportProgress(percent, message)) break;
        MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (IsCancelled()) break;
    }

    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }

    if (useTimer) KillTimer(progressOwner, kProgressTimerId);

    if (IsCancelled()) return ModuleInfo();

    for (const auto& result : results) {
        if (result.cancelled) {
            m_cancelled = true;
            return ModuleInfo();
        }
    }

    for (const auto& result : results) {
        if (!result.error.empty()) {
            m_lastError = result.error;
            return ModuleInfo();
        }
    }

    moduleInfo.functions = std::move(results[0].module.functions);
    moduleInfo.globalVariables = std::move(results[1].module.globalVariables);
    moduleInfo.classes = std::move(results[2].module.classes);
    moduleInfo.structs = std::move(results[2].module.structs);
    moduleInfo.unions = std::move(results[2].module.unions);
    moduleInfo.enums = std::move(results[3].module.enums);

    RenderFunctionSignatures(moduleInfo);

    if (!ReportProgress(100, L"Complete")) return moduleInfo;
    return moduleInfo;
}

void PDBParser::RenderFunctionSignatures(ModuleInfo& moduleInfo) {
    std::vector<FunctionInfo>& functions = moduleInfo.functions;
    if (functions.empty()) return;

    unsigned int threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 4;
    threads = (std::min)(threads, static_cast<unsigned int>(8));
    if (functions.size() < 256) threads = 1;

    std::atomic<size_t> next{0};
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (unsigned int slot = 0; slot < threads; ++slot) {
        workers.emplace_back([this, &functions, &next]() {
            while (!IsCancelled()) {
                const size_t index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= functions.size()) break;
                functions[index].displaySignature =
                    GenerateFunctionSignature(functions[index]);
            }
        });
    }
    for (auto& thread : workers) {
        if (thread.joinable()) thread.join();
    }
}

void PDBParser::ParseFunctions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    // Symbol-level parallelism: functions are independent, so shard the global
    // function enumeration across worker sessions. Each worker collects its own
    // slice of FunctionInfo; a final merge replays the shards in ordinal order
    // to preserve the serial enumeration ordering.
    ULONG functionCount = 0;
    {
        IDiaEnumSymbols* pEnum = nullptr;
        if (FAILED(pGlobal->findChildren(SymTagFunction, nullptr, nsNone, &pEnum)) || !pEnum) {
            return;
        }
        LONG count = 0;
        if (SUCCEEDED(pEnum->get_Count(&count)) && count > 0) {
            functionCount = static_cast<ULONG>(count);
        }
        pEnum->Release();
    }

    unsigned int workerCount = std::thread::hardware_concurrency();
    if (workerCount == 0) workerCount = 4;
    workerCount = (std::min)(workerCount, static_cast<unsigned int>(6));
    if (functionCount < 512) workerCount = 1;
    if (functionCount == 0) workerCount = 1;
    const ULONG functionChunkSize =
        (functionCount + workerCount - 1) / workerCount;

    std::vector<std::vector<FunctionInfo>> perWorkerFunctions(workerCount);
    std::vector<ULONGLONG> perWorkerMs(workerCount, 0);
    std::vector<bool> workerFailed(workerCount, false);
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    if (workerCount > 1) {
        for (unsigned int worker = 0; worker < workerCount; ++worker) {
            workers.emplace_back([this, &perWorkerFunctions, &perWorkerMs, &workerFailed,
                                  worker, workerCount, functionChunkSize, functionCount]() {
                HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                const bool comInitialized = SUCCEEDED(comResult);
                if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                    workerFailed[worker] = true;
                    return;
                }

                IDiaDataSource* dataSource = nullptr;
                IDiaSession* session = nullptr;
                IDiaSymbol* globalScope = nullptr;
                std::wstring errorMessage;
                if (!OpenSession(m_pdbPath, &dataSource, &session, &globalScope, &errorMessage)) {
                    workerFailed[worker] = true;
                    if (dataSource) dataSource->Release();
                    if (session) session->Release();
                    if (globalScope) globalScope->Release();
                    if (comInitialized) CoUninitialize();
                    return;
                }

                IDiaEnumSymbols* pEnumSymbols = nullptr;
                if (SUCCEEDED(globalScope->findChildren(SymTagFunction, nullptr, nsNone,
                                                        &pEnumSymbols)) && pEnumSymbols) {
                    const ULONGLONG start = GetTickCount64();
                    std::vector<FunctionInfo>& local = perWorkerFunctions[worker];
                    const ULONG begin = worker * functionChunkSize;
                    const ULONG finish = (std::min)(begin + functionChunkSize, functionCount);
                    local.reserve(finish > begin ? finish - begin : 0);
                    if (begin > 0) pEnumSymbols->Skip(begin);
                    IDiaSymbol* pSymbol = nullptr;
                    ULONG fetched = 0;
                    ULONG ordinal = 0;
                    while (!IsCancelled() &&
                           SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &fetched)) && fetched == 1) {
                        FunctionInfo funcInfo;
                        ParseFunctionDetails(pSymbol, funcInfo);
                        if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
                            local.push_back(std::move(funcInfo));
                        }
                        pSymbol->Release();
                        if (++ordinal >= functionChunkSize) break;
                    }
                    pEnumSymbols->Release();
                    perWorkerMs[worker] = GetTickCount64() - start;
                } else {
                    workerFailed[worker] = true;
                }

                globalScope->Release();
                session->Release();
                dataSource->Release();
                if (comInitialized) CoUninitialize();
            });
        }

        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    bool allWorkersFailed = workerCount > 1;
    for (unsigned int worker = 0; worker < workerCount; ++worker) {
        if (!workerFailed[worker]) { allWorkersFailed = false; break; }
    }

    if (workerCount == 1 || allWorkersFailed) {
        // Serial fallback on the caller's session.
        IDiaEnumSymbols* pEnumSymbols = nullptr;
        if (SUCCEEDED(pGlobal->findChildren(SymTagFunction, nullptr, nsNone, &pEnumSymbols)) &&
            pEnumSymbols) {
            IDiaSymbol* pSymbol = nullptr;
            ULONG fetched = 0;
            while (!IsCancelled() &&
                   SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &fetched)) && fetched == 1) {
                FunctionInfo funcInfo;
                ParseFunctionDetails(pSymbol, funcInfo);
                if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
                    moduleInfo.functions.push_back(std::move(funcInfo));
                }
                pSymbol->Release();
            }
            pEnumSymbols->Release();
        }
    } else {
        std::vector<size_t> cursor(workerCount, 0);
        size_t remaining = 0;
        for (unsigned int worker = 0; worker < workerCount; ++worker) {
            remaining += perWorkerFunctions[worker].size();
        }
        while (remaining > 0) {
            for (unsigned int worker = 0; worker < workerCount; ++worker) {
                auto& local = perWorkerFunctions[worker];
                if (cursor[worker] >= local.size()) continue;
                moduleInfo.functions.push_back(std::move(local[cursor[worker]++]));
                --remaining;
            }
        }
    }

    if (m_stageTimingCallback) {
        std::wstring detail = L"functions-workers=" + std::to_wstring(workerCount);
        for (unsigned int worker = 0; worker < workerCount; ++worker) {
            detail += L" w" + std::to_wstring(worker) + L":" +
                std::to_wstring(perWorkerFunctions[worker].size()) + L"/" +
                std::to_wstring(perWorkerMs[worker]) + L"ms";
        }
        std::lock_guard<std::mutex> lock(s_timingMutex);
        m_stageTimingCallback(detail);
    }

    // Public symbols supplement the function list. Keep this on the caller's
    // session; it is a cheap pass and must observe the RVAs already collected.
    IDiaEnumSymbols* pEnumPublic = nullptr;
    std::unordered_set<ULONGLONG> knownFunctionRvas;
    for (const auto& function : moduleInfo.functions) {
        if (function.rva != 0) knownFunctionRvas.insert(function.rva);
    }
    HRESULT hr = pGlobal->findChildren(SymTagPublicSymbol, nullptr, nsNone, &pEnumPublic);
    if (SUCCEEDED(hr) && pEnumPublic) {
        IDiaSymbol* pPublicSymbol = nullptr;
        ULONG celt = 0;
        while (!IsCancelled() && SUCCEEDED(pEnumPublic->Next(1, &pPublicSymbol, &celt)) && celt == 1) {
            BOOL isFunction = FALSE;
            if (SUCCEEDED(pPublicSymbol->get_function(&isFunction)) && isFunction) {
                FunctionInfo funcInfo;
                funcInfo.name = GetSymbolName(pPublicSymbol);

                BSTR bstrUndecorated = nullptr;
                if (SUCCEEDED(pPublicSymbol->get_undecoratedName(&bstrUndecorated)) && bstrUndecorated) {
                    funcInfo.undecoratedName = bstrUndecorated;
                    SysFreeString(bstrUndecorated);
                }

                pPublicSymbol->get_relativeVirtualAddress(&funcInfo.rva);
                pPublicSymbol->get_virtualAddress(&funcInfo.virtualAddress);
                pPublicSymbol->get_length(&funcInfo.size);

                if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
                    const bool duplicate = funcInfo.rva != 0 &&
                        !knownFunctionRvas.insert(funcInfo.rva).second;
                    if (!duplicate) {
                        moduleInfo.functions.push_back(std::move(funcInfo));
                    }
                }
            }
            pPublicSymbol->Release();
        }
        pEnumPublic->Release();
    }
}

void PDBParser::ParseUdtSymbols(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    // UDT parsing is the long pole: each type needs members, bases, member
    // functions and virtual functions. Enumerate the global UDT list once to
    // learn the count, then have every worker open its own DIA session and walk
    // its own findChildren enumeration, taking every workerCount-th symbol.
    // IDiaSymbol objects stay inside the session that produced them, and each
    // symbol keeps a global ordinal so the merged order matches a serial walk.
    const ULONGLONG discoverStart = GetTickCount64();
    ULONG udtCount = 0;
    {
        IDiaEnumSymbols* enumSymbols = nullptr;
        HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &enumSymbols);
        if (FAILED(hr) || !enumSymbols) return;
        LONG count = 0;
        if (SUCCEEDED(enumSymbols->get_Count(&count)) && count > 0) {
            udtCount = static_cast<ULONG>(count);
        }
        enumSymbols->Release();
    }

    if (udtCount == 0) return;

    if (m_stageTimingCallback) {
        m_stageTimingCallback(L"udt-discover=" +
            std::to_wstring(GetTickCount64() - discoverStart) + L"ms/" +
            std::to_wstring(udtCount) + L"symbols");
    }

    struct TaggedClass {
        size_t order = 0;
        ClassInfo info;
    };

    unsigned int systemThreads = std::thread::hardware_concurrency();
    unsigned int requested = systemThreads > 0 ? systemThreads : 8;
    unsigned int workerCount = (std::min)(requested, static_cast<unsigned int>(6));
    if (workerCount < 1) workerCount = 1;
    if (udtCount < 64) workerCount = 1;
    else if (udtCount < 512) workerCount = (std::min)(workerCount, 2u);

    struct TaggedClassSpan {
        size_t order = 0;
        std::vector<TaggedClass> items;
    };
    std::vector<TaggedClassSpan> perWorkerResults(workerCount);
    std::vector<std::atomic<size_t>> perWorkerCount(workerCount);
    for (auto& counter : perWorkerCount) counter.store(0);
    std::vector<ULONGLONG> perWorkerMs(workerCount, 0);
    std::vector<bool> workerFailed(workerCount, false);
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int worker = 0; worker < workerCount; ++worker) {
        workers.emplace_back([this, &udtCount, &perWorkerResults, &perWorkerCount,
                              &perWorkerMs, &workerFailed, worker, workerCount]() {
            HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool comInitialized = SUCCEEDED(comResult);
            if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                workerFailed[worker] = true;
                return;
            }

            IDiaDataSource* dataSource = nullptr;
            IDiaSession* session = nullptr;
            IDiaSymbol* globalScope = nullptr;
            std::wstring errorMessage;
            if (!OpenSession(m_pdbPath, &dataSource, &session, &globalScope, &errorMessage)) {
                workerFailed[worker] = true;
                if (dataSource) dataSource->Release();
                if (session) session->Release();
                if (globalScope) globalScope->Release();
                if (comInitialized) CoUninitialize();
                return;
            }

            IDiaEnumSymbols* enumSymbols = nullptr;
            if (FAILED(globalScope->findChildren(SymTagUDT, nullptr, nsNone, &enumSymbols)) ||
                !enumSymbols) {
                workerFailed[worker] = true;
                globalScope->Release();
                session->Release();
                dataSource->Release();
                if (comInitialized) CoUninitialize();
                return;
            }

            const ULONGLONG workerStart = GetTickCount64();
            TaggedClassSpan& span = perWorkerResults[worker];
            span.order = worker;
            std::vector<TaggedClass>& local = span.items;
            local.reserve(udtCount / workerCount + 1);
            size_t worked = 0;
            size_t ordinal = 0;
            IDiaSymbol* symbol = nullptr;
            ULONG fetched = 0;
            while (!IsCancelled()) {
                const HRESULT nextResult = enumSymbols->Next(1, &symbol, &fetched);
                if (FAILED(nextResult) || fetched != 1 || !symbol) break;
                const size_t globalOrdinal = ordinal++;
                if (globalOrdinal % workerCount != worker) {
                    symbol->Release();
                    continue;
                }
                DWORD udtKind = 0;
                symbol->get_udtKind(&udtKind);
                if (udtKind == UdtClass || udtKind == UdtStruct || udtKind == UdtUnion) {
                    ClassInfo classInfo{};
                    classInfo.isStruct = udtKind == UdtStruct;
                    classInfo.isUnion = udtKind == UdtUnion;
                    ParseClassDetails(symbol, classInfo);
                    if (!IsAnonymousTypeName(classInfo.name)) {
                        TaggedClass tagged;
                        tagged.order = globalOrdinal;
                        tagged.info = std::move(classInfo);
                        local.push_back(std::move(tagged));
                        ++worked;
                    }
                }
                symbol->Release();
            }

            perWorkerCount[worker].store(worked);
            perWorkerMs[worker] = GetTickCount64() - workerStart;
            enumSymbols->Release();
            globalScope->Release();
            session->Release();
            dataSource->Release();
            if (comInitialized) CoUninitialize();
        });
    }

    for (auto& worker : workers) worker.join();

    bool allWorkersFailed = true;
    for (unsigned int worker = 0; worker < workerCount; ++worker) {
        if (!workerFailed[worker]) { allWorkersFailed = false; break; }
    }

    if (m_stageTimingCallback) {
        std::wstring detail = L"udt-workers=" + std::to_wstring(workerCount);
        for (unsigned int worker = 0; worker < workerCount; ++worker) {
            detail += L" w";
            detail += std::to_wstring(worker);
            detail += L":" + std::to_wstring(perWorkerCount[worker].load()) +
                L"/" + std::to_wstring(perWorkerMs[worker]) + L"ms";
        }
        m_stageTimingCallback(detail);
    }

    // If every worker failed to open a session, fall back to the original
    // single-session walk instead of silently returning an empty module.
    if (allWorkersFailed) {
        const ULONGLONG fallbackStart = GetTickCount64();
        ParseUdtSymbolsSingleSession(pGlobal, moduleInfo);
        if (m_stageTimingCallback) {
            m_stageTimingCallback(L"udt-fallback=" +
                std::to_wstring(GetTickCount64() - fallbackStart) + L"ms");
        }
        return;
    }

    // Workers each took every workerCount-th symbol, so interleaving results by
    // local position reproduces the original serial enumeration order.
    std::vector<size_t> workerCursor(workerCount, 0);
    size_t remaining = 0;
    for (unsigned int worker = 0; worker < workerCount; ++worker) {
        remaining += perWorkerResults[worker].items.size();
    }
    while (remaining > 0) {
        for (unsigned int worker = 0; worker < workerCount; ++worker) {
            auto& local = perWorkerResults[worker].items;
            if (workerCursor[worker] >= local.size()) continue;
            ClassInfo& info = local[workerCursor[worker]++].info;
            --remaining;
            if (info.isUnion) moduleInfo.unions.push_back(std::move(info));
            else if (info.isStruct) moduleInfo.structs.push_back(std::move(info));
            else moduleInfo.classes.push_back(std::move(info));
        }
    }
}

void PDBParser::ParseUdtSymbolsSingleSession(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* enumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &enumSymbols);
    if (FAILED(hr) || !enumSymbols) return;

    IDiaSymbol* symbol = nullptr;
    ULONG count = 0;
    while (!IsCancelled() && SUCCEEDED(enumSymbols->Next(1, &symbol, &count)) && count == 1) {
        std::wstring name = GetSymbolName(symbol);
        if (!IsAnonymousTypeName(name)) {
            DWORD udtKind = 0;
            symbol->get_udtKind(&udtKind);
            if (udtKind == UdtClass || udtKind == UdtStruct || udtKind == UdtUnion) {
                ClassInfo classInfo{};
                classInfo.isStruct = udtKind == UdtStruct;
                classInfo.isUnion = udtKind == UdtUnion;
                ParseClassDetails(symbol, classInfo);
                if (classInfo.isUnion) moduleInfo.unions.push_back(std::move(classInfo));
                else if (classInfo.isStruct) moduleInfo.structs.push_back(std::move(classInfo));
                else moduleInfo.classes.push_back(std::move(classInfo));
            }
        }
        symbol->Release();
    }
    enumSymbols->Release();
}

#if 0
void PDBParser::ParseClasses(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        DWORD udtKind = 0;
        pSymbol->get_udtKind(&udtKind);
        
        if (udtKind == UdtClass) {
            ClassInfo classInfo;
            classInfo.isStruct = false;
            classInfo.isUnion = false;
            ParseClassDetails(pSymbol, classInfo);
            moduleInfo.classes.push_back(classInfo);
        }
        
        pSymbol->Release();
    }

    pEnumSymbols->Release();
}

void PDBParser::ParseStructs(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        DWORD udtKind = 0;
        pSymbol->get_udtKind(&udtKind);
        
        if (udtKind == UdtStruct) {
            ClassInfo classInfo;
            classInfo.isStruct = true;
            classInfo.isUnion = false;
            ParseClassDetails(pSymbol, classInfo);
            moduleInfo.structs.push_back(classInfo);
        }
        
        pSymbol->Release();
    }

    pEnumSymbols->Release();
}

void PDBParser::ParseUnions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        DWORD udtKind = 0;
        pSymbol->get_udtKind(&udtKind);
        
        if (udtKind == UdtUnion) {
            ClassInfo classInfo;
            classInfo.isStruct = false;
            classInfo.isUnion = true;
            ParseClassDetails(pSymbol, classInfo);
            moduleInfo.unions.push_back(classInfo);
        }
        
        pSymbol->Release();
    }

    pEnumSymbols->Release();
}

#endif

void PDBParser::ParseEnums(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagEnum, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        EnumInfo enumInfo;
        enumInfo.name = GetSymbolName(pSymbol);
        
        IDiaSymbol* pType = nullptr;
        if (SUCCEEDED(pSymbol->get_type(&pType)) && pType) {
            enumInfo.underlyingType = GetTypeName(pType);
            pType->Release();
        }
        
        IDiaEnumSymbols* pEnumChildren = nullptr;
        if (SUCCEEDED(pSymbol->findChildren(SymTagData, nullptr, nsNone, &pEnumChildren)) && pEnumChildren) {
            IDiaSymbol* pChild = nullptr;
            ULONG celtChild = 0;
            while (!IsCancelled() && SUCCEEDED(pEnumChildren->Next(1, &pChild, &celtChild)) && celtChild == 1) {
                EnumValueInfo valueInfo;
                valueInfo.name = GetSymbolName(pChild);
                
                VARIANT varValue;
                VariantInit(&varValue);
                if (SUCCEEDED(pChild->get_value(&varValue))) {
                    if (varValue.vt == VT_I4) {
                        valueInfo.value = varValue.lVal;
                    } else if (varValue.vt == VT_I8) {
                        valueInfo.value = varValue.llVal;
                    } else if (varValue.vt == VT_UI4) {
                        valueInfo.value = varValue.ulVal;
                    } else if (varValue.vt == VT_UI8) {
                        valueInfo.value = varValue.ullVal;
                    } else if (varValue.vt == VT_I2) {
                        valueInfo.value = varValue.iVal;
                    } else if (varValue.vt == VT_UI2) {
                        valueInfo.value = varValue.uiVal;
                    }
                }
                VariantClear(&varValue);
                
                enumInfo.values.push_back(valueInfo);
                pChild->Release();
            }
            pEnumChildren->Release();
        }
        
        moduleInfo.enums.push_back(enumInfo);
        pSymbol->Release();
    }

    pEnumSymbols->Release();
}

void PDBParser::ParseClassDetails(IDiaSymbol* pClass, ClassInfo& classInfo) {
    classInfo.name = GetSymbolName(pClass);

    ULONGLONG length = 0;
    pClass->get_length(&length);
    classInfo.size = length;

    classInfo.alignment = 8;

    IDiaEnumSymbols* pEnumBase = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagBaseClass, nullptr, nsNone, &pEnumBase)) && pEnumBase) {
        IDiaSymbol* pBase = nullptr;
        ULONG celt = 0;
        while (!IsCancelled() && SUCCEEDED(pEnumBase->Next(1, &pBase, &celt)) && celt == 1) {
            BaseClassInfo baseInfo;
            baseInfo.name = GetSymbolName(pBase);

            BOOL isVirtual = FALSE;
            pBase->get_virtualBaseClass(&isVirtual);
            baseInfo.inheritanceType = isVirtual ? PDB_INHERITANCE_VIRTUAL : PDB_INHERITANCE_NORMAL;

            LONG offset = -1;
            if (FAILED(pBase->get_offset(&offset)) || offset < 0) offset = -1;
            baseInfo.offset = offset;

            DWORD access = 0;
            pBase->get_access(&access);
            baseInfo.access = GetAccessType(access);

            classInfo.baseClasses.push_back(baseInfo);
            pBase->Release();
        }
        pEnumBase->Release();
    }
    ParseMemberVariables(pClass, classInfo.members);
    IDiaEnumSymbols* pEnumFunc = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagFunction, nullptr, nsNone, &pEnumFunc)) && pEnumFunc) {
        IDiaSymbol* pFunc = nullptr;
        ULONG celt = 0;
        int vtableIndex = 0;
        while (!IsCancelled() && SUCCEEDED(pEnumFunc->Next(1, &pFunc, &celt)) && celt == 1) {
            std::wstring memberName = GetSymbolName(pFunc);
            classInfo.memberFunctions.push_back(memberName);
            BOOL isVirtual = FALSE;
            pFunc->get_virtual(&isVirtual);
            if (!isVirtual) {
                pFunc->Release();
                continue;
            }

            FunctionInfo functionInfo;
            ParseFunctionDetails(pFunc, functionInfo);
            functionInfo.name = std::move(memberName);
            functionInfo.isVirtual = isVirtual != FALSE;
            if (functionInfo.isVirtual) {
                VirtualFunctionInfo virtualInfo;
                virtualInfo.name = functionInfo.name;
                virtualInfo.returnType = functionInfo.returnType;
                virtualInfo.returnTypeRef = functionInfo.returnTypeRef;
                virtualInfo.parameters = functionInfo.parameters;
                virtualInfo.rva = functionInfo.rva;
                virtualInfo.virtualAddress = functionInfo.virtualAddress;
                virtualInfo.vtableIndex = vtableIndex++;
                DWORD access = 0;
                pFunc->get_access(&access);
                virtualInfo.access = GetAccessType(access);
                BOOL isPure = FALSE;
                virtualInfo.isPure = SUCCEEDED(pFunc->get_pure(&isPure)) && isPure;
                classInfo.virtualFunctions.push_back(std::move(virtualInfo));
            }
            pFunc->Release();
        }
        pEnumFunc->Release();
    }
}

void PDBParser::ParseMemberVariables(IDiaSymbol* owner, std::vector<MemberVariableInfo>& members, int depth) {
    if (!owner || depth > 32) return;
    IDiaEnumSymbols* enumData = nullptr;
    if (FAILED(owner->findChildren(SymTagData, nullptr, nsNone, &enumData)) || !enumData) return;

    IDiaSymbol* data = nullptr;
    ULONG count = 0;
    while (!IsCancelled() && SUCCEEDED(enumData->Next(1, &data, &count)) && count == 1) {
        MemberVariableInfo member;
        member.name = GetSymbolName(data);
        member.isAnonymous = IsAnonymousTypeName(member.name);
        if (member.isAnonymous) member.name.clear();

        IDiaSymbol* type = nullptr;
        if (SUCCEEDED(data->get_type(&type)) && type) {
            member.typeRef = BuildTypeRef(type, depth + 1);
            member.type = PDBHeaderGenerator::RenderDeclaration(member.typeRef, L"");
            ULONGLONG typeSize = 0;
            if (SUCCEEDED(type->get_length(&typeSize)) && typeSize > 0) {
                member.typeSize = typeSize;
                member.typeSizeKnown = true;
            }
            member.typeAlignment = InferTypeAlignment(type, member.typeSizeKnown ? member.typeSize : 0);
            member.typeAlignmentKnown = member.typeAlignment > 0;
            DWORD typeTag = SymTagNull;
            type->get_symTag(&typeTag);
            if (typeTag == SymTagUDT && IsAnonymousTypeName(GetSymbolName(type))) {
                member.isAnonymous = true;
                member.name.clear();
            }
            if (member.isAnonymous && typeTag == SymTagUDT) {
                DWORD udtKind = 0;
                type->get_udtKind(&udtKind);
                auto aggregate = std::make_shared<ClassInfo>();
                aggregate->isUnion = udtKind == UdtUnion;
                aggregate->isStruct = udtKind == UdtStruct;
                type->get_length(&aggregate->size);
                ParseMemberVariables(type, aggregate->members, depth + 1);
                member.anonymousType = std::move(aggregate);
            }
            type->Release();
        }

        LONG memberOffset = -1;
        DWORD dataKind = DataIsUnknown;
        const bool hasDataKind = SUCCEEDED(data->get_dataKind(&dataKind));
        DWORD locationType = LocIsNull;
        const bool hasLocation = SUCCEEDED(data->get_locationType(&locationType));
        member.isStatic = (hasDataKind &&
            (dataKind == DataIsStaticMember || dataKind == DataIsConstant)) ||
            (hasLocation && (locationType == LocIsStatic ||
                             locationType == LocIsTLS ||
                             locationType == LocIsConstant));

        const bool hasInstanceOffset = !member.isStatic && hasLocation &&
            locationType == LocIsThisRel &&
            SUCCEEDED(data->get_offset(&memberOffset)) && memberOffset >= 0;
        if (!hasInstanceOffset) memberOffset = -1;
        member.offset = memberOffset;
        DWORD access = 0;
        data->get_access(&access);
        member.access = GetAccessType(access);
        member.isBitfield = data->get_bitPosition(&member.bitPosition) == S_OK;
        ULONGLONG bitSize = 0;
        if (member.isBitfield && SUCCEEDED(data->get_length(&bitSize))) {
            member.bitSize = static_cast<DWORD>(bitSize);
        }
        members.push_back(std::move(member));
        data->Release();
    }
    enumData->Release();

    ReconstructMemberOffsetsIfNeeded(members);
}

bool PDBParser::IsAnonymousTypeName(const std::wstring& name) const {
    return name.empty() || name == L"anonymous" ||
           name.find(L"<anonymous") != std::wstring::npos ||
           name.find(L"<unnamed") != std::wstring::npos;
}

void PDBParser::ReconstructMemberOffsetsIfNeeded(std::vector<MemberVariableInfo>& members) {
    bool hasLayoutConflict = false;
    LONG previousKnownOffset = -1;
    for (const auto& member : members) {
        if (member.isStatic || member.offset < 0) continue;
        if (previousKnownOffset >= 0 && member.offset <= previousKnownOffset) {
            hasLayoutConflict = true;
            break;
        }
        previousKnownOffset = member.offset;
    }
    if (!hasLayoutConflict) return;

    LONG nextOffset = -1;
    for (size_t index = 0; index < members.size(); ++index) {
        MemberVariableInfo& member = members[index];
        if (member.isStatic || member.offset < 0) continue;

        const LONG reportedOffset = member.offset;
        if (nextOffset >= 0 && reportedOffset < nextOffset) {
            if (!member.typeSizeKnown || member.typeSize == 0) continue;

            ULONGLONG alignment = member.typeAlignmentKnown ? member.typeAlignment : 1;
            if (alignment == 0) alignment = 1;
            const ULONGLONG mask = alignment - 1;
            const ULONGLONG aligned = (nextOffset + mask) & ~mask;
            if (aligned > static_cast<ULONGLONG>(LONG_MAX)) continue;

            member.offset = static_cast<LONG>(aligned);
            member.offsetReconstructed = true;
            member.typeAlignment = alignment;
        }

        if (!member.typeSizeKnown || member.typeSize == 0) {
            nextOffset = -1;
            continue;
        }
        const ULONGLONG end = static_cast<ULONGLONG>(member.offset) + member.typeSize;
        nextOffset = end <= static_cast<ULONGLONG>(LONG_MAX) ? static_cast<LONG>(end) : -1;
    }
}

ULONGLONG PDBParser::InferTypeAlignment(IDiaSymbol* pType, ULONGLONG typeSize) const {
    if (!pType) return typeSize == 0 ? 0 : (std::min)(typeSize, 8ULL);

    DWORD tag = SymTagNull;
    pType->get_symTag(&tag);
    if (tag == SymTagPointerType) return 4;
    if (tag == SymTagArrayType || tag == SymTagFunctionType) return 4;
    if (tag == SymTagEnum) {
        ULONGLONG enumSize = 0;
        if (SUCCEEDED(pType->get_length(&enumSize)) && enumSize > 0) {
            return (std::min)(enumSize, 8ULL);
        }
        return 4;
    }
    if (tag == SymTagBaseType) {
        return typeSize == 0 ? 1 : (std::min)(typeSize, 8ULL);
    }
    if (tag == SymTagUDT) {
        DWORD udtKind = UdtStruct;
        pType->get_udtKind(&udtKind);
        if (udtKind == UdtUnion) return typeSize == 0 ? 1 : (std::min)(typeSize, 8ULL);
        return typeSize == 0 ? 1 : (std::min)(typeSize, 8ULL);
    }
    return typeSize == 0 ? 1 : (std::min)(typeSize, 8ULL);
}

namespace {
void ReadTypeQualifiers(IDiaSymbol* type, TypeRef& result) {
    BOOL value = FALSE;
    if (SUCCEEDED(type->get_constType(&value)) && value) result.isConst = true;
    value = FALSE;
    if (SUCCEEDED(type->get_volatileType(&value)) && value) result.isVolatile = true;
    value = FALSE;
    if (SUCCEEDED(type->get_RValueReference(&value)) && value) result.isRValueReference = true;
}
}

TypeRef PDBParser::BuildTypeRef(IDiaSymbol* pType, int depth) {
    TypeRef result;
    if (!pType || depth > 64) {
        result.kind = TypeRefKind::Named;
        return result;
    }

    DWORD tag = SymTagNull;
    pType->get_symTag(&tag);
    ReadTypeQualifiers(pType, result);
    if (tag == SymTagPointerType) {
        BOOL isReference = FALSE;
        pType->get_reference(&isReference);
        result.kind = isReference ? TypeRefKind::Reference : TypeRefKind::Pointer;
        IDiaSymbol* pointee = nullptr;
        if (SUCCEEDED(pType->get_type(&pointee)) && pointee) {
            result.child = std::make_shared<TypeRef>(BuildTypeRef(pointee, depth + 1));
            pointee->Release();
        }
        return result;
    }
    if (tag == SymTagArrayType) {
        result.kind = TypeRefKind::Array;
        DWORD count = 0;
        result.hasKnownArrayCount = SUCCEEDED(pType->get_count(&count));
        result.arrayCount = count;
        IDiaSymbol* element = nullptr;
        if (SUCCEEDED(pType->get_type(&element)) && element) {
            result.child = std::make_shared<TypeRef>(BuildTypeRef(element, depth + 1));
            element->Release();
        }
        return result;
    }
    if (tag == SymTagFunctionType) {
        result.kind = TypeRefKind::Function;
        DWORD callingConvention = 0;
        if (SUCCEEDED(pType->get_callingConvention(&callingConvention))) {
            result.callingConvention = GetCallingConvention(callingConvention);
        }
        IDiaSymbol* returnType = nullptr;
        if (SUCCEEDED(pType->get_type(&returnType)) && returnType) {
            result.child = std::make_shared<TypeRef>(BuildTypeRef(returnType, depth + 1));
            returnType->Release();
        }

        IDiaEnumSymbols* args = nullptr;
        if (SUCCEEDED(pType->findChildren(SymTagFunctionArgType, nullptr, nsNone, &args)) && args) {
            IDiaSymbol* arg = nullptr;
            ULONG count = 0;
            while (SUCCEEDED(args->Next(1, &arg, &count)) && count == 1) {
                IDiaSymbol* argType = nullptr;
                if (SUCCEEDED(arg->get_type(&argType)) && argType) {
                    result.functionParameters.push_back(BuildTypeRef(argType, depth + 1));
                    argType->Release();
                }
                arg->Release();
            }
            args->Release();
        }
        return result;
    }

    result.kind = TypeRefKind::Named;
    if (tag == SymTagBaseType) {
        result.name = GetTypeName(pType);
    } else {
        result.name = GetSymbolName(pType);
        if (result.name.empty()) result.name = GetTypeName(pType);
    }
    return result;
}

#if 0
void PDBParser::ParseVirtualFunctions(IDiaSymbol* pClass, ClassInfo& classInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagFunction, nullptr, nsNone, &pEnumSymbols)) && pEnumSymbols) {
        IDiaSymbol* pSymbol = nullptr;
        ULONG celt = 0;
        int vtableIndex = 0;
        
        while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
            BOOL isVirtual = FALSE;
            if (SUCCEEDED(pSymbol->get_virtual(&isVirtual)) && isVirtual) {
                VirtualFunctionInfo vfuncInfo;
                vfuncInfo.name = GetSymbolName(pSymbol);
                vfuncInfo.vtableIndex = vtableIndex++;
                
                IDiaSymbol* pType = nullptr;
                if (SUCCEEDED(pSymbol->get_type(&pType)) && pType) {
                    IDiaSymbol* pReturnType = nullptr;
                    if (SUCCEEDED(pType->get_type(&pReturnType)) && pReturnType) {
                        vfuncInfo.returnType = GetTypeName(pReturnType);
                        pReturnType->Release();
                    }
                    pType->Release();
                }
                
                ParseParameters(pSymbol, vfuncInfo.parameters);
                
                DWORD rva = 0;
                if (SUCCEEDED(pSymbol->get_relativeVirtualAddress(&rva))) {
                    vfuncInfo.rva = rva;
                }
                
                ULONGLONG va = 0;
                if (SUCCEEDED(pSymbol->get_virtualAddress(&va))) {
                    vfuncInfo.virtualAddress = va;
                }
                
                classInfo.virtualFunctions.push_back(vfuncInfo);
            }
            pSymbol->Release();
        }
        pEnumSymbols->Release();
    }
}

#endif

void PDBParser::ParseFunctionDetails(IDiaSymbol* pFunction, FunctionInfo& funcInfo) {
    funcInfo.name = GetSymbolName(pFunction);
    funcInfo.undecoratedName = GetUndecoratedName(pFunction);

    IDiaSymbol* pType = nullptr;
    if (SUCCEEDED(pFunction->get_type(&pType)) && pType) {
        IDiaSymbol* pReturnType = nullptr;
        if (SUCCEEDED(pType->get_type(&pReturnType)) && pReturnType) {
            funcInfo.returnTypeRef = BuildTypeRef(pReturnType);
            funcInfo.returnType = PDBHeaderGenerator::RenderDeclaration(funcInfo.returnTypeRef, L"");
            pReturnType->Release();
        }

        DWORD cc = 0;
        if (SUCCEEDED(pType->get_callingConvention(&cc))) {
            funcInfo.callingConvention = GetCallingConvention(cc);
        }

        pType->Release();
    }

    ParseParameters(pFunction, funcInfo.parameters);

    DWORD rva = 0;
    if (SUCCEEDED(pFunction->get_relativeVirtualAddress(&rva))) {
        funcInfo.rva = rva;
    }

    ULONGLONG va = 0;
    if (SUCCEEDED(pFunction->get_virtualAddress(&va))) {
        funcInfo.virtualAddress = va;
    }

    ULONGLONG length = 0;
    if (SUCCEEDED(pFunction->get_length(&length))) {
        funcInfo.size = length;
    }

    BOOL isStatic = FALSE;
    if (SUCCEEDED(pFunction->get_isStatic(&isStatic))) {
        funcInfo.isStatic = isStatic != FALSE;
    }

    BOOL isVirtual = FALSE;
    if (SUCCEEDED(pFunction->get_virtual(&isVirtual))) {
        funcInfo.isVirtual = isVirtual != FALSE;
    }

    funcInfo.isMemberFunction = false;
    funcInfo.className = L"";

    IDiaSymbol* pClassParent = nullptr;
    if (SUCCEEDED(pFunction->get_classParent(&pClassParent)) && pClassParent) {
        funcInfo.isMemberFunction = true;
        funcInfo.className = GetSymbolName(pClassParent);
        pClassParent->Release();
    }
}

void PDBParser::ParseParameters(IDiaSymbol* pFunction, std::vector<ParameterInfo>& params) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    if (SUCCEEDED(pFunction->findChildren(SymTagData, nullptr, nsNone, &pEnumSymbols)) && pEnumSymbols) {
        IDiaSymbol* pData = nullptr;
        ULONG celt = 0;
        while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pData, &celt)) && celt == 1) {
            DWORD dataKind = 0;
            if (SUCCEEDED(pData->get_dataKind(&dataKind)) && dataKind == DataIsParam) {
                ParameterInfo paramInfo;
                paramInfo.name = GetSymbolName(pData);
                
                if (paramInfo.name.empty()) {
                    static int unnamedParamIndex = 0;
                    std::wstringstream ss;
                    ss << L"unnamedParam" << unnamedParamIndex++;
                    paramInfo.name = ss.str();
                }

                IDiaSymbol* pType = nullptr;
                if (SUCCEEDED(pData->get_type(&pType)) && pType) {
                    paramInfo.typeRef = BuildTypeRef(pType);
                    paramInfo.type = PDBHeaderGenerator::RenderDeclaration(paramInfo.typeRef, L"");
                    pType->Release();
                }

                paramInfo.hasDefaultValue = false;
                params.push_back(paramInfo);
            }
            pData->Release();
        }
        pEnumSymbols->Release();
    }

    if (params.empty()) {
        IDiaEnumSymbols* pEnumArgs = nullptr;
        if (SUCCEEDED(pFunction->findChildren(SymTagFunctionArgType, nullptr, nsNone, &pEnumArgs)) && pEnumArgs) {
            IDiaSymbol* pArg = nullptr;
            ULONG celt = 0;
            int index = 0;
            while (!IsCancelled() && SUCCEEDED(pEnumArgs->Next(1, &pArg, &celt)) && celt == 1) {
                ParameterInfo paramInfo;
                
                std::wstringstream ss;
                ss << L"param" << index++;
                paramInfo.name = ss.str();

                IDiaSymbol* pType = nullptr;
                if (SUCCEEDED(pArg->get_type(&pType)) && pType) {
                    paramInfo.typeRef = BuildTypeRef(pType);
                    paramInfo.type = PDBHeaderGenerator::RenderDeclaration(paramInfo.typeRef, L"");
                    pType->Release();
                }

                paramInfo.hasDefaultValue = false;
                params.push_back(paramInfo);
                pArg->Release();
            }
            pEnumArgs->Release();
        }
    }
}

std::wstring PDBParser::GetSymbolName(IDiaSymbol* pSymbol) {
    if (!pSymbol) return L"";

    BSTR bstrName = nullptr;
    HRESULT hr = pSymbol->get_name(&bstrName);
    if (SUCCEEDED(hr) && bstrName) {
        std::wstring name = bstrName;
        SysFreeString(bstrName);
        return name;
    }
    return L"";
}

std::wstring PDBParser::GetUndecoratedName(IDiaSymbol* pSymbol) {
    if (!pSymbol) return L"";

    BSTR bstrName = nullptr;
    HRESULT hr = pSymbol->get_undecoratedName(&bstrName);
    if (SUCCEEDED(hr) && bstrName) {
        std::wstring name = bstrName;
        SysFreeString(bstrName);
        return name;
    }
    return GetSymbolName(pSymbol);
}

std::wstring PDBParser::GetTypeName(IDiaSymbol* pType) {
    if (!pType) return L"";

    DWORD symTag = 0;
    pType->get_symTag(&symTag);

    if (symTag == SymTagBaseType) {
        DWORD baseType = 0;
        pType->get_baseType(&baseType);
        
        ULONGLONG length = 0;
        pType->get_length(&length);
        
        switch (baseType) {
            case btVoid:
                return L"void";
            case btChar:
                if (length == 1) return L"char";
                break;
            case btWChar:
                return L"wchar_t";
            case btInt:
                if (length == 1) return L"char";
                else if (length == 2) return L"short";
                else if (length == 4) return L"int";
                else if (length == 8) return L"long long";
                break;
            case btUInt:
                if (length == 1) return L"unsigned char";
                else if (length == 2) return L"unsigned short";
                else if (length == 4) return L"unsigned int";
                else if (length == 8) return L"unsigned long long";
                break;
            case btFloat:
                if (length == 4) return L"float";
                else if (length == 8) return L"double";
                else if (length == 10 || length == 16) return L"long double";
                break;
            case btBool:
                return L"bool";
            case btLong:
                return L"long";
            case btULong:
                return L"unsigned long";
            case btBSTR:
                return L"BSTR";
            case btHresult:
                return L"HRESULT";
            case btChar16:
                return L"char16_t";
            case btChar32:
                return L"char32_t";
        }
    }
    else if (symTag == SymTagTypedef) {
        return GetSymbolName(pType);
    }

    return GetSymbolName(pType);
}

CallingConvention PDBParser::GetCallingConvention(DWORD cc) {
    switch (cc) {
        case CV_CALL_NEAR_C: return PDB_CALL_CDECL;
        case CV_CALL_NEAR_STD: return PDB_CALL_STDCALL;
        case CV_CALL_THISCALL: return PDB_CALL_THISCALL;
        case CV_CALL_NEAR_FAST: return PDB_CALL_FASTCALL;
        case CV_CALL_NEAR_VECTOR: return PDB_CALL_VECTORCALL;
        default: return PDB_CALL_UNKNOWN;
    }
}

AccessType PDBParser::GetAccessType(DWORD access) {
    switch (access) {
        case CV_public: return PDB_ACCESS_PUBLIC;
        case CV_protected: return PDB_ACCESS_PROTECTED;
        case CV_private: return PDB_ACCESS_PRIVATE;
        default: return PDB_ACCESS_UNKNOWN;
    }
}

std::wstring PDBParser::AccessTypeToString(AccessType access) {
    switch (access) {
        case PDB_ACCESS_PUBLIC: return L"public";
        case PDB_ACCESS_PROTECTED: return L"protected";
        case PDB_ACCESS_PRIVATE: return L"private";
        default: return L"unknown";
    }
}

std::wstring PDBParser::CallingConventionToString(CallingConvention cc) {
    switch (cc) {
        case PDB_CALL_CDECL: return L"__cdecl";
        case PDB_CALL_STDCALL: return L"__stdcall";
        case PDB_CALL_THISCALL: return L"__thiscall";
        case PDB_CALL_FASTCALL: return L"__fastcall";
        case PDB_CALL_VECTORCALL: return L"__vectorcall";
        default: return L"unknown";
    }
}

std::wstring PDBParser::GenerateFunctionSignature(const FunctionInfo& funcInfo) {
    std::wostringstream ss;
    
    if (!funcInfo.returnType.empty()) {
        ss << funcInfo.returnType << L" ";
    }
    
    std::wstring functionName = funcInfo.undecoratedName.empty() ? funcInfo.name : funcInfo.undecoratedName;
    size_t pos = functionName.find(L'(');
    if (pos != std::wstring::npos) {
        functionName = functionName.substr(0, pos);
    }
    
    if (funcInfo.isMemberFunction && !funcInfo.className.empty()) {
        ss << funcInfo.className << L"::";
    }
    
    ss << functionName << L"(";
    
    for (size_t i = 0; i < funcInfo.parameters.size(); ++i) {
        if (i > 0) {
            ss << L", ";
        }
        if (funcInfo.parameters[i].typeRef.kind == TypeRefKind::Named && funcInfo.parameters[i].typeRef.name.empty()) {
            ss << funcInfo.parameters[i].name;
        } else {
            ss << PDBHeaderGenerator::RenderDeclaration(funcInfo.parameters[i].typeRef, funcInfo.parameters[i].name);
        }
    }
    
    ss << L")";
    
    return ss.str();
}

void PDBParser::ParseGlobalVariables(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagData, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        DWORD dataKind = 0;
        if (SUCCEEDED(pSymbol->get_dataKind(&dataKind)) && dataKind == DataIsGlobal) {
            GlobalVariableInfo varInfo;
            varInfo.name = GetSymbolName(pSymbol);

            IDiaSymbol* pType = nullptr;
            if (SUCCEEDED(pSymbol->get_type(&pType)) && pType) {
                varInfo.typeRef = BuildTypeRef(pType);
                varInfo.type = PDBHeaderGenerator::RenderDeclaration(varInfo.typeRef, L"");
                pType->Release();
            }

            pSymbol->get_relativeVirtualAddress(&varInfo.rva);
            pSymbol->get_virtualAddress(&varInfo.virtualAddress);
            pSymbol->get_length(&varInfo.size);

            if (!varInfo.name.empty()) {
                moduleInfo.globalVariables.push_back(varInfo);
            }
        }
        pSymbol->Release();
    }

    pEnumSymbols->Release();
}
