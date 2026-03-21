#include "DragDropManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "LanguageManager.h"
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
                UpdateStatusBar(LanguageManager::GetInstance().GetString(L"status_loading", L"正在加载 PDB 文件..."));
                
                if (g_parser.LoadPDB(filePath)) {
                    g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                        std::wstring status = LanguageManager::GetInstance().GetString(L"status_parsing", L"解析中: ") + text + L" (" + std::to_wstring(progress) + L"%)";
                        UpdateStatusBar(status);
                    });
                    g_moduleInfo = g_parser.ParseModule();
                    g_moduleInfo.pdbFileName = filePath;
                    g_pdbLoaded = true;

                    PopulateTreeView();

                    std::wstringstream ss;
                    ss << LanguageManager::GetInstance().GetString(L"status_loaded", L"已加载: ") << filePath
                       << L" | " << LanguageManager::GetInstance().GetString(L"tree_functions", L"函数: ") << g_moduleInfo.functions.size()
                       << L" | " << LanguageManager::GetInstance().GetString(L"tree_classes", L"类: ") << g_moduleInfo.classes.size()
                       << L" | " << LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体: ") << g_moduleInfo.structs.size()
                       << L" | " << LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体: ") << g_moduleInfo.unions.size()
                       << L" | " << LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举: ") << g_moduleInfo.enums.size();
                    UpdateStatusBar(ss.str());
                }
                else {
                    std::wstring errorMsg = LanguageManager::GetInstance().GetString(L"msg_pdb_load_fail", L"无法加载 PDB 文件: ") + g_parser.GetLastError();
                    MessageBoxW(hWnd, errorMsg.c_str(), 
                               LanguageManager::GetInstance().GetString(L"msg_error", L"错误").c_str(), MB_OK | MB_ICONERROR);
                    UpdateStatusBar(L"加载失败");
                }
            }
        }
    }
    
    DragFinish(hDrop);
}
