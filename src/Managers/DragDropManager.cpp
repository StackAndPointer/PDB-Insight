#include "DragDropManager.h"
#include "ControlsManager.h"
#include "TreeViewManager.h"
#include "CacheManager.h"
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
                // 检查是否存在缓存文件
                if (CacheManager::GetInstance().CacheExists(filePath)) {
                    // 尝试加载缓存文件
                    ModuleInfo cachedModuleInfo;
                    std::wstring cachePath = CacheManager::GetInstance().GetCacheFilePath(filePath);
                    std::wstring errorMsg;
                    if (CacheManager::GetInstance().LoadCache(cachedModuleInfo, cachePath, errorMsg)) {
                        g_moduleInfo = cachedModuleInfo;
                        g_pdbLoaded = true;

                        PopulateTreeView();

                        std::wstringstream ss;
                        ss << LanguageManager::GetInstance().GetString(L"status_loaded_cache", L"已从缓存加载: ") << filePath
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_functions", L"函数: ") << g_moduleInfo.functions.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_classes", L"类: ") << g_moduleInfo.classes.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体: ") << g_moduleInfo.structs.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体: ") << g_moduleInfo.unions.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举: ") << g_moduleInfo.enums.size();
                        UpdateStatusBar(ss.str());
                    }
                }
                else {
                    if (g_parser.LoadPDB(filePath)) {
                        g_parser.SetProgressCallback([](int progress, const std::wstring& text) {
                            std::wstring status = LanguageManager::GetInstance().GetString(L"status_parsing", L"解析中: ") + text + L" (" + std::to_wstring(progress) + L"%)";
                            UpdateStatusBar(status);
                        });
                        g_moduleInfo = g_parser.ParseModule();
                        g_moduleInfo.pdbFileName = filePath;
                        g_pdbLoaded = true;

                        // 保存缓存文件
                        CacheManager::GetInstance().SaveCache(g_moduleInfo, filePath);
                        
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
                }
            }
            else if (extension == L".pdbbc") {
                ModuleInfo moduleInfo;
                std::wstring errorMsg;
                if (CacheManager::GetInstance().LoadCache(moduleInfo, filePath, errorMsg)) {
                    g_moduleInfo = moduleInfo;
                    g_pdbLoaded = true;

                    PopulateTreeView();

                    std::wstringstream ss;
                        ss << LanguageManager::GetInstance().GetString(L"status_loaded_cache", L"已从缓存加载: ") << filePath
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_functions", L"函数: ") << g_moduleInfo.functions.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_classes", L"类: ") << g_moduleInfo.classes.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_structs", L"结构体: ") << g_moduleInfo.structs.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_unions", L"联合体: ") << g_moduleInfo.unions.size()
                           << L" | " << LanguageManager::GetInstance().GetString(L"tree_enums", L"枚举: ") << g_moduleInfo.enums.size();
                        UpdateStatusBar(ss.str());
                }
            }
        }
    }
    
    DragFinish(hDrop);
}
