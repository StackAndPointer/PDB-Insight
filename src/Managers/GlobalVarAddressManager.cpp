#include "GlobalVarAddressManager.h"
#include <sstream>
#include <iomanip>

std::wstring GlobalVarAddressManager::FormatAddress(ULONGLONG address, NumberDisplayMode mode) {
    std::wostringstream ss;
    
    switch (mode) {
        case NUMBER_HEX:
            ss << L"0x" << std::hex << std::uppercase << std::setw(16) << std::setfill(L'0') << address;
            break;
        case NUMBER_DEC:
            ss << std::dec << address;
            break;
        case NUMBER_BOTH:
            ss << L"0x" << std::hex << std::uppercase << std::setw(16) << std::setfill(L'0') << address;
            ss << L" (" << std::dec << address << L")";
            break;
    }
    
    return ss.str();
}

std::wstring GlobalVarAddressManager::FormatRVA(DWORD rva) {
    std::wostringstream ss;
    ss << L"RVA: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << rva;
    return ss.str();
}

std::wstring GlobalVarAddressManager::GetSegmentOffset(ULONGLONG address) {
    std::wostringstream ss;
    ss << L".text+0x" << std::hex << std::uppercase << address;
    return ss.str();
}

bool GlobalVarAddressManager::CopyAddressToClipboard(HWND hWnd, ULONGLONG address) {
    std::wstring addressText = FormatAddress(address, NUMBER_HEX);
    
    if (!OpenClipboard(hWnd)) {
        return false;
    }
    
    EmptyClipboard();
    
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (addressText.length() + 1) * sizeof(WCHAR));
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }
    
    WCHAR* pData = (WCHAR*)GlobalLock(hGlobal);
    if (!pData) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }
    
    wcscpy_s(pData, addressText.length() + 1, addressText.c_str());
    GlobalUnlock(hGlobal);
    
    SetClipboardData(CF_UNICODETEXT, hGlobal);
    CloseClipboard();
    
    return true;
}

void GlobalVarAddressManager::AddAddressToTreeView(HTREEITEM hItem, const GlobalVariableInfo& var) {
}
