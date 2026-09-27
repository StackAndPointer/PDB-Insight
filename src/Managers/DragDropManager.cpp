#include "DragDropManager.h"
#include "LoadingManager.h"
#include <shellapi.h>

HWND DragDropManager::s_hWnd = nullptr;

void DragDropManager::Initialize(HWND hWnd) {
    s_hWnd = hWnd;
    DragAcceptFiles(hWnd, TRUE);
}

void DragDropManager::Cleanup() {
    if (s_hWnd) DragAcceptFiles(s_hWnd, FALSE);
    s_hWnd = nullptr;
}

void DragDropManager::HandleDropFiles(WPARAM wParam, HWND hWnd) {
    HDROP drop = reinterpret_cast<HDROP>(wParam);
    UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    if (fileCount > 0 && !LoadingManager::IsActive()) {
        WCHAR file[MAX_PATH] = L"";
        DragQueryFileW(drop, 0, file, _countof(file));
        std::wstring path = file;
        std::wstring extension;
        size_t dot = path.find_last_of(L'.');
        if (dot != std::wstring::npos) {
            extension = path.substr(dot);
            for (auto& c : extension) c = towlower(c);
        }

        if (extension == L".pdb") {
            LoadingManager::StartPdbFile(hWnd, path);
        } else if (extension == L".dll" || extension == L".exe") {
            LoadingManager::StartDllFile(hWnd, path);
        }
    }
    DragFinish(drop);
}
