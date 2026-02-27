#include "PDBParser.h"
#include "diaCreate.h"
#include "cvConst.h"
#include <comdef.h>
#include <sstream>
#include <iomanip>
#include <shlwapi.h>

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
}

bool PDBParser::Initialize() {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        m_lastError = L"Failed to initialize COM";
        return false;
    }
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

bool PDBParser::LoadPDB(const std::wstring& pdbPath) {
    Cleanup();

    HRESULT hr = NoRegCoCreate(L"msdia140.dll", _uuidof(DiaSource), _uuidof(IDiaDataSource), (void**)&m_pDataSource);
    
    if (FAILED(hr)) {
        const wchar_t* debugInfo = GetDebugInfo();
        std::wstring errorMsg = L"Failed to create DIA data source:\n";
        errorMsg += debugInfo;
        m_lastError = errorMsg;
        return false;
    }

    hr = m_pDataSource->loadDataFromPdb(pdbPath.c_str());
    if (FAILED(hr)) {
        m_lastError = L"Failed to load PDB file";
        return false;
    }

    hr = m_pDataSource->openSession(&m_pSession);
    if (FAILED(hr)) {
        m_lastError = L"Failed to open session";
        return false;
    }

    hr = m_pSession->get_globalScope(&m_pGlobal);
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

    if (m_progressCallback) {
        m_progressCallback(0, L"Parsing functions...");
    }
    ParseFunctions(m_pGlobal, moduleInfo);

    if (m_progressCallback) {
        m_progressCallback(20, L"Parsing global variables...");
    }
    ParseGlobalVariables(m_pGlobal, moduleInfo);

    if (m_progressCallback) {
        m_progressCallback(40, L"Parsing classes...");
    }
    ParseClasses(m_pGlobal, moduleInfo);

    if (m_progressCallback) {
        m_progressCallback(66, L"Parsing structs and unions...");
    }
    ParseStructs(m_pGlobal, moduleInfo);
    ParseUnions(m_pGlobal, moduleInfo);
    ParseEnums(m_pGlobal, moduleInfo);

    if (m_progressCallback) {
        m_progressCallback(100, L"Complete");
    }

    return moduleInfo;
}

void PDBParser::ParseFunctions(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagFunction, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        FunctionInfo funcInfo;
        ParseFunctionDetails(pSymbol, funcInfo);
        
        if (!funcInfo.name.empty() || !funcInfo.undecoratedName.empty()) {
            moduleInfo.functions.push_back(funcInfo);
        }
        
        pSymbol->Release();
    }

    pEnumSymbols->Release();
    
    IDiaEnumSymbols* pEnumPublic = nullptr;
    hr = pGlobal->findChildren(SymTagPublicSymbol, nullptr, nsNone, &pEnumPublic);
    if (SUCCEEDED(hr) && pEnumPublic) {
        IDiaSymbol* pPublicSymbol = nullptr;
        while (SUCCEEDED(pEnumPublic->Next(1, &pPublicSymbol, &celt)) && celt == 1) {
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
                    bool found = false;
                    for (const auto& existing : moduleInfo.functions) {
                        if (existing.rva == funcInfo.rva && existing.rva != 0) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        moduleInfo.functions.push_back(funcInfo);
                    }
                }
            }
            pPublicSymbol->Release();
        }
        pEnumPublic->Release();
    }
}

void PDBParser::ParseClasses(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagUDT, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
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
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
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
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
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

void PDBParser::ParseEnums(IDiaSymbol* pGlobal, ModuleInfo& moduleInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    HRESULT hr = pGlobal->findChildren(SymTagEnum, nullptr, nsNone, &pEnumSymbols);
    if (FAILED(hr) || !pEnumSymbols) {
        return;
    }

    IDiaSymbol* pSymbol = nullptr;
    ULONG celt = 0;
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
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
            while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celtChild)) && celtChild == 1) {
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
        while (SUCCEEDED(pEnumBase->Next(1, &pBase, &celt)) && celt == 1) {
            BaseClassInfo baseInfo;
            baseInfo.name = GetSymbolName(pBase);

            BOOL isVirtual = FALSE;
            pBase->get_virtualBaseClass(&isVirtual);
            baseInfo.inheritanceType = isVirtual ? PDB_INHERITANCE_VIRTUAL : PDB_INHERITANCE_NORMAL;

            LONG offset = 0;
            pBase->get_offset(&offset);
            baseInfo.offset = offset;

            DWORD access = 0;
            pBase->get_access(&access);
            baseInfo.access = GetAccessType(access);

            classInfo.baseClasses.push_back(baseInfo);
            pBase->Release();
        }
        pEnumBase->Release();
    }

    IDiaEnumSymbols* pEnumData = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagData, nullptr, nsNone, &pEnumData)) && pEnumData) {
        IDiaSymbol* pData = nullptr;
        ULONG celt = 0;
        while (SUCCEEDED(pEnumData->Next(1, &pData, &celt)) && celt == 1) {
            MemberVariableInfo memberInfo;
            memberInfo.name = GetSymbolName(pData);

            IDiaSymbol* pType = nullptr;
            if (SUCCEEDED(pData->get_type(&pType)) && pType) {
                memberInfo.type = GetTypeName(pType);
                pType->Release();
            }

            LONG offset = 0;
            pData->get_offset(&offset);
            memberInfo.offset = offset;

            DWORD access = 0;
            pData->get_access(&access);
            memberInfo.access = GetAccessType(access);

            DWORD bitPos = 0;
            pData->get_bitPosition(&bitPos);
            memberInfo.bitPosition = bitPos;

            ULONGLONG bitSize = 0;
            pData->get_length(&bitSize);
            memberInfo.bitSize = (DWORD)bitSize;

            classInfo.members.push_back(memberInfo);
            pData->Release();
        }
        pEnumData->Release();
    }

    IDiaEnumSymbols* pEnumFunc = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagFunction, nullptr, nsNone, &pEnumFunc)) && pEnumFunc) {
        IDiaSymbol* pFunc = nullptr;
        ULONG celt = 0;
        while (SUCCEEDED(pEnumFunc->Next(1, &pFunc, &celt)) && celt == 1) {
            classInfo.memberFunctions.push_back(GetSymbolName(pFunc));
            pFunc->Release();
        }
        pEnumFunc->Release();
    }

    ParseVirtualFunctions(pClass, classInfo);
}

void PDBParser::ParseVirtualFunctions(IDiaSymbol* pClass, ClassInfo& classInfo) {
    IDiaEnumSymbols* pEnumSymbols = nullptr;
    if (SUCCEEDED(pClass->findChildren(SymTagFunction, nullptr, nsNone, &pEnumSymbols)) && pEnumSymbols) {
        IDiaSymbol* pSymbol = nullptr;
        ULONG celt = 0;
        int vtableIndex = 0;
        
        while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
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

void PDBParser::ParseFunctionDetails(IDiaSymbol* pFunction, FunctionInfo& funcInfo) {
    funcInfo.name = GetSymbolName(pFunction);
    funcInfo.undecoratedName = GetUndecoratedName(pFunction);

    IDiaSymbol* pType = nullptr;
    if (SUCCEEDED(pFunction->get_type(&pType)) && pType) {
        IDiaSymbol* pReturnType = nullptr;
        if (SUCCEEDED(pType->get_type(&pReturnType)) && pReturnType) {
            funcInfo.returnType = GetTypeName(pReturnType);
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
        while (SUCCEEDED(pEnumSymbols->Next(1, &pData, &celt)) && celt == 1) {
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
                    paramInfo.type = GetTypeName(pType);
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
            while (SUCCEEDED(pEnumArgs->Next(1, &pArg, &celt)) && celt == 1) {
                ParameterInfo paramInfo;
                
                std::wstringstream ss;
                ss << L"param" << index++;
                paramInfo.name = ss.str();

                IDiaSymbol* pType = nullptr;
                if (SUCCEEDED(pArg->get_type(&pType)) && pType) {
                    paramInfo.type = GetTypeName(pType);
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

    if (symTag == SymTagPointerType) {
        IDiaSymbol* pBaseType = nullptr;
        if (SUCCEEDED(pType->get_type(&pBaseType)) && pBaseType) {
            std::wstring baseName = GetTypeName(pBaseType);
            pBaseType->Release();
            return baseName + L"*";
        }
    }
    else if (symTag == SymTagArrayType) {
        IDiaSymbol* pBaseType = nullptr;
        if (SUCCEEDED(pType->get_type(&pBaseType)) && pBaseType) {
            std::wstring baseName = GetTypeName(pBaseType);
            pBaseType->Release();
            
            DWORD count = 0;
            pType->get_count(&count);
            std::wstringstream ss;
            ss << baseName << L"[" << count << L"]";
            return ss.str();
        }
    }
    else if (symTag == SymTagBaseType) {
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
        if (!funcInfo.parameters[i].type.empty()) {
            ss << funcInfo.parameters[i].type;
            if (!funcInfo.parameters[i].name.empty()) {
                ss << L" " << funcInfo.parameters[i].name;
            }
        } else if (!funcInfo.parameters[i].name.empty()) {
            ss << funcInfo.parameters[i].name;
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
    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1) {
        DWORD dataKind = 0;
        if (SUCCEEDED(pSymbol->get_dataKind(&dataKind)) && dataKind == DataIsGlobal) {
            GlobalVariableInfo varInfo;
            varInfo.name = GetSymbolName(pSymbol);

            IDiaSymbol* pType = nullptr;
            if (SUCCEEDED(pSymbol->get_type(&pType)) && pType) {
                varInfo.type = GetTypeName(pType);
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
