#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct CommandLineOptions {
    std::wstring pdbPath;
    std::wstring dllPath;
    bool showHelp = false;
    bool autoDownloadPdb = false;
    bool quietMode = false;
    std::wstring exportFormat;
    std::wstring exportPath;
    std::wstring className;
    bool exportAll = false;
    bool listClasses = false;
};

class CommandLineManager {
public:
    static CommandLineManager& GetInstance();

    bool ParseCommandLine(LPWSTR lpCmdLine);
    const CommandLineOptions& GetOptions() const { return m_options; }
    
    std::wstring GetHelpText() const;
    void ShowHelp(HWND hWnd = nullptr) const;
    
    bool HasValidPdbPath() const;
    bool HasValidDllPath() const;
    bool ShouldAutoDownloadPdb() const;
    bool ShouldExport() const;
    bool ShouldExportClass() const;
    bool ShouldListClasses() const;

private:
    CommandLineManager() {}
    ~CommandLineManager() = default;
    CommandLineManager(const CommandLineManager&) = delete;
    CommandLineManager& operator=(const CommandLineManager&) = delete;

    std::wstring ExtractFilePath(const std::wstring& arg);
    bool IsPdbFile(const std::wstring& path);
    bool IsDllFile(const std::wstring& path);
    std::wstring ToLower(const std::wstring& str);

    CommandLineOptions m_options;
    std::vector<std::wstring> m_args;
};