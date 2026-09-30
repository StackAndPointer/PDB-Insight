#include "ListViewManager.h"
#include "DPIManager.h"
#include "FunctionInfoDisplayManager.h"
#include "LanguageManager.h"
#include "TreeNodeHelper.h"

void PopulateListView(HTREEITEM hItem)
{
    ListView_DeleteAllItems(hListView);
    SendMessageW(hListView, WM_SETREDRAW, FALSE, 0);

    for (int i = Header_GetItemCount(ListView_GetHeader(hListView)) - 1; i >= 0; --i)
    {
        ListView_DeleteColumn(hListView, i);
    }

    if (!hItem || !g_pdbLoaded) {
        SendMessageW(hListView, WM_SETREDRAW, TRUE, 0);
        return;
    }

    TVITEM tvi;
    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(hTreeView, &tvi);

    DWORD param = (DWORD)tvi.lParam;

    if (param == 1)
    {
        AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_name", L"名称"), 200);
        AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_return_type", L"返回类型"), 100);
        AddListViewColumn(2, LanguageManager::GetInstance().GetString(L"col_calling_convention", L"调用约定"), 100);
        AddListViewColumn(3, LanguageManager::GetInstance().GetString(L"col_rva", L"RVA"), 100);
        AddListViewColumn(4, LanguageManager::GetInstance().GetString(L"col_size", L"大小"), 80);
        AddListViewColumn(5, LanguageManager::GetInstance().GetString(L"col_class_name", L"类名"), 150);

        for (size_t i = 0; i < g_moduleInfo.functions.size(); ++i)
        {
            const auto& func = g_moduleInfo.functions[i];
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)(func.undecoratedName.empty() ? func.name : func.undecoratedName).c_str();
            ListView_InsertItem(hListView, &lvi);

            ListView_SetItemText(hListView, (int)i, 1, (LPWSTR)func.returnType.c_str());
            ListView_SetItemText(hListView, (int)i, 2, (LPWSTR)PDBParser::CallingConventionToString(func.callingConvention).c_str());
            
            std::wstringstream ssRVA;
            ssRVA << std::hex << std::showbase << func.rva;
            ListView_SetItemText(hListView, (int)i, 3, (LPWSTR)ssRVA.str().c_str());
            
            std::wstringstream ssSize;
            ssSize << func.size;
            ListView_SetItemText(hListView, (int)i, 4, (LPWSTR)ssSize.str().c_str());
            
            ListView_SetItemText(hListView, (int)i, 5, (LPWSTR)func.className.c_str());
        }
    }
    else if (param == 2 || param == 3 || param == 4)
    {
        const std::vector<ClassInfo>* pClasses = nullptr;
        if (param == 2) pClasses = &g_moduleInfo.classes;
        else if (param == 3) pClasses = &g_moduleInfo.structs;
        else if (param == 4) pClasses = &g_moduleInfo.unions;

        if (pClasses)
        {
            AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_name", L"名称"), 250);
            AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_size", L"大小"), 80);
            AddListViewColumn(2, LanguageManager::GetInstance().GetString(L"col_member_count", L"成员数量"), 100);
            AddListViewColumn(3, LanguageManager::GetInstance().GetString(L"col_base_count", L"基类数量"), 100);

            for (size_t i = 0; i < pClasses->size(); ++i)
            {
                const auto& cls = (*pClasses)[i];
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = (int)i;
                lvi.iSubItem = 0;
                lvi.pszText = (LPWSTR)cls.name.c_str();
                ListView_InsertItem(hListView, &lvi);

                std::wstringstream ssSize;
                ssSize << cls.size;
                ListView_SetItemText(hListView, (int)i, 1, (LPWSTR)ssSize.str().c_str());
                
                std::wstringstream ssMembers;
                ssMembers << cls.members.size();
                ListView_SetItemText(hListView, (int)i, 2, (LPWSTR)ssMembers.str().c_str());
                
                std::wstringstream ssBases;
                ssBases << cls.baseClasses.size();
                ListView_SetItemText(hListView, (int)i, 3, (LPWSTR)ssBases.str().c_str());
            }
        }
    }
    else if (param == 5)
    {
        AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_name", L"名称"), 250);
        AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_type", L"类型"), 150);
        AddListViewColumn(2, LanguageManager::GetInstance().GetString(L"col_member_count", L"值数量"), 100);

        for (size_t i = 0; i < g_moduleInfo.enums.size(); ++i)
        {
            const auto& enm = g_moduleInfo.enums[i];
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)enm.name.c_str();
            ListView_InsertItem(hListView, &lvi);

            ListView_SetItemText(hListView, (int)i, 1, (LPWSTR)enm.underlyingType.c_str());
            
            std::wstringstream ssValues;
            ssValues << enm.values.size();
            ListView_SetItemText(hListView, (int)i, 2, (LPWSTR)ssValues.str().c_str());
        }
    }
    else if (param == 6)
    {
        AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_name", L"名称"), 250);
        AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_type", L"类型"), 150);
        AddListViewColumn(2, LanguageManager::GetInstance().GetString(L"col_rva", L"RVA"), 100);
        AddListViewColumn(3, LanguageManager::GetInstance().GetString(L"col_virtual_address", L"虚拟地址"), 150);
        AddListViewColumn(4, LanguageManager::GetInstance().GetString(L"col_size", L"大小"), 80);

        for (size_t i = 0; i < g_moduleInfo.globalVariables.size(); ++i)
        {
            const auto& var = g_moduleInfo.globalVariables[i];
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)var.name.c_str();
            ListView_InsertItem(hListView, &lvi);

            ListView_SetItemText(hListView, (int)i, 1, (LPWSTR)var.type.c_str());
            
            std::wstringstream ssRVA;
            ssRVA << std::hex << std::showbase << var.rva;
            ListView_SetItemText(hListView, (int)i, 2, (LPWSTR)ssRVA.str().c_str());
            
            std::wstringstream ssVA;
            ssVA << std::hex << std::showbase << var.virtualAddress;
            ListView_SetItemText(hListView, (int)i, 3, (LPWSTR)ssVA.str().c_str());
            
            std::wstringstream ssSize;
            ssSize << var.size;
            ListView_SetItemText(hListView, (int)i, 4, (LPWSTR)ssSize.str().c_str());
        }
    }
    else if (TreeNodeParamHelper::IsFunctionNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.functions.size())
        {
            const auto& func = g_moduleInfo.functions[idx];
            auto displayInfo = FunctionInfoDisplayManager::GetDisplayInfo(func);
            
            AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_property", L"属性"), 150);
            AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_value", L"值"), 400);

            auto addItem = [&](const std::wstring& name, const std::wstring& value) {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hListView);
                lvi.iSubItem = 0;
                lvi.pszText = (LPWSTR)name.c_str();
                ListView_InsertItem(hListView, &lvi);
                ListView_SetItemText(hListView, lvi.iItem, 1, (LPWSTR)value.c_str());
            };

            addItem(LanguageManager::GetInstance().GetString(L"prop_name_decorated", L"名称 (修饰)"), func.name.empty() ? L"(未知)" : func.name);
            addItem(LanguageManager::GetInstance().GetString(L"prop_name_undecorated", L"名称 (未修饰)"), func.undecoratedName.empty() ? L"(未知)" : func.undecoratedName);
            addItem(LanguageManager::GetInstance().GetString(L"col_return_type", L"返回类型"), func.returnType.empty() ? L"(未知)" : func.returnType);
            addItem(LanguageManager::GetInstance().GetString(L"col_calling_convention", L"调用约定"), PDBParser::CallingConventionToString(func.callingConvention).empty() ? L"(未知)" : PDBParser::CallingConventionToString(func.callingConvention));
            
            std::wstringstream ssRVA;
            ssRVA << std::hex << std::showbase << func.rva;
            addItem(LanguageManager::GetInstance().GetString(L"prop_rva", L"RVA"), ssRVA.str());
            
            std::wstringstream ssVA;
            ssVA << std::hex << std::showbase << func.virtualAddress;
            addItem(LanguageManager::GetInstance().GetString(L"prop_virtual_address", L"虚拟地址"), ssVA.str());
            
            addItem(LanguageManager::GetInstance().GetString(L"col_size", L"大小"), PDBHeaderGenerator::FormatNumber(func.size, g_numberMode));
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_static", L"静态"), func.isStatic ? LanguageManager::GetInstance().GetString(L"yes", L"是") : LanguageManager::GetInstance().GetString(L"no", L"否"));
            addItem(LanguageManager::GetInstance().GetString(L"prop_virtual", L"虚函数"), func.isVirtual ? LanguageManager::GetInstance().GetString(L"yes", L"是") : LanguageManager::GetInstance().GetString(L"no", L"否"));
            addItem(LanguageManager::GetInstance().GetString(L"prop_member_function", L"成员函数"), func.isMemberFunction ? LanguageManager::GetInstance().GetString(L"yes", L"是") : LanguageManager::GetInstance().GetString(L"no", L"否"));
            addItem(LanguageManager::GetInstance().GetString(L"prop_class", L"所属类"), func.className.empty() ? L"(无)" : func.className);

            if (func.parameters.empty()) {
                addItem(LanguageManager::GetInstance().GetString(L"prop_param_prefix", L"参数"), L"(无参数)");
            } else {
                for (size_t i = 0; i < func.parameters.size(); ++i)
                {
                    std::wstringstream ssName;
                    ssName << LanguageManager::GetInstance().GetString(L"prop_param_prefix", L"参数 ") << i;
                    std::wstring paramType = func.parameters[i].type.empty() ? L"(未知)" : func.parameters[i].type;
                    std::wstring paramName = func.parameters[i].name.empty() ? L"(未知)" : func.parameters[i].name;
                    std::wstring paramInfo = paramType + L" " + paramName;
                    addItem(ssName.str(), paramInfo);
                }
            }
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_virtual_functions", L"信息完整性"), FunctionInfoDisplayManager::FormatCompletenessLabel(displayInfo.completenessPercent));
        }
    }
    else if (TreeNodeParamHelper::IsClassNode(param) || TreeNodeParamHelper::IsStructNode(param) || TreeNodeParamHelper::IsUnionNode(param))
    {
        size_t idx;
        const ClassInfo* pClass = nullptr;
        
        if (TreeNodeParamHelper::IsClassNode(param))
        {
            idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.classes.size()) pClass = &g_moduleInfo.classes[idx];
        }
        else if (TreeNodeParamHelper::IsStructNode(param))
        {
            idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.structs.size()) pClass = &g_moduleInfo.structs[idx];
        }
        else if (TreeNodeParamHelper::IsUnionNode(param))
        {
            idx = TreeNodeParamHelper::GetIndex(param);
            if (idx < g_moduleInfo.unions.size()) pClass = &g_moduleInfo.unions[idx];
        }

        if (pClass)
        {
            AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_property", L"属性"), 150);
            AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_value", L"值"), 450);
            
            auto addItem = [&](const std::wstring& name, const std::wstring& value) {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hListView);
                lvi.iSubItem = 0;
                lvi.pszText = (LPWSTR)name.c_str();
                ListView_InsertItem(hListView, &lvi);
                ListView_SetItemText(hListView, lvi.iItem, 1, (LPWSTR)value.c_str());
            };
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_name", L"名称"), pClass->name);
            std::wstring typeName = LanguageManager::GetInstance().GetString(L"type_class", L"类");
            if (pClass->isStruct) typeName = LanguageManager::GetInstance().GetString(L"type_struct", L"结构体");
            else if (pClass->isUnion) typeName = LanguageManager::GetInstance().GetString(L"type_union", L"联合体");
            addItem(LanguageManager::GetInstance().GetString(L"prop_type", L"类型"), typeName);
            
            addItem(LanguageManager::GetInstance().GetString(L"col_size", L"大小"), PDBHeaderGenerator::FormatNumber(pClass->size, g_numberMode));
            
            if (!pClass->baseClasses.empty())
            {
                std::wstring baseStr;
                for (size_t i = 0; i < pClass->baseClasses.size(); ++i)
                {
                    if (i > 0) baseStr += L", ";
                    baseStr += pClass->baseClasses[i].name;
                }
                addItem(LanguageManager::GetInstance().GetString(L"prop_base_classes", L"基类"), baseStr);
            }
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_member_count", L"成员数量"), std::to_wstring(pClass->members.size()));
            addItem(LanguageManager::GetInstance().GetString(L"col_virtual_function_count", L"虚函数数量"), std::to_wstring(pClass->virtualFunctions.size()));
            addItem(L"", L"");
            
            if (!pClass->virtualFunctions.empty())
            {
                addItem(LanguageManager::GetInstance().GetString(L"prop_virtual_function_table", L"--- 虚函数表 ---"), L"");
                for (size_t i = 0; i < pClass->virtualFunctions.size(); ++i)
                {
                    const auto& vfunc = pClass->virtualFunctions[i];
                    std::wstringstream ssVFunc;
                    ssVFunc << LanguageManager::GetInstance().GetString(L"comment_vtable_slot_prefix", L"// [slot ")
                            << vfunc.vtableIndex << L"] " << vfunc.returnType << L" " << vfunc.name;
                    std::wstringstream ssAddr;
                    if (vfunc.rva != 0)
                    {
                        ssAddr << L"RVA: 0x" << std::hex << vfunc.rva;
                    }
                    if (vfunc.virtualAddress != 0)
                    {
                        if (vfunc.rva != 0) ssAddr << L", ";
                        ssAddr << L"VA: 0x" << std::hex << vfunc.virtualAddress;
                    }
                    addItem(ssVFunc.str(), ssAddr.str());
                }
                addItem(L"", L"");
            }
            
            if (g_expandBaseClasses && !pClass->baseClasses.empty())
            {
                std::vector<MemberVariableInfo> allMembers;
                PDBHeaderGenerator::CollectAllMembersFromOffsetZero(
                    *pClass, &g_moduleInfo, L"", allMembers);
                std::wstring activeBaseClass;
                for (const auto& member : allMembers)
                {
                    if (member.fromBaseClass && member.baseClassName != activeBaseClass)
                    {
                        if (!activeBaseClass.empty()) addItem(L"", L"");
                        activeBaseClass = member.baseClassName;
                        addItem(LanguageManager::GetInstance().GetString(
                                    L"prop_expanded_from_base", L"--- Expanded from base class: ") +
                                activeBaseClass + L" ---", L"");
                    }
                    std::wstringstream ssMember;
                    ssMember << PDBParser::AccessTypeToString(member.access) << L" "
                             << (member.isStatic ? L"static " : L"")
                             << member.type << L" " << member.name;
                    std::wstringstream ssOffset;
                    ssOffset << LanguageManager::GetInstance().GetString(L"col_offset", L"偏移: ")
                             << PDBHeaderGenerator::FormatOffset(member.offset, g_numberMode, member.offsetReconstructed);
                    if (member.bitSize > 0)
                    {
                        ssOffset << L", " << LanguageManager::GetInstance().GetString(L"col_bit_position", L"位位置: ") << member.bitPosition
                                 << L", " << LanguageManager::GetInstance().GetString(L"col_bit_size", L"位大小: ") << member.bitSize;
                    }
                    addItem(ssMember.str(), ssOffset.str());
                }
                if (!activeBaseClass.empty()) addItem(L"", L"");
            }
            else
            {
                addItem(LanguageManager::GetInstance().GetString(L"prop_members_list", L"--- 成员列表 ---"), L"");
                for (size_t i = 0; i < pClass->members.size(); ++i)
                {
                    const auto& member = pClass->members[i];
                    std::wstringstream ssMember;
                    ssMember << PDBParser::AccessTypeToString(member.access) << L" "
                             << (member.isStatic ? L"static " : L"")
                             << member.type << L" " << member.name;
                    std::wstringstream ssOffset;
                    ssOffset << LanguageManager::GetInstance().GetString(L"col_offset", L"偏移: ")
                             << PDBHeaderGenerator::FormatOffset(member.offset, g_numberMode, member.offsetReconstructed);
                    if (member.bitSize > 0)
                    {
                        ssOffset << L", " << LanguageManager::GetInstance().GetString(L"col_bit_position", L"位位置: ") << member.bitPosition
                                 << L", " << LanguageManager::GetInstance().GetString(L"col_bit_size", L"位大小: ") << member.bitSize;
                    }
                    addItem(ssMember.str(), ssOffset.str());
                }
            }
        }
    }
    else if (TreeNodeParamHelper::IsEnumNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.enums.size())
        {
            const auto& enm = g_moduleInfo.enums[idx];
            AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_property", L"属性"), 150);
            AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_value", L"值"), 450);
            
            auto addItem = [&](const std::wstring& name, const std::wstring& value) {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hListView);
                lvi.iSubItem = 0;
                lvi.pszText = (LPWSTR)name.c_str();
                ListView_InsertItem(hListView, &lvi);
                ListView_SetItemText(hListView, lvi.iItem, 1, (LPWSTR)value.c_str());
            };
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_name", L"名称"), enm.name);
            addItem(LanguageManager::GetInstance().GetString(L"col_type", L"底层类型"), enm.underlyingType);
            addItem(LanguageManager::GetInstance().GetString(L"col_member_count", L"值数量"), std::to_wstring(enm.values.size()));
            addItem(L"", L"");
            addItem(LanguageManager::GetInstance().GetString(L"prop_members_list", L"--- 枚举值列表 ---"), L"");
            
            for (size_t i = 0; i < enm.values.size(); ++i)
            {
                std::wstringstream ssValue;
                ssValue << enm.values[i].name;
                std::wstringstream ssVal;
                ssVal << std::hex << std::showbase << enm.values[i].value;
                addItem(ssValue.str(), ssVal.str());
            }
        }
    }
    else if (TreeNodeParamHelper::IsGlobalVarNode(param))
    {
        size_t idx = TreeNodeParamHelper::GetIndex(param);
        if (idx < g_moduleInfo.globalVariables.size())
        {
            const auto& var = g_moduleInfo.globalVariables[idx];
            AddListViewColumn(0, LanguageManager::GetInstance().GetString(L"col_property", L"属性"), 150);
            AddListViewColumn(1, LanguageManager::GetInstance().GetString(L"col_value", L"值"), 450);
            
            auto addItem = [&](const std::wstring& name, const std::wstring& value) {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hListView);
                lvi.iSubItem = 0;
                lvi.pszText = (LPWSTR)name.c_str();
                ListView_InsertItem(hListView, &lvi);
                ListView_SetItemText(hListView, lvi.iItem, 1, (LPWSTR)value.c_str());
            };
            
            addItem(LanguageManager::GetInstance().GetString(L"prop_name", L"名称"), var.name);
            addItem(LanguageManager::GetInstance().GetString(L"prop_type", L"类型"), var.type);
            
            std::wstringstream ssRVA;
            ssRVA << std::hex << std::showbase << var.rva;
            addItem(LanguageManager::GetInstance().GetString(L"prop_rva", L"RVA"), ssRVA.str());
            
            std::wstringstream ssVA;
            ssVA << std::hex << std::showbase << var.virtualAddress;
            addItem(LanguageManager::GetInstance().GetString(L"prop_virtual_address", L"虚拟地址"), ssVA.str());
            
            addItem(LanguageManager::GetInstance().GetString(L"col_size", L"大小"), PDBHeaderGenerator::FormatNumber(var.size, g_numberMode));
        }
    }
    ResizeListViewColumns();
    SendMessageW(hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hListView, nullptr, TRUE);
}

void AddListViewColumn(int index, const std::wstring& text, int width)
{
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.iSubItem = index;
    lvc.pszText = (LPWSTR)text.c_str();
    lvc.cx = DPIManager::ScaleX(width);
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hListView, index, &lvc);
}

void ResizeListViewColumns()
{
    if (!hListView) return;
    int count = Header_GetItemCount(ListView_GetHeader(hListView));
    if (count <= 0) return;

    RECT client{};
    GetClientRect(hListView, &client);
    int available = max(0, client.right - client.left - GetSystemMetrics(SM_CXVSCROLL));
    std::vector<int> widths(count);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        widths[i] = ListView_GetColumnWidth(hListView, i);
        total += widths[i];
    }
    if (total < available) widths[count - 1] += available - total;
    for (int i = 0; i < count; ++i) ListView_SetColumnWidth(hListView, i, widths[i]);
}
