#include "framework.h"
#include "CommandLineManager.h"
#include "PDBViewerGlobals.h"
#include "MainWindow.h"
#include "DPIManager.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    DPIManager::SetProcessDPIAware();

    CommandLineManager& commandLine = CommandLineManager::GetInstance();
    if (!commandLine.ParseCommandLine(lpCmdLine)) return 2;
    if (commandLine.ShouldRunCommandLine()) return commandLine.RunCommandLine();

    INITCOMMONCONTROLSEX commonControls{sizeof(INITCOMMONCONTROLSEX),
        ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES |
        ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_WIN95_CLASSES};
    InitCommonControlsEx(&commonControls);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
    LoadStringW(hInstance, IDC_PDBVIEWER, szWindowClass, _countof(szWindowClass));
    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow, lpCmdLine)) return FALSE;

    HACCEL accelerators = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDC_PDBVIEWER));
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_TAB && !GetKeyState(VK_CONTROL)) {
            HWND root = GetAncestor(message.hwnd, GA_ROOT);
            HWND next = GetNextDlgTabItem(root, GetFocus(), GetKeyState(VK_SHIFT) < 0);
            if (next) {
                SetFocus(next);
                continue;
            }
        }
        if (!TranslateAcceleratorW(message.hwnd, accelerators, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return static_cast<int>(message.wParam);
}
