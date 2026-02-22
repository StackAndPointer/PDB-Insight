#include "ExportManager.h"
#include "ExportEnhancer.h"

void ExportToCSV(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"export.csv";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"CSV 文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (GetSaveFileNameW(&ofn))
    {
        if (PDBExporter::ExportToCSV(g_moduleInfo, ofn.lpstrFile, g_expandBaseClasses, g_currentLanguage))
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportToXML(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"export.xml";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"XML 文件 (*.xml)\0*.xml\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"xml";

    if (GetSaveFileNameW(&ofn))
    {
        if (PDBExporter::ExportToXML(g_moduleInfo, ofn.lpstrFile, g_expandBaseClasses))
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportFunctionsToCSV(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"functions.csv";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"CSV 文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (GetSaveFileNameW(&ofn))
    {
        if (PDBExporter::ExportFunctionsToCSV(g_moduleInfo.functions, ofn.lpstrFile, g_currentLanguage))
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportClassesToCSV(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"classes.csv";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"CSV 文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (GetSaveFileNameW(&ofn))
    {
        std::vector<ClassInfo> allClasses;
        allClasses.insert(allClasses.end(), g_moduleInfo.classes.begin(), g_moduleInfo.classes.end());
        allClasses.insert(allClasses.end(), g_moduleInfo.structs.begin(), g_moduleInfo.structs.end());
        allClasses.insert(allClasses.end(), g_moduleInfo.unions.begin(), g_moduleInfo.unions.end());

        if (PDBExporter::ExportClassesToCSV(allClasses, ofn.lpstrFile, g_expandBaseClasses, g_currentLanguage, &g_moduleInfo))
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportHeader(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
    if (!hSelected)
    {
        MessageBoxW(hWnd, L"请在树视图中选择一个类或结构体", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    TVITEM tvi;
    tvi.hItem = hSelected;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(hTreeView, &tvi);
    DWORD param = (DWORD)tvi.lParam;

    const ClassInfo* pClass = nullptr;
    if (param >= 3000 && param < 4000)
    {
        size_t idx = param - 3000;
        if (idx < g_moduleInfo.classes.size())
        {
            pClass = &g_moduleInfo.classes[idx];
        }
    }
    else if (param >= 4000 && param < 5000)
    {
        size_t idx = param - 4000;
        if (idx < g_moduleInfo.structs.size())
        {
            pClass = &g_moduleInfo.structs[idx];
        }
    }
    else if (param >= 5000 && param < 6000)
    {
        size_t idx = param - 5000;
        if (idx < g_moduleInfo.unions.size())
        {
            pClass = &g_moduleInfo.unions[idx];
        }
    }

    if (!pClass)
    {
        MessageBoxW(hWnd, L"请选择一个有效的类、结构体或联合体", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"";
    const auto& settings = ConfigManager::GetInstance().GetExportSettings();
    std::wstring fileName = settings.flattenNamespaces ? PDBHeaderGenerator::FlattenName(pClass->name) : pClass->name;
    wcscpy_s(szFile, (fileName + L".h").c_str());

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"头文件 (*.h)\0*.h\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"h";

    if (GetSaveFileNameW(&ofn))
    {
        bool success = PDBHeaderGenerator::GenerateClassHeader(*pClass, ofn.lpstrFile, settings, g_numberMode, g_expandBaseClasses, &g_moduleInfo);

        if (success)
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ExportAllHeaders(HWND hWnd)
{
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    BROWSEINFOW bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hWnd;
    bi.lpszTitle = L"选择保存头文件的目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl)
    {
        WCHAR path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path))
        {
            const auto& settings = ConfigManager::GetInstance().GetExportSettings();
            bool success = PDBHeaderGenerator::GenerateAllHeaders(g_moduleInfo, path, settings, g_numberMode, g_expandBaseClasses);

            if (success)
            {
                MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(hWnd, L"部分文件导出失败!", L"警告", MB_OK | MB_ICONWARNING);
            }
        }
        CoTaskMemFree(pidl);
    }
}

void ExportEnumsHeader(HWND hWnd) {
    if (!g_pdbLoaded)
    {
        MessageBoxW(hWnd, L"请先打开一个 PDB 文件", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn;
    WCHAR szFile[MAX_PATH] = L"Enums.h";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"头文件 (*.h)\0*.h\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"h";

    if (GetSaveFileNameW(&ofn))
    {
        const auto& settings = ConfigManager::GetInstance().GetExportSettings();
        bool success = PDBHeaderGenerator::GenerateEnumsHeader(g_moduleInfo, ofn.lpstrFile, settings, g_numberMode, g_expandBaseClasses);

        if (success)
        {
            MessageBoxW(hWnd, L"导出成功!", L"成功", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(hWnd, L"导出失败!", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}
