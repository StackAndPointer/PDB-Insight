#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>

enum NumberDisplayMode {
    NUMBER_HEX,
    NUMBER_DEC,
    NUMBER_BOTH
};

enum AccessType {
    PDB_ACCESS_PUBLIC,
    PDB_ACCESS_PROTECTED,
    PDB_ACCESS_PRIVATE,
    PDB_ACCESS_UNKNOWN
};

enum CallingConvention {
    PDB_CALL_CDECL,
    PDB_CALL_STDCALL,
    PDB_CALL_FASTCALL,
    PDB_CALL_VECTORCALL,
    PDB_CALL_THISCALL,
    PDB_CALL_UNKNOWN
};

enum InheritanceType {
    PDB_INHERITANCE_NORMAL,
    PDB_INHERITANCE_VIRTUAL,
    PDB_INHERITANCE_UNKNOWN
};

struct ParameterInfo {
    std::wstring name;
    std::wstring type;
    bool hasDefaultValue;
};

struct FunctionInfo {
    std::wstring name;
    std::wstring undecoratedName;
    std::wstring returnType;
    CallingConvention callingConvention;
    std::vector<ParameterInfo> parameters;
    DWORD rva;
    ULONGLONG virtualAddress;
    ULONGLONG size;
    bool isStatic;
    bool isVirtual;
    bool isMemberFunction;
    std::wstring className;
};

struct MemberVariableInfo {
    std::wstring name;
    std::wstring type;
    LONG offset;
    AccessType access;
    DWORD bitPosition;
    DWORD bitSize;
};

struct BaseClassInfo {
    std::wstring name;
    InheritanceType inheritanceType;
    LONG offset;
    AccessType access;
};

struct VirtualFunctionInfo {
    std::wstring name;
    std::wstring returnType;
    std::vector<ParameterInfo> parameters;
    DWORD rva;
    ULONGLONG virtualAddress;
    int vtableIndex;
};

struct ClassInfo {
    std::wstring name;
    ULONGLONG size;
    ULONGLONG alignment;
    std::vector<BaseClassInfo> baseClasses;
    std::vector<MemberVariableInfo> members;
    std::vector<std::wstring> memberFunctions;
    std::vector<VirtualFunctionInfo> virtualFunctions;
    bool isStruct;
    bool isUnion;
};

struct EnumValueInfo {
    std::wstring name;
    LONGLONG value;
};

struct EnumInfo {
    std::wstring name;
    std::wstring underlyingType;
    std::vector<EnumValueInfo> values;
};

struct GlobalVariableInfo {
    std::wstring name;
    std::wstring type;
    DWORD rva;
    ULONGLONG virtualAddress;
    ULONGLONG size;
};

struct ModuleInfo {
    std::wstring name;
    std::wstring pdbFileName;
    std::vector<FunctionInfo> functions;
    std::vector<ClassInfo> classes;
    std::vector<ClassInfo> structs;
    std::vector<ClassInfo> unions;
    std::vector<EnumInfo> enums;
    std::vector<GlobalVariableInfo> globalVariables;
};
