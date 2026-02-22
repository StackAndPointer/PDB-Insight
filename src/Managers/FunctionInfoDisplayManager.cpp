#include "FunctionInfoDisplayManager.h"
#include <sstream>
#include <iomanip>

std::wstring FunctionInfoDisplayManager::GetCallingConventionString(CallingConvention conv) {
    switch (conv) {
        case PDB_CALL_CDECL:
            return L"__cdecl";
        case PDB_CALL_STDCALL:
            return L"__stdcall";
        case PDB_CALL_FASTCALL:
            return L"__fastcall";
        case PDB_CALL_VECTORCALL:
            return L"__vectorcall";
        case PDB_CALL_THISCALL:
            return L"__thiscall";
        default:
            return L"Unknown";
    }
}

std::wstring FunctionInfoDisplayManager::FormatCompletenessLabel(int percent) {
    std::wostringstream ss;
    ss << percent << L"% complete";
    return ss.str();
}

bool FunctionInfoDisplayManager::IsInfoComplete(const FunctionInfo& func) {
    return !func.returnType.empty() && !func.parameters.empty();
}

int FunctionInfoDisplayManager::CalculateCompletenessPercent(const FunctionInfo& func) {
    int score = 0;
    int total = 5;
    
    if (!func.name.empty()) score++;
    if (func.virtualAddress != 0) score++;
    if (!func.returnType.empty()) score++;
    if (!func.parameters.empty()) score++;
    if (func.callingConvention != PDB_CALL_UNKNOWN) score++;
    
    return (score * 100) / total;
}

FunctionDisplayInfo FunctionInfoDisplayManager::GetDisplayInfo(const FunctionInfo& func) {
    FunctionDisplayInfo info;
    
    info.name = func.name.empty() ? L"Unknown" : func.name;
    
    std::wostringstream addrSS;
    addrSS << L"0x" << std::hex << std::uppercase << std::setw(16) << std::setfill(L'0') << func.virtualAddress;
    info.address = func.virtualAddress == 0 ? L"0x0000000000000000" : addrSS.str();
    
    info.returnType = func.returnType.empty() ? L"Unknown" : func.returnType;
    
    std::wostringstream paramSS;
    if (func.parameters.empty()) {
        paramSS << L"()";
    } else {
        paramSS << L"(";
        for (size_t i = 0; i < func.parameters.size(); i++) {
            if (i > 0) {
                paramSS << L", ";
            }
            paramSS << func.parameters[i].type;
            if (!func.parameters[i].name.empty()) {
                paramSS << L" " << func.parameters[i].name;
            }
        }
        paramSS << L")";
    }
    info.parameters = paramSS.str();
    
    info.callingConvention = GetCallingConventionString(func.callingConvention);
    
    std::wostringstream sizeSS;
    sizeSS << func.size;
    info.frameSize = func.size == 0 ? L"Unknown" : sizeSS.str();
    
    info.isComplete = IsInfoComplete(func);
    info.completenessPercent = CalculateCompletenessPercent(func);
    
    return info;
}
