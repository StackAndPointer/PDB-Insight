#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

class LanguageManager {
public:
    static LanguageManager& GetInstance();

    bool LoadLanguageFile(const std::wstring& filePath);
    bool LoadLanguageByCode(const std::wstring& languageCode);
    std::wstring GetString(const std::wstring& key, const std::wstring& defaultValue = L"");
    
    void SetDefaultLanguage();
    bool DetectAndLoadSystemLanguage();
    std::wstring GetSystemLanguageCode();
    bool IsLanguageLoaded() const { return m_loaded; }

private:
    LanguageManager() : m_loaded(false) {}
    ~LanguageManager() = default;
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    bool ParseJSON(const std::wstring& content);
    std::wstring GetLanguageFilePath(const std::wstring& languageCode);
    bool LoadLanguageFromResource(UINT resourceId);

    std::map<std::wstring, std::wstring> m_strings;
    bool m_loaded;
    std::wstring m_currentLanguageCode;
};

#define LANG_STR(key) LanguageManager::GetInstance().GetString(key, key)
