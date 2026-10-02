#include "TreeNodeHelper.h"

VirtualTreeState g_virtualTreeState;

TreeNodeType TreeNodeParamHelper::GetType(DWORD param) {
    if (param == 0) return TreeNodeType::Root;
    if (param >= 1 && param <= 6) {
        return static_cast<TreeNodeType>(param);
    }
    if (param >= 10000 && param < 20000) return TreeNodeType::FunctionItem;
    if (param >= 20000 && param < 30000) return TreeNodeType::ClassItem;
    if (param >= 30000 && param < 40000) return TreeNodeType::StructItem;
    if (param >= 40000 && param < 50000) return TreeNodeType::UnionItem;
    if (param >= 50000 && param < 60000) return TreeNodeType::EnumItem;
    if (param >= 60000 && param < 70000) return TreeNodeType::GlobalVarItem;
    return TreeNodeType::Root;
}

size_t TreeNodeParamHelper::GetIndex(DWORD param) {
    if (param >= 10000 && param < 20000) return param - 10000;
    if (param >= 20000 && param < 30000) return param - 20000;
    if (param >= 30000 && param < 40000) return param - 30000;
    if (param >= 40000 && param < 50000) return param - 40000;
    if (param >= 50000 && param < 60000) return param - 50000;
    if (param >= 60000 && param < 70000) return param - 60000;
    return 0;
}

DWORD TreeNodeParamHelper::MakeParam(TreeNodeType type, size_t index) {
    return static_cast<DWORD>(type) + static_cast<DWORD>(index);
}

bool TreeNodeParamHelper::IsCategoryNode(DWORD param) {
    return param >= 0 && param <= 6;
}

bool TreeNodeParamHelper::IsItemNode(DWORD param) {
    return param >= 10000;
}

bool TreeNodeParamHelper::IsFunctionNode(DWORD param) {
    return param >= 10000 && param < 20000;
}

bool TreeNodeParamHelper::IsClassNode(DWORD param) {
    return param >= 20000 && param < 30000;
}

bool TreeNodeParamHelper::IsStructNode(DWORD param) {
    return param >= 30000 && param < 40000;
}

bool TreeNodeParamHelper::IsUnionNode(DWORD param) {
    return param >= 40000 && param < 50000;
}

bool TreeNodeParamHelper::IsEnumNode(DWORD param) {
    return param >= 50000 && param < 60000;
}

bool TreeNodeParamHelper::IsGlobalVarNode(DWORD param) {
    return param >= 60000 && param < 70000;
}

void ResetVirtualTreeState() {

    g_virtualTreeState = VirtualTreeState{};

}



void SetVirtualTreeCategory(TreeCategory category, size_t itemCount) {

    g_virtualTreeState.category = category;

    g_virtualTreeState.itemCount = itemCount;

    g_virtualTreeState.filteredIndices.clear();

}



void SetVirtualTreeFilter(std::vector<size_t> indices) {

    g_virtualTreeState.filteredIndices = std::move(indices);

}



bool IsVirtualTreeCategoryNode(LPARAM param) {

    return false;

}
