#include "PDBDownloader.h"
#include "ConfigManager.h"
#include <wininet.h>
#include <sstream>
#include <iomanip>
#include <fstream>

#pragma comment(lib, "wininet.lib")

PDBDownloader& PDBDownloader::GetInstance() {
    static PDBDownloader instance;
    return instance;
}

PDBDownloadResult PDBDownloader::DownloadPDBForDll(const std::wstring& dllPath, const std::wstring& outputPath, std::function<void(int, const std::wstring&)> progressCallback) {
    PDBDownloadResult result;
    
    if (progressCallback) {
        progressCallback(0, L"Reading debug info from DLL/EXE...");
    }
    
    std::wstring pdbName;
    std::wstring guid;
    DWORD age = 0;
    
    if (!ReadPeDebugInfo(dllPath, pdbName, guid, age)) {
        result.errorMessage = L"Failed to read debug info from DLL/EXE file";
        return result;
    }
    
    if (progressCallback) {
        progressCallback(10, L"Debug info found: " + pdbName);
    }
    
    result.pdbGuid = guid;
    result.pdbAge = age;
    
    std::wstring targetPath = outputPath;
    if (targetPath.empty()) {
        targetPath = GetDllDirectory(dllPath) + L"\\";
    }
    targetPath += pdbName;
    
    std::wstring url = BuildMicrosoftSymbolUrl(pdbName, guid, age);
    
    if (progressCallback) {
        progressCallback(20, L"Connecting to Microsoft symbol server...");
    }
    
    if (DownloadFile(url, targetPath, progressCallback)) {
        result.success = true;
        result.pdbPath = targetPath;
    }
    else {
        result.errorMessage = L"Failed to download PDB file from Microsoft symbol server";
    }
    
    return result;
}

bool PDBDownloader::GetDebugInfoFromDll(const std::wstring& dllPath, std::wstring& guid, DWORD& age) {
    std::wstring pdbName;
    return ReadPeDebugInfo(dllPath, pdbName, guid, age);
}

std::wstring PDBDownloader::BuildMicrosoftSymbolUrl(const std::wstring& pdbName, const std::wstring& guid, DWORD age) {
    std::wstring formattedGuid = FormatGuidForUrl(guid, age);
    
    std::wstring baseUrl;
    if (ConfigManager::GetInstance().GetUseMirrorSource()) {
        std::wstring customUrl = ConfigManager::GetInstance().GetMirrorSourceUrl();
        if (!customUrl.empty()) {
            baseUrl = customUrl;
            if (baseUrl.back() != L'/') {
                baseUrl += L'/';
            }
        } else {
            baseUrl = L"https://symbols.yandex.ru/";
        }
    } else {
        baseUrl = L"https://msdl.microsoft.com/download/symbols/";
    }
    
    std::wstring url = baseUrl;
    url += pdbName;
    url += L"/";
    url += formattedGuid;
    url += L"/";
    url += pdbName;
    
    return url;
}

bool PDBDownloader::DownloadFile(const std::wstring& url, const std::wstring& localPath, std::function<void(int, const std::wstring&)> progressCallback) {
    HINTERNET hInternet = InternetOpenW(L"PDBInsight", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }
    
    if (progressCallback) {
        progressCallback(25, L"Opening connection...");
    }
    
    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }
    
    DWORD contentLength = 0;
    DWORD contentLengthSize = sizeof(contentLength);
    BOOL hasContentLength = HttpQueryInfoW(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, NULL);
    
    std::ofstream outFile(localPath, std::ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    char buffer[8192];
    DWORD bytesRead = 0;
    DWORD totalBytesRead = 0;
    bool success = true;
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer, bytesRead);
        totalBytesRead += bytesRead;
        
        if (progressCallback && hasContentLength && contentLength > 0) {
            int percent = 30 + (int)((double)totalBytesRead / contentLength * 70);
            std::wstring status = L"Downloading... " + std::to_wstring(percent) + L"% (" + std::to_wstring(totalBytesRead / 1024) + L" KB)";
            progressCallback(percent, status);
        }
        else if (progressCallback) {
            std::wstring status = L"Downloading... " + std::to_wstring(totalBytesRead / 1024) + L" KB";
            progressCallback(50, status);
        }
        
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    outFile.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    
    if (progressCallback) {
        progressCallback(100, L"Download complete");
    }
    
    return outFile.good();
}

std::wstring PDBDownloader::GetPdbFileNameFromDll(const std::wstring& dllPath) {
    std::wstring pdbName;
    std::wstring guid;
    DWORD age;
    ReadPeDebugInfo(dllPath, pdbName, guid, age);
    return pdbName;
}

std::wstring PDBDownloader::GetDllDirectory(const std::wstring& dllPath) {
    size_t lastSlash = dllPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return dllPath.substr(0, lastSlash);
    }
    return L".";
}

bool PDBDownloader::ReadPeDebugInfo(const std::wstring& dllPath, std::wstring& pdbName, std::wstring& guid, DWORD& age) {
    HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    HANDLE hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) {
        CloseHandle(hFile);
        return false;
    }
    
    LPVOID pBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!pBase) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return false;
    }
    
    bool found = false;
    
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBase;
    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)pBase + pDosHeader->e_lfanew);
        if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE) {
            DWORD debugDirRva = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            DWORD debugDirSize = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
            
            if (debugDirRva != 0) {
                PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
                for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
                    if (debugDirRva >= pSection[i].VirtualAddress && 
                        debugDirRva < pSection[i].VirtualAddress + pSection[i].Misc.VirtualSize) {
                        
                        DWORD fileOffset = debugDirRva - pSection[i].VirtualAddress + pSection[i].PointerToRawData;
                        PIMAGE_DEBUG_DIRECTORY pDebugDir = (PIMAGE_DEBUG_DIRECTORY)((BYTE*)pBase + fileOffset);
                        
                        DWORD numEntries = debugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);
                        for (DWORD j = 0; j < numEntries; j++) {
                            if (pDebugDir[j].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                                DWORD cvOffset = pDebugDir[j].PointerToRawData;
                                DWORD cvSize = pDebugDir[j].SizeOfData;
                                
                                BYTE* pCvData = (BYTE*)pBase + cvOffset;
                                DWORD* pSignature = (DWORD*)pCvData;
                                
                                if (*pSignature == 0x53445352) {
                                    struct CV_INFO_PDB70 {
                                        DWORD Signature;
                                        GUID Guid;
                                        DWORD Age;
                                        char PdbFileName[1];
                                    };
                                    
                                    CV_INFO_PDB70* pPdb70 = (CV_INFO_PDB70*)pCvData;
                                    
                                    std::wstringstream ssGuid;
                                    ssGuid << std::hex << std::setfill(L'0');
                                    ssGuid << std::setw(8) << pPdb70->Guid.Data1 << L"-";
                                    ssGuid << std::setw(4) << pPdb70->Guid.Data2 << L"-";
                                    ssGuid << std::setw(4) << pPdb70->Guid.Data3 << L"-";
                                    for (int k = 0; k < 2; k++) {
                                        ssGuid << std::setw(2) << (int)pPdb70->Guid.Data4[k];
                                    }
                                    ssGuid << L"-";
                                    for (int k = 2; k < 8; k++) {
                                        ssGuid << std::setw(2) << (int)pPdb70->Guid.Data4[k];
                                    }
                                    
                                    guid = ssGuid.str();
                                    age = pPdb70->Age;
                                    
                                    int pdbNameLen = cvSize - sizeof(CV_INFO_PDB70) + 1;
                                    std::string pdbNameA(pPdb70->PdbFileName, pdbNameLen);
                                    pdbNameLen = (int)strlen(pPdb70->PdbFileName);
                                    pdbNameA = std::string(pPdb70->PdbFileName, pdbNameLen);
                                    
                                    pdbName.clear();
                                    for (char c : pdbNameA) {
                                        pdbName += (wchar_t)c;
                                    }
                                    
                                    found = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    
    UnmapViewOfFile(pBase);
    CloseHandle(hMap);
    CloseHandle(hFile);
    
    return found;
}

std::wstring PDBDownloader::FormatGuidForUrl(const std::wstring& guid, DWORD age) {
    std::wstring formatted = guid;
    
    formatted.erase(std::remove(formatted.begin(), formatted.end(), L'-'), formatted.end());
    
    std::wstringstream ss;
    ss << formatted << std::hex << age;
    
    std::wstring result = ss.str();
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = toupper(result[i]);
    }
    
    return result;
}