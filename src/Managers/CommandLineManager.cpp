#include "CommandLineManager.h"
#include <sstream>
#include <algorithm>

CommandLineManager& CommandLineManager::GetInstance() {
    static CommandLineManager instance;
    return instance;
}

bool CommandLineManager::ParseCommandLine(LPWSTR lpCmdLine) {
    if (!lpCmdLine || wcslen(lpCmdLine) == 0) {
        return true;
    }

    std::wstring cmdLine = lpCmdLine;
    std::wstringstream ss(cmdLine);
    std::wstring arg;
    
    while (ss >> arg) {
        if (arg[0] == L'"') {
            size_t endPos = cmdLine.find(L'"', 1);
            if (endPos != std::wstring::npos) {
                arg = cmdLine.substr(1, endPos - 1);
                cmdLine = cmdLine.substr(endPos + 1);
                ss.clear();
                ss.str(cmdLine);
            }
        }
        
        std::wstring lowerArg = ToLower(arg);
        
        if (lowerArg == L"-h" || lowerArg == L"--help" || lowerArg == L"/?") {
            m_options.showHelp = true;
        }
        else if (lowerArg == L"-q" || lowerArg == L"--quiet") {
            m_options.quietMode = true;
        }
        else if (lowerArg == L"-d" || lowerArg == L"--download-pdb") {
            m_options.autoDownloadPdb = true;
        }
        else if (lowerArg == L"-e" || lowerArg == L"--export") {
            if (ss >> arg) {
                m_options.exportFormat = ToLower(arg);
            }
        }
        else if (lowerArg == L"-o" || lowerArg == L"--output") {
            if (ss >> arg) {
                m_options.exportPath = ExtractFilePath(arg);
            }
        }
        else if (lowerArg == L"-c" || lowerArg == L"--class") {
            if (ss >> arg) {
                m_options.className = arg;
            }
        }
        else if (lowerArg == L"-a" || lowerArg == L"--all") {
            m_options.exportAll = true;
        }
        else if (lowerArg == L"-l" || lowerArg == L"--list") {
            m_options.listClasses = true;
        }
        else if (IsPdbFile(arg)) {
            m_options.pdbPath = ExtractFilePath(arg);
        }
        else if (IsDllFile(arg)) {
            m_options.dllPath = ExtractFilePath(arg);
            m_options.autoDownloadPdb = true;
        }
        else if (arg[0] != L'-' && arg[0] != L'/') {
            std::wstring ext = ToLower(arg.substr(arg.find_last_of(L'.')));
            if (ext == L".pdb") {
                m_options.pdbPath = ExtractFilePath(arg);
            }
            else if (ext == L".dll" || ext == L".exe") {
                m_options.dllPath = ExtractFilePath(arg);
                m_options.autoDownloadPdb = true;
            }
        }
        
        m_args.push_back(arg);
    }

    return true;
}

std::wstring CommandLineManager::GetHelpText() const {
    std::wstring help = L"PDB Insight - PDB File Viewer\n\n";
    help += L"Usage: PDBInsight.exe [options] [file]\n\n";
    help += L"Options:\n";
    help += L"  -h, --help, /?     Show this help message\n";
    help += L"  -q, --quiet        Quiet mode (no UI, for batch processing)\n";
    help += L"  -d, --download-pdb Auto-download PDB for DLL/EXE files\n";
    help += L"  -e, --export <format> Export format: csv, xml, header, allheaders\n";
    help += L"  -o, --output <path> Output file/directory path\n";
    help += L"  -c, --class <name> Export specific class/struct\n";
    help += L"  -a, --all          Export all classes/structs\n";
    help += L"  -l, --list         List all classes in PDB\n\n";
    help += L"File:\n";
    help += L"  <pdb_file>         PDB file to open\n";
    help += L"  <dll/exe_file>     DLL/EXE file (will auto-download PDB)\n\n";
    help += L"Examples:\n";
    help += L"  PDBInsight.exe mylib.pdb\n";
    help += L"  PDBInsight.exe -d kernel32.dll\n";
    help += L"  PDBInsight.exe -e csv -o output.csv mylib.pdb\n";
    help += L"  PDBInsight.exe -e header -c MyClass -o MyClass.h mylib.pdb\n";
    help += L"  PDBInsight.exe -e allheaders -o ./headers mylib.pdb\n";
    help += L"  PDBInsight.exe -l mylib.pdb\n";
    return help;
}

void CommandLineManager::ShowHelp(HWND hWnd) const {
    std::wstring help = GetHelpText();
    if (hWnd) {
        MessageBoxW(hWnd, help.c_str(), L"PDB Insight Help", MB_OK | MB_ICONINFORMATION);
    }
    else {
        wprintf(L"%s\n", help.c_str());
    }
}

bool CommandLineManager::HasValidPdbPath() const {
    return !m_options.pdbPath.empty();
}

bool CommandLineManager::HasValidDllPath() const {
    return !m_options.dllPath.empty();
}

bool CommandLineManager::ShouldAutoDownloadPdb() const {
    return m_options.autoDownloadPdb && HasValidDllPath();
}

bool CommandLineManager::ShouldExport() const {
    return !m_options.exportFormat.empty() && HasValidPdbPath();
}

bool CommandLineManager::ShouldExportClass() const {
    return ShouldExport() && !m_options.className.empty();
}

bool CommandLineManager::ShouldListClasses() const {
    return m_options.listClasses && HasValidPdbPath();
}

std::wstring CommandLineManager::ExtractFilePath(const std::wstring& arg) {
    if (arg.empty()) return L"";
    
    std::wstring path = arg;
    if (path[0] == L'"') {
        size_t endPos = path.find(L'"', 1);
        if (endPos != std::wstring::npos) {
            path = path.substr(1, endPos - 1);
        }
    }
    return path;
}

bool CommandLineManager::IsPdbFile(const std::wstring& path) {
    std::wstring lower = ToLower(path);
    return lower.find(L".pdb") != std::wstring::npos;
}

bool CommandLineManager::IsDllFile(const std::wstring& path) {
    std::wstring lower = ToLower(path);
    return lower.find(L".dll") != std::wstring::npos || lower.find(L".exe") != std::wstring::npos;
}

std::wstring CommandLineManager::ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}