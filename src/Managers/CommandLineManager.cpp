#include "CommandLineManager.h"
#include "ConfigManager.h"
#include "PDBDownloader.h"
#include "PDBExporter.h"
#include "PDBHeaderGenerator.h"
#include "PDBParser.h"
#include "LanguageManager.h"
#include <sstream>
#include <algorithm>
#include <shellapi.h>
#include <chrono>

#pragma comment(lib, "shell32.lib")

CommandLineManager& CommandLineManager::GetInstance() {
    static CommandLineManager instance;
    return instance;
}

bool CommandLineManager::ParseCommandLine(LPWSTR lpCmdLine) {
    UNREFERENCED_PARAMETER(lpCmdLine);
    m_options = CommandLineOptions{};
    m_args.clear();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;

    std::wstring arg;
    
    for (int i = 1; i < argc; ++i) {
        auto takeValue = [&](std::wstring& target) -> bool {
            if (i + 1 >= argc) return false;
            target = argv[++i];
            return true;
        };
        arg = argv[i];
        
        std::wstring lowerArg = ToLower(arg);
        
        if (lowerArg == L"-h" || lowerArg == L"--help" || lowerArg == L"/?") {
            m_options.showHelp = true;
        }
        else if (lowerArg == L"-q" || lowerArg == L"--quiet") {
            m_options.quietMode = true;
        }
        else if (lowerArg == L"-d" || lowerArg == L"--download" ||
                 lowerArg == L"--download-pdb") {
            m_options.autoDownloadPdb = true;
        }
        else if (lowerArg == L"-e" || lowerArg == L"--export") {
            std::wstring value;
            if (takeValue(value)) m_options.exportFormat = ToLower(value);
        }
        else if (lowerArg == L"-o" || lowerArg == L"--output") {
            std::wstring value;
            if (takeValue(value)) m_options.exportPath = ExtractFilePath(value);
        }
        else if (lowerArg == L"-c" || lowerArg == L"--class") {
            takeValue(m_options.className);
        }
        else if (lowerArg == L"-a" || lowerArg == L"--all") {
            m_options.exportAll = true;
        }
        else if (lowerArg == L"-l" || lowerArg == L"--list") {
            m_options.listClasses = true;
        }
        else if (lowerArg == L"--timing") {
            m_options.measureTiming = true;
        }
        else if (IsPdbFile(arg)) {
            m_options.pdbPath = ExtractFilePath(arg);
        }
        else if (IsDllFile(arg)) {
            m_options.dllPath = ExtractFilePath(arg);
            m_options.autoDownloadPdb = true;
        }
        
        m_args.push_back(arg);
    }

    LocalFree(argv);

    return true;
}

std::wstring CommandLineManager::GetHelpText() const {
    std::wstring help = L"PDB Insight - PDB File Viewer\n\n";
    help += L"Usage: PDBInsight.exe [options] [file]\n\n";
    help += L"Options:\n";
    help += L"  -h, --help, /?     Show this help message\n";
    help += L"  -q, --quiet        Quiet mode (no UI, for batch processing)\n";
    help += L"  -d, --download     Auto-download PDB for DLL/EXE files\n";
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
    return !m_options.exportFormat.empty() &&
        (HasValidPdbPath() || ShouldAutoDownloadPdb());
}

bool CommandLineManager::ShouldExportClass() const {
    return ShouldExport() && !m_options.className.empty();
}

bool CommandLineManager::ShouldListClasses() const {
    return m_options.listClasses && HasValidPdbPath();
}

namespace {
void WriteCommandLineText(HANDLE handle, const std::wstring& text) {
    if (!handle || handle == INVALID_HANDLE_VALUE || text.empty()) return;

    DWORD consoleMode = 0;
    if (GetConsoleMode(handle, &consoleMode)) {
        DWORD written = 0;
        WriteConsoleW(handle, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
        return;
    }

    const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Size <= 0) return;
    std::string utf8(static_cast<size_t>(utf8Size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
        utf8.data(), utf8Size, nullptr, nullptr);
    DWORD written = 0;
    WriteFile(handle, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

void WriteCommandLineLine(HANDLE handle, const std::wstring& text) {
    WriteCommandLineText(handle, text + L"\r\n");
}

const ClassInfo* FindCommandLineType(const ModuleInfo& module, const std::wstring& name) {
    if (name.empty()) return nullptr;

    auto search = [&](const std::vector<ClassInfo>& types) -> const ClassInfo* {
        for (const auto& type : types) {
            if (_wcsicmp(type.name.c_str(), name.c_str()) == 0) return &type;
        }
        const std::wstring flattened = PDBHeaderGenerator::FlattenName(name);
        for (const auto& type : types) {
            if (_wcsicmp(PDBHeaderGenerator::FlattenName(type.name).c_str(),
                         flattened.c_str()) == 0) return &type;
        }
        return nullptr;
    };

    if (const ClassInfo* type = search(module.classes)) return type;
    if (const ClassInfo* type = search(module.structs)) return type;
    return search(module.unions);
}

bool EnsureCommandLineDirectory(const std::wstring& path) {
    if (path.empty()) return false;
    if (CreateDirectoryW(path.c_str(), nullptr)) return true;
    return GetLastError() == ERROR_ALREADY_EXISTS &&
        (GetFileAttributesW(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring ResolveCommandLineFile(const std::wstring& requested,
                                    const std::wstring& defaultName) {
    if (requested.empty()) return defaultName;
    const DWORD attributes = GetFileAttributesW(requested.c_str());
    const bool isDirectory = attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (!isDirectory) return requested;
    return requested + L"\\" + defaultName;
}
}

bool CommandLineManager::ShouldRunCommandLine() const {
    return m_options.showHelp || m_options.quietMode ||
        ShouldExport() || ShouldListClasses();
}

int CommandLineManager::RunCommandLine() const {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE errorOutput = GetStdHandle(STD_ERROR_HANDLE);

    if (m_options.showHelp) {
        WriteCommandLineLine(output, GetHelpText());
        return 0;
    }

    ConfigManager::GetInstance().Load();
    const std::wstring configuredLanguage = ConfigManager::GetInstance().GetLanguage();
    if (!LanguageManager::GetInstance().LoadLanguageByCode(configuredLanguage)) {
        LanguageManager::GetInstance().DetectAndLoadSystemLanguage();
    }
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        WriteCommandLineLine(errorOutput, L"Failed to initialize COM");
        return 3;
    }

    std::wstring pdbPath = m_options.pdbPath;
    if (pdbPath.empty() && ShouldAutoDownloadPdb()) {
        PDBDownloadResult download = PDBDownloader::GetInstance().DownloadPDBForDll(m_options.dllPath);
        if (!download.success) {
            WriteCommandLineLine(errorOutput, L"PDB download failed: " + download.errorMessage);
            return 3;
        }
        pdbPath = download.pdbPath;
    }

    if (pdbPath.empty()) {
        WriteCommandLineLine(errorOutput, L"A PDB or DLL/EXE input is required.");
        return 2;
    }

    PDBParser parser;
    int lastReportedProgress = -1;
    parser.SetProgressCallback([&](int percent, const std::wstring& status) {
        if (m_options.quietMode) return;
        if (percent < 100 && percent - lastReportedProgress < 5) return;
        lastReportedProgress = percent;
        WriteCommandLineLine(errorOutput, L"Progress " + std::to_wstring(percent) + L"%: " + status);
    });
    std::vector<std::wstring> stageTimings;
    if (m_options.measureTiming) {
        parser.SetStageTimingCallback([&stageTimings](const std::wstring& fragment) {
            stageTimings.push_back(fragment);
        });
    }
    if (!parser.LoadPDB(pdbPath)) {
        WriteCommandLineLine(errorOutput, L"Failed to open PDB: " + parser.GetLastError());
        return 3;
    }
    const auto parseStart = std::chrono::steady_clock::now();
    ModuleInfo module = parser.ParseModule();
    const auto parseEnd = std::chrono::steady_clock::now();
    module.pdbFileName = pdbPath;

    if (m_options.measureTiming) {
        const auto parseMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            parseEnd - parseStart).count();
        std::wstring stageText;
        for (const auto& fragment : stageTimings) {
            if (!stageText.empty()) stageText += L" ";
            stageText += fragment;
        }
        WriteCommandLineLine(errorOutput,
            L"Timing: parse " + std::to_wstring(parseMs) + L" ms | classes " +
            std::to_wstring(module.classes.size()) + L" structs " +
            std::to_wstring(module.structs.size()) + L" unions " +
            std::to_wstring(module.unions.size()));
        if (!stageText.empty()) {
            WriteCommandLineLine(errorOutput, L"Timing: stages " + stageText);
        }
    }

    if (m_options.listClasses) {
        auto writeGroup = [&](const wchar_t* label, const std::vector<ClassInfo>& types) {
            WriteCommandLineLine(output, label);
            for (const auto& type : types) WriteCommandLineLine(output, L"  " + type.name);
        };
        writeGroup(L"Classes:", module.classes);
        writeGroup(L"Structs:", module.structs);
        writeGroup(L"Unions:", module.unions);
        return 0;
    }

    if (m_options.exportFormat.empty()) return 0;

    const ExportSettings& settings = ConfigManager::GetInstance().GetExportSettings();
    const bool expandBaseClasses = ConfigManager::GetInstance().GetExpandBaseClasses();
    const NumberDisplayMode numberMode = ConfigManager::GetInstance().GetNumberMode();
    const std::wstring language = ConfigManager::GetInstance().GetLanguage();
    std::wstring outputPath = m_options.exportPath;
    bool success = false;

    if (m_options.exportFormat == L"csv") {
        outputPath = ResolveCommandLineFile(outputPath, L"export.csv");
        success = PDBExporter::ExportToCSV(module, outputPath, expandBaseClasses, language);
    } else if (m_options.exportFormat == L"xml") {
        outputPath = ResolveCommandLineFile(outputPath, L"export.xml");
        success = PDBExporter::ExportToXML(module, outputPath, expandBaseClasses);
    } else if (m_options.exportFormat == L"header" ||
               m_options.exportFormat == L"allheaders") {
        const bool exportAll = m_options.exportAll ||
            m_options.exportFormat == L"allheaders";
        if (exportAll) {
            outputPath = outputPath.empty() ? L"headers" : outputPath;
            if (!EnsureCommandLineDirectory(outputPath)) {
                WriteCommandLineLine(errorOutput, L"Cannot create output directory: " + outputPath);
                return 4;
            }
            success = PDBHeaderGenerator::GenerateAllHeaders(module, outputPath, settings,
                numberMode, expandBaseClasses);
        } else {
            const ClassInfo* type = FindCommandLineType(module, m_options.className);
            if (!type) {
                WriteCommandLineLine(errorOutput, L"Class/struct not found: " + m_options.className);
                if (SUCCEEDED(comResult)) CoUninitialize();
                return 4;
            }
            const std::wstring baseName = settings.flattenNamespaces
                ? PDBHeaderGenerator::FlattenName(type->name) : type->name;
            outputPath = ResolveCommandLineFile(outputPath, baseName + L".h");
            if (!m_options.exportPath.empty()) {
                const DWORD attributes = GetFileAttributesW(m_options.exportPath.c_str());
                const bool exportPathIsDirectory = attributes != INVALID_FILE_ATTRIBUTES &&
                    (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if (!exportPathIsDirectory) {
                    const size_t separator = outputPath.find_last_of(L"\\/");
                    if (separator != std::wstring::npos) {
                        const std::wstring parent = outputPath.substr(0, separator);
                        if (!EnsureCommandLineDirectory(parent)) {
                            WriteCommandLineLine(errorOutput,
                                L"Cannot create output directory: " + parent);
                            if (SUCCEEDED(comResult)) CoUninitialize();
                            return 4;
                        }
                    }
                }
            }
            success = PDBHeaderGenerator::GenerateClassHeader(*type, outputPath, settings,
                numberMode, expandBaseClasses, &module);
        }
    } else {
        WriteCommandLineLine(errorOutput, L"Unsupported export format: " + m_options.exportFormat);
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 2;
    }

    if (!success) {
        WriteCommandLineLine(errorOutput, L"Export failed: " + outputPath);
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 4;
    }

    WriteCommandLineLine(output, L"Exported: " + outputPath);
    if (SUCCEEDED(comResult)) CoUninitialize();
    return 0;
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
    constexpr size_t suffixLength = 4;
    return lower.size() >= suffixLength &&
        lower.compare(lower.size() - suffixLength, suffixLength, L".pdb") == 0;
}

bool CommandLineManager::IsDllFile(const std::wstring& path) {
    std::wstring lower = ToLower(path);
    auto hasSuffix = [&](const wchar_t* suffix) {
        const size_t length = wcslen(suffix);
        return lower.size() >= length &&
            lower.compare(lower.size() - length, length, suffix) == 0;
    };
    return hasSuffix(L".dll") || hasSuffix(L".exe");
}

std::wstring CommandLineManager::ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}
