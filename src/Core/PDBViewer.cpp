#include "framework.h"
#include "PDBViewerGlobals.h"
#include "MainWindow.h"
#include "DPIManager.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    DPIManager::SetProcessDPIAware();

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    if (!LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING)) {
        wcscpy_s(szTitle, L"PDB Insight");
    }
    if (!LoadStringW(hInstance, IDC_PDBVIEWER, szWindowClass, MAX_LOADSTRING)) {
        wcscpy_s(szWindowClass, L"PDBINSIGHT");
    }
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow, lpCmdLine))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PDBVIEWER));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
