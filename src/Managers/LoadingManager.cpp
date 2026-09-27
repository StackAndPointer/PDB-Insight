#include "LoadingManager.h"
#include "ControlsManager.h"
#include "PDBParser.h"
#include "PDBDownloader.h"
#include "TreeViewManager.h"
#include <chrono>
#include <memory>
#include <thread>

namespace {
ULONGLONG ElapsedMs(std::chrono::steady_clock::time_point start) {
    return static_cast<ULONGLONG>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count());
}

std::atomic_bool s_active{false};
std::shared_ptr<LoadingTaskState> s_task;

void SetTaskActive(bool active) {
    s_active.store(active);
    SetTaskMode(active);
}
}

bool LoadingManager::StartPdbFile(HWND owner, const std::wstring& pdbPath) {
    return StartTask(owner, pdbPath, false);
}

bool LoadingManager::StartDllFile(HWND owner, const std::wstring& dllPath) {
    return StartTask(owner, dllPath, true);
}

bool LoadingManager::StartTask(HWND owner, const std::wstring& path, bool downloadPdb) {
    if (s_active.exchange(true)) return false;

    auto task = std::make_shared<LoadingTaskState>();
    task->owner.store(owner);
    s_task = task;
    SetTaskActive(true);
    UpdateStatusBar(downloadPdb ? LANG_STR(L"status_downloading_pdb") : LANG_STR(L"status_loading"));

    std::thread([path, downloadPdb, task]() {
        auto result = std::make_unique<LoadingResult>();
        result->task = task;
        const auto backgroundStart = std::chrono::steady_clock::now();
        std::wstring pdbPath = path;

        if (downloadPdb) {
            const auto downloadStart = std::chrono::steady_clock::now();
            PDBDownloadResult download = PDBDownloader::GetInstance().DownloadPDBForDll(
                path, L"",
                [task](int percent, const std::wstring& status) {
                    PostProgress(task, percent, status);
                },
                [task]() { return task->cancelled.load(); });
            result->downloadMs = ElapsedMs(downloadStart);

            if (download.cancelled || task->cancelled.load()) {
                result->cancelled = true;
                PostResult(std::move(result));
                return;
            }
            if (!download.success) {
                result->errorMessage = download.errorMessage;
                PostResult(std::move(result));
                return;
            }
            pdbPath = download.pdbPath;
        }

        PDBParser parser;
        parser.SetCancellationCallback([task]() { return task->cancelled.load(); });
        parser.SetProgressCallback([task](int percent, const std::wstring& status) {
            PostProgress(task, percent, status);
        });

        const auto openStart = std::chrono::steady_clock::now();
        if (!parser.LoadPDB(pdbPath)) {
            result->openMs = ElapsedMs(openStart);
            if (parser.WasCancelled() || task->cancelled.load()) {
                result->cancelled = true;
            } else {
                result->errorMessage = parser.GetLastError();
            }
            PostResult(std::move(result));
            return;
        }
        result->openMs = ElapsedMs(openStart);

        const auto parseStart = std::chrono::steady_clock::now();
        result->module = parser.ParseModule();
        result->parseMs = ElapsedMs(parseStart);
        if (parser.WasCancelled() || task->cancelled.load()) {
            result->cancelled = true;
        } else {
            result->success = true;
            result->module.pdbFileName = pdbPath;
            result->pdbPath = pdbPath;
        }
        result->backgroundMs = ElapsedMs(backgroundStart);
        PostResult(std::move(result));
    }).detach();

    return true;
}

bool LoadingManager::IsActive() {
    return s_active.load();
}

void LoadingManager::CancelCurrentTask() {
    if (!s_active.load() || !s_task) return;
    s_task->cancelled.store(true);
    EnableWindow(hButtonCancelTask, FALSE);
    UpdateStatusBar(LANG_STR(L"status_canceling"));
}

void LoadingManager::Shutdown() {
    if (s_task) {
        s_task->cancelled.store(true);
        s_task->owner.store(nullptr);
    }
    s_active.store(false);
    s_task.reset();
}

void LoadingManager::PostProgress(const std::shared_ptr<LoadingTaskState>& task, int percent, const std::wstring& message) {
    if (!task || task->cancelled.load()) return;
    HWND owner = task->owner.load();
    if (!IsWindow(owner)) return;

    auto progress = std::make_unique<LoadingProgress>();
    progress->task = task;
    progress->text = message;
    LoadingProgress* rawProgress = progress.release();
    if (!PostMessageW(owner, WM_APP_LOADING_PROGRESS, static_cast<WPARAM>(percent), reinterpret_cast<LPARAM>(rawProgress))) {
        delete rawProgress;
    }
}

void LoadingManager::PostResult(std::unique_ptr<LoadingResult> result) {
    HWND owner = result->task ? result->task->owner.load() : nullptr;
    LoadingResult* rawResult = result.release();
    if (!IsWindow(owner) || !PostMessageW(owner, WM_APP_LOADING_COMPLETE, 0, reinterpret_cast<LPARAM>(rawResult))) {
        delete rawResult;
    }
}
std::wstring LoadingManager::LocalizeStatus(const std::wstring& status) {
    if (status == L"Reading debug info from DLL/EXE...") return LANG_STR(L"status_reading_debug_info");
    if (status.rfind(L"Debug info found: ", 0) == 0) return LANG_STR(L"status_debug_info_found") + status.substr(18);
    if (status == L"Connecting to Microsoft symbol server...") return LANG_STR(L"status_connecting_symbol_server");
    if (status == L"Opening connection...") return LANG_STR(L"status_opening_connection");
    if (status == L"Download complete") return LANG_STR(L"status_download_complete");
    if (status == L"Parsing functions...") return LANG_STR(L"status_parsing_functions");
    if (status == L"Parsing global variables...") return LANG_STR(L"status_parsing_global_variables");
    if (status == L"Parsing classes...") return LANG_STR(L"status_parsing_classes");
    if (status == L"Parsing structs and unions...") return LANG_STR(L"status_parsing_structs_unions");
    if (status == L"Complete") return LANG_STR(L"status_complete");
    if (status.rfind(L"Downloading... ", 0) == 0) return LANG_STR(L"status_downloading_progress") + status.substr(15);
    return status;
}

void LoadingManager::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_APP_LOADING_PROGRESS) {
        std::unique_ptr<LoadingProgress> progress(reinterpret_cast<LoadingProgress*>(lParam));
        if (!progress || (progress->task && progress->task->cancelled.load())) return;
        int percent = static_cast<int>(wParam);
        SendMessageW(hProgressTask, PBM_SETPOS, percent, 0);
        UpdateStatusBar(LocalizeStatus(progress->text));
        return;
    }

    if (message != WM_APP_LOADING_COMPLETE) return;

    std::unique_ptr<LoadingResult> result(reinterpret_cast<LoadingResult*>(lParam));
    if (!result) return;
    if (result->task) result->task->owner.store(nullptr);
    s_task.reset();
    SetTaskActive(false);
    EnableWindow(hButtonCancelTask, TRUE);

    if (result->cancelled) {
        UpdateStatusBar(LANG_STR(L"status_task_cancelled"));
        return;
    }

    if (!result->success) {
        std::wstring messageText = LANG_STR(L"msg_pdb_load_fail") + L": " + result->errorMessage;
        MessageBoxW(hWnd, messageText.c_str(), LANG_STR(L"msg_error").c_str(), MB_OK | MB_ICONERROR);
        UpdateStatusBar(LANG_STR(L"status_task_failed"));
        return;
    }

    const auto uiStart = std::chrono::steady_clock::now();
    g_moduleInfo = std::move(result->module);
    g_pdbLoaded = true;
    ClearSearchBox();
    PopulateTreeView();
    result->uiMs = ElapsedMs(uiStart);

    std::wstringstream status;
    status << LANG_STR(L"status_loaded") << L": " << result->pdbPath
           << L" | " << LANG_STR(L"tree_functions") << L": " << g_moduleInfo.functions.size()
           << L" | " << LANG_STR(L"tree_classes") << L": " << g_moduleInfo.classes.size()
           << L" | " << LANG_STR(L"tree_structs") << L": " << g_moduleInfo.structs.size()
           << L" | " << LANG_STR(L"tree_unions") << L": " << g_moduleInfo.unions.size()
           << L" | " << LANG_STR(L"tree_enums") << L": " << g_moduleInfo.enums.size()
           << L" | " << result->backgroundMs << L" ms"
           << L" (open " << result->openMs << L", parse " << result->parseMs
           << L", UI " << result->uiMs << L")";
    UpdateStatusBar(status.str());
}
