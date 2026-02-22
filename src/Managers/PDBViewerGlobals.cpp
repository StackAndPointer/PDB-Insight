#include "PDBViewerGlobals.h"

HINSTANCE hInst = nullptr;
WCHAR szTitle[MAX_LOADSTRING] = { 0 };
WCHAR szWindowClass[MAX_LOADSTRING] = { 0 };

HWND hTreeView = nullptr;
HWND hTabCtrl = nullptr;
HWND hListView = nullptr;
HWND hRichEdit = nullptr;
HWND hStatusBar = nullptr;
HWND hEditSearch = nullptr;
HWND hButtonSearch = nullptr;
HWND hButtonSearchHistory = nullptr;
HWND hSplitter = nullptr;

bool g_splitterDragging = false;
int g_splitterPos = 0;

WCHAR g_szTabText1[64] = L"详细信息";
WCHAR g_szTabText2[64] = L"头文件视图";
int g_currentTabIndex = 0;

PDBParser g_parser;
ModuleInfo g_moduleInfo;
bool g_pdbLoaded = false;
NumberDisplayMode g_numberMode = NUMBER_HEX;
bool g_expandBaseClasses = false;
std::wstring g_currentLanguage = L"zh-CN";

WNDPROC g_pOldEditProc = nullptr;
WNDPROC g_pOldRichEditProc = nullptr;
WNDPROC g_pOldSplitterProc = nullptr;
WNDPROC g_pOldListViewProc = nullptr;

int g_lastClickedSubItem = 0;
