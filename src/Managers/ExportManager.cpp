#include "ExportManager.h"
#include "TreeNodeHelper.h"

namespace {
bool EnsureLoaded(HWND owner) {
    if (g_pdbLoaded) return true;
    MessageBoxW(owner, LANG_STR(L"msg_open_pdb_first").c_str(), LANG_STR(L"msg_info").c_str(),
        MB_OK | MB_ICONINFORMATION);
    return false;
}

void ShowResult(HWND owner, bool success, bool partial = false) {
    if (success) {
        MessageBoxW(owner, LANG_STR(L"msg_export_success").c_str(), LANG_STR(L"msg_success").c_str(),
            MB_OK | MB_ICONINFORMATION);
    } else {
        const std::wstring& text = partial ? LANG_STR(L"msg_export_partial") : LANG_STR(L"msg_export_fail");
        MessageBoxW(owner, text.c_str(), partial ? LANG_STR(L"msg_warning").c_str() : LANG_STR(L"msg_error").c_str(),
            MB_OK | (partial ? MB_ICONWARNING : MB_ICONERROR));
    }
}

void AppendFilter(std::wstring& filter, const std::wstring& label, const wchar_t* pattern) {
    filter += label;
    filter.push_back(L'\0');
    filter += pattern;
    filter.push_back(L'\0');
}

std::wstring MakeFilter(const std::wstring& label, const wchar_t* pattern) {
    std::wstring filter;
    AppendFilter(filter, label, pattern);
    AppendFilter(filter, LANG_STR(L"filter_all") + L" (*.*)", L"*.*");
    filter.push_back(L'\0');
    return filter;
}

bool PromptSaveFile(HWND owner, const std::wstring& filter, const std::wstring& defaultFile,
                    const wchar_t* extension, std::wstring& selectedPath) {
    std::vector<WCHAR> file(MAX_PATH);
    wcscpy_s(file.data(), file.size(), defaultFile.c_str());
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = file.data();
    dialog.nMaxFile = static_cast<DWORD>(file.size());
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = extension;
    if (!GetSaveFileNameW(&dialog)) return false;
    selectedPath = file.data();
    return true;
}

const ClassInfo* GetSelectedClass(size_t& index) {
    HTREEITEM selected = TreeView_GetSelection(hTreeView);
    if (!selected) return nullptr;
    TVITEM item{};
    item.mask = TVIF_PARAM;
    item.hItem = selected;
    if (!TreeView_GetItem(hTreeView, &item)) return nullptr;
    DWORD param = static_cast<DWORD>(item.lParam);
    if (!TreeNodeParamHelper::IsClassNode(param) &&
        !TreeNodeParamHelper::IsStructNode(param) &&
        !TreeNodeParamHelper::IsUnionNode(param)) return nullptr;

    index = TreeNodeParamHelper::GetIndex(param);
    if (TreeNodeParamHelper::IsClassNode(param) && index < g_moduleInfo.classes.size()) return &g_moduleInfo.classes[index];
    if (TreeNodeParamHelper::IsStructNode(param) && index < g_moduleInfo.structs.size()) return &g_moduleInfo.structs[index];
    if (TreeNodeParamHelper::IsUnionNode(param) && index < g_moduleInfo.unions.size()) return &g_moduleInfo.unions[index];
    return nullptr;
}
}

void ExportToCSV(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_csv") + L" (*.csv)", L"*.csv");
    if (!PromptSaveFile(owner, filter, L"export.csv", L"csv", path)) return;
    ShowResult(owner, PDBExporter::ExportToCSV(g_moduleInfo, path, g_expandBaseClasses, g_currentLanguage));
}

void ExportToXML(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_xml") + L" (*.xml)", L"*.xml");
    if (!PromptSaveFile(owner, filter, L"export.xml", L"xml", path)) return;
    ShowResult(owner, PDBExporter::ExportToXML(g_moduleInfo, path, g_expandBaseClasses));
}

void ExportFunctionsToCSV(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_csv") + L" (*.csv)", L"*.csv");
    if (!PromptSaveFile(owner, filter, L"functions.csv", L"csv", path)) return;
    ShowResult(owner, PDBExporter::ExportFunctionsToCSV(g_moduleInfo.functions, path, g_currentLanguage));
}

void ExportClassesToCSV(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_csv") + L" (*.csv)", L"*.csv");
    if (!PromptSaveFile(owner, filter, L"classes.csv", L"csv", path)) return;

    std::vector<ClassInfo> classes;
    classes.insert(classes.end(), g_moduleInfo.classes.begin(), g_moduleInfo.classes.end());
    classes.insert(classes.end(), g_moduleInfo.structs.begin(), g_moduleInfo.structs.end());
    classes.insert(classes.end(), g_moduleInfo.unions.begin(), g_moduleInfo.unions.end());
    ShowResult(owner, PDBExporter::ExportClassesToCSV(classes, path, g_expandBaseClasses, g_currentLanguage, &g_moduleInfo));
}

void ExportHeader(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    size_t index = 0;
    const ClassInfo* selectedClass = GetSelectedClass(index);
    if (!selectedClass) {
        MessageBoxW(owner, LANG_STR(L"msg_select_valid_class").c_str(), LANG_STR(L"msg_info").c_str(),
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    const ExportSettings& settings = ConfigManager::GetInstance().GetExportSettings();
    std::wstring defaultName = settings.flattenNamespaces ?
        PDBHeaderGenerator::FlattenName(selectedClass->name) : selectedClass->name;
    defaultName += L".h";
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_header") + L" (*.h)", L"*.h");
    if (!PromptSaveFile(owner, filter, defaultName, L"h", path)) return;
    ShowResult(owner, PDBHeaderGenerator::GenerateClassHeader(*selectedClass, path, settings,
        g_numberMode, g_expandBaseClasses, &g_moduleInfo));
}

void ExportAllHeaders(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    BROWSEINFOW browse{};
    browse.hwndOwner = owner;
    browse.lpszTitle = LANG_STR(L"dlg_save_header").c_str();
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST item = SHBrowseForFolderW(&browse);
    if (!item) return;

    WCHAR path[MAX_PATH] = L"";
    if (SHGetPathFromIDListW(item, path)) {
        bool success = PDBHeaderGenerator::GenerateAllHeaders(g_moduleInfo, path,
            ConfigManager::GetInstance().GetExportSettings(), g_numberMode, g_expandBaseClasses);
        ShowResult(owner, success, !success);
    }
    CoTaskMemFree(item);
}

void ExportEnumsHeader(HWND owner) {
    if (!EnsureLoaded(owner)) return;
    std::wstring path;
    std::wstring filter = MakeFilter(LANG_STR(L"filter_header") + L" (*.h)", L"*.h");
    if (!PromptSaveFile(owner, filter, L"Enums.h", L"h", path)) return;
    ShowResult(owner, PDBHeaderGenerator::GenerateEnumsHeader(g_moduleInfo, path,
        ConfigManager::GetInstance().GetExportSettings(), g_numberMode, g_expandBaseClasses));
}
