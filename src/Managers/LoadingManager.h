#pragma once

#include "PDBViewerGlobals.h"
#include <atomic>
#include <memory>

constexpr UINT WM_APP_LOADING_PROGRESS = WM_APP + 40;
constexpr UINT WM_APP_LOADING_COMPLETE = WM_APP + 41;

struct LoadingTaskState {
    std::atomic<HWND> owner{nullptr};
    std::atomic_bool cancelled{false};
};

struct LoadingProgress {
    std::shared_ptr<LoadingTaskState> task;
    std::wstring text;
};

struct LoadingResult {
    bool success = false;
    bool cancelled = false;
    ULONGLONG downloadMs = 0;
    ULONGLONG openMs = 0;
    ULONGLONG parseMs = 0;
    ULONGLONG backgroundMs = 0;
    ULONGLONG uiMs = 0;
    ModuleInfo module;
    std::wstring pdbPath;
    std::wstring errorMessage;
    std::shared_ptr<LoadingTaskState> task;
};

class LoadingManager {
public:
    static bool StartPdbFile(HWND owner, const std::wstring& pdbPath);
    static bool StartDllFile(HWND owner, const std::wstring& dllPath);
    static bool IsActive();
    static void CancelCurrentTask();
    static void HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void Shutdown();

private:
    static bool StartTask(HWND owner, const std::wstring& path, bool downloadPdb);
    static void PostProgress(const std::shared_ptr<LoadingTaskState>& task, int percent, const std::wstring& message);
    static void PostResult(std::unique_ptr<LoadingResult> result);
    static std::wstring LocalizeStatus(const std::wstring& status);
};
