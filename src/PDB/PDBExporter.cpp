#include "PDBExporter.h"
#include "PDBParser.h"
#include "PDBHeaderGenerator.h"
#include "LanguageManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

const ClassInfo* PDBExporter::FindClassInfo(const std::wstring& className, const ModuleInfo* moduleInfo) {
    if (!moduleInfo) return nullptr;
    
    for (const auto& cls : moduleInfo->classes) {
        if (cls.name == className) return &cls;
    }
    for (const auto& str : moduleInfo->structs) {
        if (str.name == className) return &str;
    }
    for (const auto& uni : moduleInfo->unions) {
        if (uni.name == className) return &uni;
    }
    return nullptr;
}

bool PDBExporter::ExportToCSV(const ModuleInfo& moduleInfo, const std::wstring& filePath, bool expandBaseClasses, const std::wstring& language) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    bool isChinese = (language == L"zh-CN");
    
    file << (isChinese ? L"=== Functions ===\n" : L"=== Functions ===\n");
    file << (isChinese ? L"Name,Undecorated Name,Return Type,Calling Convention,RVA,Virtual Address,Size,Is Static,Is Virtual,Class Name\n" : L"Name,Undecorated Name,Return Type,Calling Convention,RVA,Virtual Address,Size,Is Static,Is Virtual,Class Name\n");
    for (const auto& func : moduleInfo.functions) {
        std::wstring paramsStr;
        for (size_t i = 0; i < func.parameters.size(); ++i) {
            if (i > 0) paramsStr += L"; ";
            paramsStr += func.parameters[i].type + L" " + func.parameters[i].name;
        }
        
        file << EscapeCSV(func.name) << L","
             << EscapeCSV(func.undecoratedName) << L","
             << EscapeCSV(func.returnType) << L","
             << EscapeCSV(PDBParser::CallingConventionToString(func.callingConvention)) << L","
             << std::hex << std::showbase << func.rva << L","
             << std::hex << std::showbase << func.virtualAddress << L","
             << func.size << L","
             << (func.isStatic ? L"true" : L"false") << L","
             << (func.isVirtual ? L"true" : L"false") << L","
             << EscapeCSV(func.className) << L"\n";
    }

    file << (isChinese ? L"\n=== Classes ===\n" : L"\n=== Classes ===\n");
    file << (isChinese ? L"Name,Size,Alignment,Base Classes,Member Count,Virtual Function Count\n" : L"Name,Size,Alignment,Base Classes,Member Count,Virtual Function Count\n");
    for (const auto& cls : moduleInfo.classes) {
        std::wstring baseClassesStr;
        for (size_t i = 0; i < cls.baseClasses.size(); ++i) {
            if (i > 0) baseClassesStr += L"; ";
            baseClassesStr += cls.baseClasses[i].name;
        }
        
        file << EscapeCSV(cls.name) << L"," 
             << cls.size << L"," 
             << cls.alignment << L"," 
             << EscapeCSV(baseClassesStr) << L"," 
             << cls.members.size() << L"," 
             << cls.virtualFunctions.size() << L"\n";
    }

    file << (isChinese ? L"\n=== Virtual Functions ===\n" : L"\n=== Virtual Functions ===\n");
    file << (isChinese ? L"Class Name,Function Name,Return Type,Parameters,RVA,Virtual Address,VTable Index\n" : L"Class Name,Function Name,Return Type,Parameters,RVA,Virtual Address,VTable Index\n");
    for (const auto& cls : moduleInfo.classes) {
        for (const auto& vfunc : cls.virtualFunctions) {
            std::wstring paramsStr;
            for (size_t i = 0; i < vfunc.parameters.size(); ++i) {
                if (i > 0) paramsStr += L"; ";
                paramsStr += vfunc.parameters[i].type + L" " + vfunc.parameters[i].name;
            }
            
            file << EscapeCSV(cls.name) << L"," 
                 << EscapeCSV(vfunc.name) << L"," 
                 << EscapeCSV(vfunc.returnType) << L"," 
                 << EscapeCSV(paramsStr) << L"," 
                 << std::hex << std::showbase << vfunc.rva << L"," 
                 << std::hex << std::showbase << vfunc.virtualAddress << L"," 
                 << vfunc.vtableIndex << L"\n";
        }
    }

    file << (isChinese ? L"\n=== Global Variables ===\n" : L"\n=== Global Variables ===\n");
    file << (isChinese ? L"Name,Type,RVA,Virtual Address,Size\n" : L"Name,Type,RVA,Virtual Address,Size\n");
    for (const auto& var : moduleInfo.globalVariables) {
        file << EscapeCSV(var.name) << L","
             << EscapeCSV(var.type) << L","
             << std::hex << std::showbase << var.rva << L","
             << std::hex << std::showbase << var.virtualAddress << L","
             << var.size << L"\n";
    }

    return true;
}

bool PDBExporter::ExportFunctionsToCSV(const std::vector<FunctionInfo>& functions, const std::wstring& filePath, const std::wstring& language) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    bool isChinese = (language == L"zh-CN");

    file << (isChinese ? L"Name,Undecorated Name,Return Type,Calling Convention,Parameters,RVA,Virtual Address,Size,Is Static,Is Virtual,Class Name\n" : L"Name,Undecorated Name,Return Type,Calling Convention,Parameters,RVA,Virtual Address,Size,Is Static,Is Virtual,Class Name\n");
    for (const auto& func : functions) {
        std::wstring paramsStr;
        for (size_t i = 0; i < func.parameters.size(); ++i) {
            if (i > 0) paramsStr += L"; ";
            paramsStr += func.parameters[i].type + L" " + func.parameters[i].name;
        }

        file << EscapeCSV(func.name) << L","
             << EscapeCSV(func.undecoratedName) << L","
             << EscapeCSV(func.returnType) << L","
             << EscapeCSV(PDBParser::CallingConventionToString(func.callingConvention)) << L","
             << EscapeCSV(paramsStr) << L","
             << std::hex << std::showbase << func.rva << L","
             << std::hex << std::showbase << func.virtualAddress << L","
             << func.size << L","
             << (func.isStatic ? L"true" : L"false") << L","
             << (func.isVirtual ? L"true" : L"false") << L","
             << EscapeCSV(func.className) << L"\n";
    }

    return true;
}

bool PDBExporter::ExportClassesToCSV(const std::vector<ClassInfo>& classes, const std::wstring& filePath, bool expandBaseClasses, const std::wstring& language, const ModuleInfo* moduleInfo) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    bool isChinese = (language == L"zh-CN");

    file << (isChinese
        ? L"Class Name,Member Name,Member Type,Offset,Access,Bit Position,Bit Size,Is Static\n"
        : L"Class Name,Member Name,Member Type,Offset,Access,Bit Position,Bit Size,Is Static\n");
    for (const auto& cls : classes) {
        if (expandBaseClasses && !cls.baseClasses.empty() && moduleInfo) {
            for (const auto& base : cls.baseClasses) {
                const ClassInfo* baseClass = FindClassInfo(base.name, moduleInfo);
                if (baseClass) {
                    for (const auto& member : baseClass->members) {
                        file << EscapeCSV(cls.name) << L","
                             << EscapeCSV(baseClass->name + L"::" + member.name) << L","
                             << EscapeCSV(member.type) << L","
                             << PDBHeaderGenerator::ResolveOffset(member.offset, base.inheritanceType == PDB_INHERITANCE_VIRTUAL ? -1 : base.offset) << L","
                             << EscapeCSV(PDBParser::AccessTypeToString(member.access)) << L","
                             << member.bitPosition << L","
                             << member.bitSize << L","
                             << (member.isStatic ? L"true" : L"false") << L"\n";
                    }
                }
            }
        }
        for (const auto& member : cls.members) {
            file << EscapeCSV(cls.name) << L","
                 << EscapeCSV(member.name) << L","
                 << EscapeCSV(member.type) << L","
                 << member.offset << L","
                 << EscapeCSV(PDBParser::AccessTypeToString(member.access)) << L","
                 << member.bitPosition << L","
                 << member.bitSize << L","
                 << (member.isStatic ? L"true" : L"false") << L"\n";
        }
    }

    return true;
}

bool PDBExporter::ExportGlobalVariablesToCSV(const std::vector<GlobalVariableInfo>& variables, const std::wstring& filePath, const std::wstring& language) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    bool isChinese = (language == L"zh-CN");

    file << (isChinese ? L"Name,Type,RVA,Virtual Address,Size\n" : L"Name,Type,RVA,Virtual Address,Size\n");
    for (const auto& var : variables) {
        file << EscapeCSV(var.name) << L","
             << EscapeCSV(var.type) << L","
             << std::hex << std::showbase << var.rva << L","
             << std::hex << std::showbase << var.virtualAddress << L","
             << var.size << L"\n";
    }

    return true;
}

bool PDBExporter::ExportToXML(const ModuleInfo& moduleInfo, const std::wstring& filePath, bool expandBaseClasses) {
    std::wofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << L"<PdbInfo comment=\"\">\n";

    file << L"  <Module name=\"" << EscapeXML(moduleInfo.name) << L"\" comment=\"\">\n";

    file << L"    <Functions comment=\"\">\n";
    for (const auto& func : moduleInfo.functions) {
        std::wstringstream ssRVA, ssVA, ssSize;
        ssRVA << std::hex << std::showbase << func.rva;
        ssVA << std::hex << std::showbase << func.virtualAddress;
        ssSize << func.size;

        file << L"      <Function name=\"" << EscapeXML(func.name) 
             << L"\" undecoratedName=\"" << EscapeXML(func.undecoratedName)
             << L"\" returnType=\"" << EscapeXML(func.returnType)
             << L"\" callingConvention=\"" << EscapeXML(PDBParser::CallingConventionToString(func.callingConvention))
             << L"\" rva=\"" << ssRVA.str()
             << L"\" virtualAddress=\"" << ssVA.str()
             << L"\" size=\"" << ssSize.str()
             << L"\" isStatic=\"" << (func.isStatic ? L"true" : L"false")
             << L"\" isVirtual=\"" << (func.isVirtual ? L"true" : L"false")
             << L"\" className=\"" << EscapeXML(func.className)
             << L"\" comment=\"\">\n";

        file << L"        <Parameters comment=\"\">\n";
        for (const auto& param : func.parameters) {
            file << L"          <Parameter name=\"" << EscapeXML(param.name)
                 << L"\" type=\"" << EscapeXML(param.type)
                 << L"\" comment=\"\"/>\n";
        }
        file << L"        </Parameters>\n";
        file << L"      </Function>\n";
    }
    file << L"    </Functions>\n";

    file << L"    <Classes comment=\"\">\n";
    for (const auto& cls : moduleInfo.classes) {
        std::wstringstream ssSize, ssAlign;
        ssSize << cls.size;
        ssAlign << cls.alignment;

        file << L"      <Class name=\"" << EscapeXML(cls.name)
             << L"\" size=\"" << ssSize.str()
             << L"\" alignment=\"" << ssAlign.str()
             << L"\" comment=\"\">\n";

        file << L"        <BaseClasses comment=\"\">\n";
        for (const auto& base : cls.baseClasses) {
            std::wstring inheritanceType = (base.inheritanceType == PDB_INHERITANCE_VIRTUAL) ? L"virtual" : L"normal";
            file << L"          <BaseClass name=\"" << EscapeXML(base.name)
                 << L"\" inheritanceType=\"" << inheritanceType
                 << L"\" offset=\"" << (base.inheritanceType == PDB_INHERITANCE_VIRTUAL
                     ? -1 : PDBHeaderGenerator::ResolveOffset(base.offset))
                 << L"\" access=\"" << EscapeXML(PDBParser::AccessTypeToString(base.access))
                 << L"\" comment=\"\"/>\n";
        }
        file << L"        </BaseClasses>\n";

        file << L"        <Members comment=\"\">\n";
        if (expandBaseClasses && !cls.baseClasses.empty()) {
            for (const auto& base : cls.baseClasses) {
                const ClassInfo* baseClass = FindClassInfo(base.name, &moduleInfo);
                if (baseClass) {
                    for (const auto& member : baseClass->members) {
                        file << L"          <Member name=\"" << EscapeXML(baseClass->name + L"::" + member.name)
                             << L"\" type=\"" << EscapeXML(member.type)
                             << L"\" offset=\"" << PDBHeaderGenerator::ResolveOffset(member.offset, base.inheritanceType == PDB_INHERITANCE_VIRTUAL ? -1 : base.offset)
                             << L"\" access=\"" << EscapeXML(PDBParser::AccessTypeToString(member.access))
                             << L"\" bitPosition=\"" << member.bitPosition
                             << L"\" bitSize=\"" << member.bitSize
                             << L"\" isStatic=\"" << (member.isStatic ? L"true" : L"false")
                             << L"\" isBaseClass=\"true\""
                             << L"\" comment=\"\"/>\n";
                    }
                }
            }
        }
        for (const auto& member : cls.members) {
            file << L"          <Member name=\"" << EscapeXML(member.name)
                 << L"\" type=\"" << EscapeXML(member.type)
                 << L"\" offset=\"" << member.offset
                 << L"\" access=\"" << EscapeXML(PDBParser::AccessTypeToString(member.access))
                 << L"\" bitPosition=\"" << member.bitPosition
                 << L"\" bitSize=\"" << member.bitSize
                 << L"\" isStatic=\"" << (member.isStatic ? L"true" : L"false")
                 << L"\" comment=\"\"/>\n";
        }
        file << L"        </Members>\n";

        if (!cls.virtualFunctions.empty()) {
            file << L"        <VirtualFunctions comment=\"\">\n";
            for (const auto& vfunc : cls.virtualFunctions) {
                std::wstringstream ssRVA, ssVA, ssIndex;
                ssRVA << std::hex << std::showbase << vfunc.rva;
                ssVA << std::hex << std::showbase << vfunc.virtualAddress;
                ssIndex << vfunc.vtableIndex;

                file << L"          <VirtualFunction name=\"" << EscapeXML(vfunc.name)
                     << L"\" returnType=\"" << EscapeXML(vfunc.returnType)
                     << L"\" rva=\"" << ssRVA.str()
                     << L"\" virtualAddress=\"" << ssVA.str()
                     << L"\" vtableIndex=\"" << ssIndex.str()
                     << L"\" comment=\"\">\n";

                file << L"            <Parameters comment=\"\">\n";
                for (const auto& param : vfunc.parameters) {
                    file << L"              <Parameter name=\"" << EscapeXML(param.name)
                         << L"\" type=\"" << EscapeXML(param.type)
                         << L"\" comment=\"\"/\n";
                }
                file << L"            </Parameters>\n";
                file << L"          </VirtualFunction>\n";
            }
            file << L"        </VirtualFunctions>\n";
        }

        file << L"      </Class>\n";
    }
    file << L"    </Classes>\n";

    file << L"    <Structs comment=\"\">\n";
    for (const auto& str : moduleInfo.structs) {
        std::wstringstream ssSize, ssAlign;
        ssSize << str.size;
        ssAlign << str.alignment;

        file << L"      <Struct name=\"" << EscapeXML(str.name)
             << L"\" size=\"" << ssSize.str()
             << L"\" alignment=\"" << ssAlign.str()
             << L"\" comment=\"\">\n";

        file << L"        <Members comment=\"\">\n";
        if (expandBaseClasses && !str.baseClasses.empty()) {
            for (const auto& base : str.baseClasses) {
                const ClassInfo* baseClass = FindClassInfo(base.name, &moduleInfo);
                if (baseClass) {
                    for (const auto& member : baseClass->members) {
                        file << L"          <Member name=\"" << EscapeXML(baseClass->name + L"::" + member.name)
                             << L"\" type=\"" << EscapeXML(member.type)
                             << L"\" offset=\"" << PDBHeaderGenerator::ResolveOffset(member.offset, base.inheritanceType == PDB_INHERITANCE_VIRTUAL ? -1 : base.offset)
                             << L"\" access=\"" << EscapeXML(PDBParser::AccessTypeToString(member.access))
                             << L"\" isStatic=\"" << (member.isStatic ? L"true" : L"false")
                             << L"\" comment=\"\"/>\n";
                    }
                }
            }
        }
        for (const auto& member : str.members) {
            file << L"          <Member name=\"" << EscapeXML(member.name)
                 << L"\" type=\"" << EscapeXML(member.type)
                 << L"\" offset=\"" << member.offset
                 << L"\" access=\"" << EscapeXML(PDBParser::AccessTypeToString(member.access))
                 << L"\" isStatic=\"" << (member.isStatic ? L"true" : L"false")
                 << L"\" comment=\"\"/>\n";
        }
        file << L"        </Members>\n";
        file << L"      </Struct>\n";
    }
    file << L"    </Structs>\n";

    file << L"    <GlobalVariables comment=\"\">\n";
    for (const auto& var : moduleInfo.globalVariables) {
        std::wstringstream ssRVA, ssVA, ssSize;
        ssRVA << std::hex << std::showbase << var.rva;
        ssVA << std::hex << std::showbase << var.virtualAddress;
        ssSize << var.size;

        file << L"      <Variable name=\"" << EscapeXML(var.name)
             << L"\" type=\"" << EscapeXML(var.type)
             << L"\" rva=\"" << ssRVA.str()
             << L"\" virtualAddress=\"" << ssVA.str()
             << L"\" size=\"" << ssSize.str()
             << L"\" comment=\"\"/>\n";
    }
    file << L"    </GlobalVariables>\n";

    file << L"  </Module>\n";
    file << L"</PdbInfo>\n";

    return true;
}

std::wstring PDBExporter::EscapeCSV(const std::wstring& str) {
    std::wstring result = str;
    bool needsQuotes = false;

    if (result.find(L',') != std::wstring::npos ||
        result.find(L'"') != std::wstring::npos ||
        result.find(L'\n') != std::wstring::npos) {
        needsQuotes = true;
    }

    size_t pos = 0;
    while ((pos = result.find(L'"', pos)) != std::wstring::npos) {
        result.insert(pos, L"\"");
        pos += 2;
    }

    if (needsQuotes) {
        result = L"\"" + result + L"\"";
    }

    return result;
}

std::wstring PDBExporter::EscapeXML(const std::wstring& str) {
    std::wstring result;
    for (wchar_t c : str) {
        switch (c) {
            case L'<': result += L"&lt;"; break;
            case L'>': result += L"&gt;"; break;
            case L'&': result += L"&amp;"; break;
            case L'"': result += L"&quot;"; break;
            case L'\'': result += L"&apos;"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::wstring PDBExporter::GetCurrentDateTime() {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_s(&tm_now, &now);

    std::wstringstream ss;
    ss << std::put_time(&tm_now, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}
