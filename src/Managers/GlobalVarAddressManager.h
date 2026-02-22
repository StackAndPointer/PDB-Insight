#pragma once

#include "PDBViewerGlobals.h"

class GlobalVarAddressManager {
public:
    static std::wstring FormatAddress(ULONGLONG address, NumberDisplayMode mode);
    static std::wstring FormatRVA(DWORD rva);
    static std::wstring GetSegmentOffset(ULONGLONG address);
    static bool CopyAddressToClipboard(HWND hWnd, ULONGLONG address);
    
    static void AddAddressToTreeView(HTREEITEM hItem, const GlobalVariableInfo& var);
};
