#pragma once

#include "PDBViewerGlobals.h"
#include <unordered_map>
#include <vector>
#include <string>

struct SearchCacheEntry {
    std::wstring query;
    std::vector<size_t> results;
    DWORD timestamp;
};

class SearchOptimizer {
public:
    static void Initialize();
    static void Cleanup();
    
    static void BuildIndex(const ModuleInfo& moduleInfo);
    static std::vector<size_t> Search(const std::wstring& query, bool searchFunctions, bool searchVariables, bool searchTypes);
    
    static void ClearCache();
    
private:
    static std::unordered_map<std::wstring, std::vector<size_t>> s_functionIndex;
    static std::unordered_map<std::wstring, std::vector<size_t>> s_variableIndex;
    static std::unordered_map<std::wstring, std::vector<size_t>> s_typeIndex;
    static std::vector<SearchCacheEntry> s_cache;
    static const size_t MAX_CACHE_SIZE = 10;
};
