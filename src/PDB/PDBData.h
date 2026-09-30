#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

enum class SyntaxKind {
    Keyword,
    Type,
    Function,
    QualifiedIdentifier,
    Identifier,
    Comment,
    Literal,
    Punctuation
};

struct SyntaxSpan {
    size_t begin = 0;
    size_t end = 0;
    SyntaxKind kind = SyntaxKind::Identifier;
};

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

enum class TypeRefKind {
    Named,
    Pointer,
    Reference,
    Array,
    Function
};

struct TypeRef {
    TypeRefKind kind = TypeRefKind::Named;
    std::wstring name;
    std::shared_ptr<TypeRef> child;
    ULONGLONG arrayCount = 0;
    bool hasKnownArrayCount = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isRValueReference = false;
    std::vector<TypeRef> functionParameters;
    bool isVariadic = false;
    CallingConvention callingConvention = PDB_CALL_UNKNOWN;
};

struct ClassInfo;

struct ParameterInfo {
    std::wstring name;
    std::wstring type;
    bool hasDefaultValue = false;
    TypeRef typeRef;
};

struct FunctionInfo {
    std::wstring name;
    std::wstring undecoratedName;
    std::wstring displaySignature;
    std::wstring returnType;
    TypeRef returnTypeRef;
    CallingConvention callingConvention = PDB_CALL_UNKNOWN;
    std::vector<ParameterInfo> parameters;
    DWORD rva = 0;
    ULONGLONG virtualAddress = 0;
    ULONGLONG size = 0;
    bool isStatic = false;
    bool isVirtual = false;
    bool isMemberFunction = false;
    std::wstring className;
};

struct MemberVariableInfo {
    std::wstring name;
    std::wstring type;
    LONG offset = -1;
    ULONGLONG typeSize = 0;
    ULONGLONG typeAlignment = 0;
    bool isStatic = false;
    bool offsetReconstructed = false;
    AccessType access = PDB_ACCESS_UNKNOWN;
    DWORD bitPosition = 0;
    DWORD bitSize = 0;
    TypeRef typeRef;
    std::wstring baseClassName;
    bool fromBaseClass = false;
    bool typeSizeKnown = false;
    bool typeAlignmentKnown = false;
    bool isBitfield = false;
    bool isAnonymous = false;
    std::shared_ptr<ClassInfo> anonymousType;
};

struct BaseClassInfo {
    std::wstring name;
    InheritanceType inheritanceType = PDB_INHERITANCE_UNKNOWN;
    LONG offset = -1;
    AccessType access = PDB_ACCESS_UNKNOWN;
};

struct VirtualFunctionInfo {
    std::wstring name;
    std::wstring returnType;
    TypeRef returnTypeRef;
    std::vector<ParameterInfo> parameters;
    DWORD rva = 0;
    ULONGLONG virtualAddress = 0;
    int vtableIndex = -1;
    AccessType access = PDB_ACCESS_PUBLIC;
    bool isPure = false;
};

struct ClassInfo {
    std::wstring name;
    ULONGLONG size = 0;
    ULONGLONG alignment = 0;
    bool alignmentKnown = false;
    std::vector<BaseClassInfo> baseClasses;
    std::vector<MemberVariableInfo> members;
    std::vector<std::wstring> memberFunctions;
    std::vector<VirtualFunctionInfo> virtualFunctions;
    bool isStruct = false;
    bool isUnion = false;
};

struct EnumValueInfo {
    std::wstring name;
    LONGLONG value = 0;
};

struct EnumInfo {
    std::wstring name;
    std::wstring underlyingType;
    std::vector<EnumValueInfo> values;
};

struct GlobalVariableInfo {
    std::wstring name;
    std::wstring type;
    DWORD rva = 0;
    ULONGLONG virtualAddress = 0;
    ULONGLONG size = 0;
    TypeRef typeRef;
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
