#include "SearchOptimizer.h"
#include <algorithm>
#include <cctype>

std::unordered_map<std::wstring, std::vector<size_t>> SearchOptimizer::s_functionIndex;
std::unordered_map<std::wstring, std::vector<size_t>> SearchOptimizer::s_variableIndex;
std::unordered_map<std::wstring, std::vector<size_t>> SearchOptimizer::s_typeIndex;
std::vector<SearchCacheEntry> SearchOptimizer::s_cache;

void SearchOptimizer::Initialize() {
    s_functionIndex.clear();
    s_variableIndex.clear();
    s_typeIndex.clear();
    s_cache.clear();
}

void SearchOptimizer::Cleanup() {
    s_functionIndex.clear();
    s_variableIndex.clear();
    s_typeIndex.clear();
    s_cache.clear();
}

void SearchOptimizer::BuildIndex(const ModuleInfo& moduleInfo) {
    s_functionIndex.clear();
    s_variableIndex.clear();
    s_typeIndex.clear();
    
    for (size_t i = 0; i < moduleInfo.functions.size(); i++) {
        std::wstring name = moduleInfo.functions[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        s_functionIndex[name].push_back(i);
    }
    
    for (size_t i = 0; i < moduleInfo.globalVariables.size(); i++) {
        std::wstring name = moduleInfo.globalVariables[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        s_variableIndex[name].push_back(i);
    }
    
    for (size_t i = 0; i < moduleInfo.classes.size(); i++) {
        std::wstring name = moduleInfo.classes[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        s_typeIndex[name].push_back(i);
    }
    
    for (size_t i = 0; i < moduleInfo.structs.size(); i++) {
        std::wstring name = moduleInfo.structs[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        s_typeIndex[name].push_back(i);
    }
    
    for (size_t i = 0; i < moduleInfo.enums.size(); i++) {
        std::wstring name = moduleInfo.enums[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        s_typeIndex[name].push_back(i);
    }
}

std::vector<size_t> SearchOptimizer::Search(const std::wstring& query, bool searchFunctions, bool searchVariables, bool searchTypes) {
    std::wstring lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
    
    for (const auto& entry : s_cache) {
        if (entry.query == lowerQuery) {
            return entry.results;
        }
    }
    
    std::vector<size_t> results;
    
    if (searchFunctions) {
        for (const auto& pair : s_functionIndex) {
            if (pair.first.find(lowerQuery) != std::wstring::npos) {
                results.insert(results.end(), pair.second.begin(), pair.second.end());
            }
        }
    }
    
    if (searchVariables) {
        for (const auto& pair : s_variableIndex) {
            if (pair.first.find(lowerQuery) != std::wstring::npos) {
                results.insert(results.end(), pair.second.begin(), pair.second.end());
            }
        }
    }
    
    if (searchTypes) {
        for (const auto& pair : s_typeIndex) {
            if (pair.first.find(lowerQuery) != std::wstring::npos) {
                results.insert(results.end(), pair.second.begin(), pair.second.end());
            }
        }
    }
    
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());
    
    if (s_cache.size() >= MAX_CACHE_SIZE) {
        s_cache.erase(s_cache.begin());
    }
    
    SearchCacheEntry entry;
    entry.query = lowerQuery;
    entry.results = results;
    entry.timestamp = GetTickCount();
    s_cache.push_back(entry);
    
    return results;
}

void SearchOptimizer::ClearCache() {
    s_cache.clear();
}
