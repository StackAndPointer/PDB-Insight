#pragma once

#include <windows.h>
#include <cstddef>

enum class TreeNodeType {
    Root = 0,
    FunctionCategory = 1,
    ClassCategory = 2,
    StructCategory = 3,
    UnionCategory = 4,
    EnumCategory = 5,
    GlobalVarCategory = 6,
    FunctionItem = 10000,
    ClassItem = 20000,
    StructItem = 30000,
    UnionItem = 40000,
    EnumItem = 50000,
    GlobalVarItem = 60000
};

class TreeNodeParamHelper {
public:
    static TreeNodeType GetType(DWORD param);
    static size_t GetIndex(DWORD param);
    static DWORD MakeParam(TreeNodeType type, size_t index);
    static bool IsCategoryNode(DWORD param);
    static bool IsItemNode(DWORD param);
    static bool IsFunctionNode(DWORD param);
    static bool IsClassNode(DWORD param);
    static bool IsStructNode(DWORD param);
    static bool IsUnionNode(DWORD param);
    static bool IsEnumNode(DWORD param);
    static bool IsGlobalVarNode(DWORD param);
    
    static const size_t MAX_ITEMS_PER_TYPE = 10000;
};