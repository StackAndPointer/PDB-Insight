#pragma once

#include <windows.h>
#include <cstddef>
#include <utility>
#include <vector>

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

enum class TreeCategory {
    None = 0,
    Functions = 1,
    Classes = 2,
    Structs = 3,
    Unions = 4,
    Enums = 5,
    GlobalVariables = 6
};

struct VirtualTreeState {
    TreeCategory category = TreeCategory::None;
    size_t itemCount = 0;
    size_t pendingTreeIndex = 0;
    void* treeParent = nullptr;
    std::vector<size_t> filteredIndices;
};

extern VirtualTreeState g_virtualTreeState;

void ResetVirtualTreeState();
void SetVirtualTreeCategory(TreeCategory category, size_t itemCount);
void SetVirtualTreeFilter(std::vector<size_t> indices);
bool IsVirtualTreeCategoryNode(LPARAM param);
