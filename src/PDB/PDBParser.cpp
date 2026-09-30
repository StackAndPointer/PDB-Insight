#include "PDBParser.h"
#include "PDBHeaderGenerator.h"
#include "diaCreate.h"
#include "cvConst.h"
#include <comdef.h>
#include <sstream>
#include <iomanip>
#include <shlwapi.h>
#include <unordered_set>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "shlwapi.lib")



PDBParser::PDBParser()
    : m_pDataSource(nullptr)
    , m_pSession(nullptr)
    , m_pGlobal(nullptr)
{
    Initialize();
}

PDBParser::~PDBParser() {
    Cleanup();
    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
}

bool PDBParser::Initialize() {
    HRESULT hr = CoInitialize(nullptr);
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

bool PDBParser::LoadPDB(const std::wstring& pdbPath) {
    Cleanup();

    if (IsCancelled()) return false;
    HRESULT hr = NoRegCoCreate(L"msdia140.dll", _uuidof(DiaSource), _uuidof(IDiaDataSource), (void**)&m_pDataSource);
    
    if (FAILED(hr)) {
        const wchar_t* debugInfo = GetDebugInfo();
        std::wstring errorMsg = L"Failed to create DIA data source:\n";
        errorMsg += debugInfo;
        m_lastError = errorMsg;
        return false;
    }

    hr = m_pDataSource->loadDataFromPdb(pdbPath.c_str());
    if (IsCancelled()) return false;
    if (FAILED(hr)) {
        m_lastError = L"Failed to load PDB file";
        return false;
    }

    hr = m_pDataSource->openSession(&m_pSession);
    if (IsCancelled()) return false;
    if (FAILED(hr)) {
        m_lastError = L"Failed to open session";
        return false;
    }

    hr = m_pSession->get_globalScope(&m_pGlobal);
    if (IsCancelled()) return false;
    if (FAILED(hr)) {
        m_lastError = L"Failed to get global scope";
        return false;
    }

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

    if (!ReportProgress(0, L"Parsing functions...")) return moduleInfo;
    ParseFunctions(m_pGlobal, moduleInfo);

    if (!ReportProgress(20, L"Parsing global variables...")) return moduleInfo;
    ParseGlobalVariables(m_pGlobal, moduleInfo);

    if (!ReportProgress(40, L"Parsing classes...")) return moduleInfo;

    ParseUdtSymbols(m_pGlobal, moduleInfo);
    ParseEnums(m_pGlobal, moduleInfo);

    if (!ReportProgress(100, L"Complete")) return moduleInfo;
    return moduleInfo;
}

void PDBParser::ParseFunctions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagFunction, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    std::unordered_set<ULONGLONG> knownFunctionRvas;
    ULONG celt = 0;
    while (!IsCancelled() && SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        FunctionInfo funcInfo;
        ParseFunctionDetails(pSymbol, funcInfo);
        funcInfo.displaySignature = GenerateFunctionSignature(funcInfo);
        
        if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
            moduleInfo.functions.push_back(funcInfo);
            if (funcInfo.rva != 0) knownFunctionRvas.insert(funcInfo.rva);
        }
        
        pSymbol->Release();
    }

    pEnumSymbols->Release();
    
    IDiaEnumSymbols* pEnumPublic = nullptr;
    hr = pGlobal->findChildren(SymTagPublicSymbol, nullptr, nsNone, &pEnumPublic);
    if (SUCCEEDED(hr) && pEnumPublic) {
        IDiaSymbol* pPublicSymbol = nullptr;
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
                funcInfo.displaySignature = GenerateFunctionSignature(funcInfo);
                
                if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
                    const bool duplicate = funcInfo.rva != 0 && !knownFunctionRvas.insert(funcInfo.rva).second;
                    if (!duplicate) {
                        moduleInfo.functions.push_back(funcInfo);
                    }
                }
            }
            pPublicSymbol->Release();
        }
        pEnumPublic->Release();
    }
}

void PDBParser::ParseUdtSymbols(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
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
            FunctionInfo functionInfo;
            ParseFunctionDetails(pFunc, functionInfo);
            classInfo.memberFunctions.push_back(functionInfo.name);
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
