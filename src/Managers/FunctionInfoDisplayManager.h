#pragma once

#include "PDBViewerGlobals.h"

struct FunctionDisplayInfo {
    std::wstring name;
    std::wstring address;
    std::wstring returnType;
    std::wstring parameters;
    std::wstring callingConvention;
    std::wstring frameSize;
    bool isComplete;
    int completenessPercent;
};

class FunctionInfoDisplayManager {
public:
    static FunctionDisplayInfo GetDisplayInfo(const FunctionInfo& func);
    static std::wstring GetCallingConventionString(CallingConvention conv);
    static std::wstring FormatCompletenessLabel(int percent);
    static bool IsInfoComplete(const FunctionInfo& func);
    static int CalculateCompletenessPercent(const FunctionInfo& func);
};
