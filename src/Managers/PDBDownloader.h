#pragma once

#include <windows.h>
#include <string>
#include <functional>

struct PDBDownloadResult {
    bool success = false;
    bool cancelled = false;
    std::wstring pdbPath;
    std::wstring errorMessage;
    std::wstring pdbGuid;
    DWORD pdbAge = 0;
};

class PDBDownloader {
public:
    static PDBDownloader& GetInstance();

    PDBDownloadResult DownloadPDBForDll(const std::wstring& dllPath,
                                        const std::wstring& outputPath = L"",
                                        std::function<void(int, const std::wstring&)> progressCallback = nullptr,
                                        std::function<bool()> cancellationCallback = nullptr);

    bool GetDebugInfoFromDll(const std::wstring& dllPath, std::wstring& guid, DWORD& age);
    std::wstring BuildMicrosoftSymbolUrl(const std::wstring& pdbName, const std::wstring& guid, DWORD age);

    bool DownloadFile(const std::wstring& url,
                      const std::wstring& localPath,
                      std::function<void(int, const std::wstring&)> progressCallback = nullptr,
                      std::function<bool()> cancellationCallback = nullptr);

    std::wstring GetPdbFileNameFromDll(const std::wstring& dllPath);
    std::wstring GetDllDirectory(const std::wstring& dllPath);

private:
    PDBDownloader() {}
    ~PDBDownloader() = default;
    PDBDownloader(const PDBDownloader&) = delete;
    PDBDownloader& operator=(const PDBDownloader&) = delete;

    bool ReadPeDebugInfo(const std::wstring& dllPath, std::wstring& pdbName, std::wstring& guid, DWORD& age);
    std::wstring FormatGuidForUrl(const std::wstring& guid, DWORD age);
};
