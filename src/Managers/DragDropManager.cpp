#include "DragDropManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include <shellapi.h>
#include <string>

HWND DragDropManager::s_hWnd = nullptr;

void DragDropManager::Initialize(HWND hWnd) {
    s_hWnd = hWnd;
    DragAcceptFiles(hWnd, TRUE);
}

void DragDropManager::Cleanup() {
    if (s_hWnd) {
        DragAcceptFiles(s_hWnd, FALSE);
    }
    s_hWnd = nullptr;
}

void DragDropManager::HandleDropFiles(WPARAM wParam, HWND hWnd) {
    HDROP hDrop = (HDROP)wParam;
    UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    
    if (fileCount > 0) {
        WCHAR filePath[MAX_PATH];
        DragQueryFileW(hDrop, 0, filePath, MAX_PATH);
        
        std::wstring filePathStr = filePath;
        std::wstring extension;
        size_t dotPos = filePathStr.find_last_of(L'.');
        if (dotPos != std::wstring::npos) {
            extension = filePathStr.substr(dotPos);
            for (size_t i = 0; i < extension.length(); i++) {
                extension[i] = towlower(extension[i]);
            }
            
            if (extension == L".pdb") {
                if (g_parser.LoadPDB(filePath)) {
                    g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                        std::wstring status = L"解析中: " + text + L" (" + std::to_wstring(progress) + L"%)";
                        UpdateStatusBar(status);
                    });
                    g_moduleInfo = g_parser.ParseModule();
                    g_moduleInfo.pdbFileName = filePath;
                    g_pdbLoaded = true;
                    
                    PopulateTreeView();
                    
                    std::wstringstream ss;
                    ss << L"已加载: " << filePath
                        << L" | 函数: " << g_moduleInfo.functions.size()
                        << L" | 类: " << g_moduleInfo.classes.size()
                        << L" | 结构体: " << g_moduleInfo.structs.size()
                        << L" | 联合体: " << g_moduleInfo.unions.size()
                        << L" | 枚举: " << g_moduleInfo.enums.size();
                    UpdateStatusBar(ss.str());
                }
            }
        }
    }
    
    DragFinish(hDrop);
}
