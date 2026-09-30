#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "PDBData.h"

struct ExportSettings {
    bool flattenNamespaces = true;
    bool expandAnonymousAggregates = true;
    bool removeVoidParams = true;
    bool idaCompatible = false;
    bool includeEnumsInEnumsH = true;
};

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    bool Load();
    bool Save();

    std::wstring GetLanguage() const { return m_language; }
    void SetLanguage(const std::wstring& lang) { m_language = lang; }

    NumberDisplayMode GetNumberMode() const { return m_numberMode; }
    void SetNumberMode(NumberDisplayMode mode) { m_numberMode = mode; }

    bool GetExpandBaseClasses() const { return m_expandBaseClasses; }
    void SetExpandBaseClasses(bool expand) { m_expandBaseClasses = expand; }

    int GetThemeMode() const { return m_themeMode; }
    void SetThemeMode(int mode) { m_themeMode = mode; }

    const ExportSettings& GetExportSettings() const { return m_exportSettings; }
    void SetExportSettings(const ExportSettings& settings) { m_exportSettings = settings; }

    bool GetUseMirrorSource() const { return m_useMirrorSource; }
    void SetUseMirrorSource(bool use) { m_useMirrorSource = use; }

    std::wstring GetMirrorSourceUrl() const { return m_mirrorSourceUrl; }
    void SetMirrorSourceUrl(const std::wstring& url) { m_mirrorSourceUrl = url; }

    const std::vector<std::wstring>& GetSearchHistory() const { return m_searchHistory; }
    void AddSearchHistory(const std::wstring& text);
    void ClearSearchHistory();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::wstring GetConfigFilePath();

    std::wstring m_language;
    NumberDisplayMode m_numberMode;
    bool m_expandBaseClasses;
    int m_themeMode;
    bool m_useMirrorSource;
    std::wstring m_mirrorSourceUrl;
    ExportSettings m_exportSettings;
    std::vector<std::wstring> m_searchHistory;
    static const size_t MAX_SEARCH_HISTORY = 20;
};
