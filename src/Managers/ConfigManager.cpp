#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() 
    : m_language(L"zh-CN")
    , m_numberMode(NUMBER_HEX)
    , m_expandBaseClasses(false)
    , m_useMirrorSource(false) {
}

std::wstring ConfigManager::GetConfigFilePath() {
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);
    
    std::wstring configPath = modulePath;
    configPath += L"\\config.ini";
    return configPath;
}

bool ConfigManager::Load() {
    std::wstring configPath = GetConfigFilePath();
    std::wifstream file(configPath);
    
    if (!file.is_open()) {
        return false;
    }
    
    std::wstring line;
    while (std::getline(file, line)) {
        size_t eqPos = line.find(L'=');
        if (eqPos != std::wstring::npos) {
            std::wstring key = line.substr(0, eqPos);
            std::wstring value = line.substr(eqPos + 1);
            
            if (key == L"language") {
                m_language = value;
            } else if (key == L"numberMode") {
                int mode = _wtoi(value.c_str());
                if (mode >= 0 && mode <= 2) {
                    m_numberMode = static_cast<NumberDisplayMode>(mode);
                }
            } else if (key == L"expandBaseClasses") {
                m_expandBaseClasses = (value == L"1");
            } else if (key == L"exportFlattenNamespaces") {
                m_exportSettings.flattenNamespaces = (value == L"1");
            } else if (key == L"exportRemoveVoidParams") {
                m_exportSettings.removeVoidParams = (value == L"1");
            } else if (key == L"exportIDACompatible") {
                m_exportSettings.idaCompatible = (value == L"1");
            } else if (key == L"exportIncludeEnumsInEnumsH") {
                m_exportSettings.includeEnumsInEnumsH = (value == L"1");
            } else if (key == L"useMirrorSource") {
                m_useMirrorSource = (value == L"1");
            } else if (key == L"mirrorSourceUrl") {
                m_mirrorSourceUrl = value;
            } else if (key.find(L"searchHistory[") == 0 && key.back() == L']') {
                m_searchHistory.push_back(value);
            }
        }
    }
    
    file.close();
    return true;
}

bool ConfigManager::Save() {
    std::wstring configPath = GetConfigFilePath();
    std::wofstream file(configPath);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << L"language=" << m_language << L"\n";
    file << L"numberMode=" << static_cast<int>(m_numberMode) << L"\n";
    file << L"expandBaseClasses=" << (m_expandBaseClasses ? L"1" : L"0") << L"\n";
    file << L"exportFlattenNamespaces=" << (m_exportSettings.flattenNamespaces ? L"1" : L"0") << L"\n";
    file << L"exportRemoveVoidParams=" << (m_exportSettings.removeVoidParams ? L"1" : L"0") << L"\n";
    file << L"exportIDACompatible=" << (m_exportSettings.idaCompatible ? L"1" : L"0") << L"\n";
    file << L"exportIncludeEnumsInEnumsH=" << (m_exportSettings.includeEnumsInEnumsH ? L"1" : L"0") << L"\n";
    file << L"useMirrorSource=" << (m_useMirrorSource ? L"1" : L"0") << L"\n";
    file << L"mirrorSourceUrl=" << m_mirrorSourceUrl << L"\n";
    
    for (size_t i = 0; i < m_searchHistory.size(); ++i) {
        file << L"searchHistory[" << i << L"]=" << m_searchHistory[i] << L"\n";
    }
    
    file.close();
    return true;
}

void ConfigManager::AddSearchHistory(const std::wstring& text) {
    if (text.empty()) {
        return;
    }
    
    for (auto it = m_searchHistory.begin(); it != m_searchHistory.end(); ++it) {
        if (*it == text) {
            m_searchHistory.erase(it);
            break;
        }
    }
    
    m_searchHistory.insert(m_searchHistory.begin(), text);
    
    while (m_searchHistory.size() > MAX_SEARCH_HISTORY) {
        m_searchHistory.pop_back();
    }
    
    Save();
}

void ConfigManager::ClearSearchHistory() {
    m_searchHistory.clear();
    Save();
}
