#include "LanguageManager.h"
#include "Resource.h"
#include <algorithm>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

LanguageManager& LanguageManager::GetInstance() {
    static LanguageManager instance;
    return instance;
}

std::wstring LanguageManager::GetLanguageFilePath(const std::wstring& languageCode) {
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);
    
    std::wstring filePath = modulePath;
    filePath += L"\\i18n\\" + languageCode + L".json";
    return filePath;
}

bool LanguageManager::LoadLanguageFromResource(UINT resourceId) {
    HMODULE hModule = GetModuleHandleW(nullptr);
    HRSRC hResource = FindResourceW(hModule, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hResource) {
        return false;
    }
    
    HGLOBAL hData = LoadResource(hModule, hResource);
    if (!hData) {
        return false;
    }
    
    DWORD size = SizeofResource(hModule, hResource);
    const char* data = static_cast<const char*>(LockResource(hData));
    if (!data || size == 0) {
        return false;
    }
    
    std::string content(data, size);
    
    std::wstring wcontent;
    int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
    if (len > 0) {
        wcontent.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wcontent[0], len);
        wcontent.pop_back();
    }
    
    if (ParseJSON(wcontent)) {
        m_loaded = true;
        return true;
    }
    return false;
}

bool LanguageManager::LoadLanguageFile(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    file.close();
    
    std::wstring wcontent;
    int len = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, nullptr, 0);
    if (len > 0) {
        wcontent.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wcontent[0], len);
        wcontent.pop_back();
    }
    
    if (ParseJSON(wcontent)) {
        m_loaded = true;
        return true;
    }
    return false;
}

bool LanguageManager::LoadLanguageByCode(const std::wstring& languageCode) {
    UINT resourceId = 0;
    if (languageCode == L"zh-CN") {
        resourceId = IDR_LANG_ZH_CN;
    } else if (languageCode == L"en-US") {
        resourceId = IDR_LANG_EN_US;
    }
    
    if (resourceId != 0 && LoadLanguageFromResource(resourceId)) {
        m_currentLanguageCode = languageCode;
        return true;
    }
    
    std::wstring filePath = GetLanguageFilePath(languageCode);
    if (LoadLanguageFile(filePath)) {
        m_currentLanguageCode = languageCode;
        return true;
    }
    return false;
}

std::wstring LanguageManager::GetSystemLanguageCode() {
    LANGID langId = GetUserDefaultUILanguage();
    WCHAR langCode[9];
    GetLocaleInfoW(langId, LOCALE_SNAME, langCode, 9);
    return std::wstring(langCode);
}

bool LanguageManager::DetectAndLoadSystemLanguage() {
    std::wstring sysLang = GetSystemLanguageCode();
    
    if (sysLang.find(L"zh") == 0) {
        if (LoadLanguageByCode(L"zh-CN")) {
            return true;
        }
    }
    
    if (LoadLanguageByCode(L"en-US")) {
        return true;
    }
    
    SetDefaultLanguage();
    return false;
}

std::wstring LanguageManager::GetString(const std::wstring& key, const std::wstring& defaultValue) {
    auto it = m_strings.find(key);
    if (it != m_strings.end()) {
        return it->second;
    }
    return defaultValue;
}

void LanguageManager::SetDefaultLanguage() {
    m_strings.clear();
    m_loaded = false;
    m_currentLanguageCode.clear();
}

bool LanguageManager::ParseJSON(const std::wstring& content) {
    m_strings.clear();
    
    std::wstring json = content;
    size_t pos = 0;
    
    while (pos < json.length()) {
        pos = json.find(L'"', pos);
        if (pos == std::wstring::npos) break;
        
        size_t keyStart = pos + 1;
        pos = json.find(L'"', keyStart);
        if (pos == std::wstring::npos) break;
        
        std::wstring key = json.substr(keyStart, pos - keyStart);
        pos = json.find(L':', pos);
        if (pos == std::wstring::npos) break;
        
        pos = json.find(L'"', pos);
        if (pos == std::wstring::npos) break;
        
        size_t valueStart = pos + 1;
        pos = json.find(L'"', valueStart);
        if (pos == std::wstring::npos) break;
        
        std::wstring value = json.substr(valueStart, pos - valueStart);
        m_strings[key] = value;
        
        pos++;
    }
    
    m_loaded = !m_strings.empty();
    return m_loaded;
}
