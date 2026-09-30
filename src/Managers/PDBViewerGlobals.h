#pragma once

#include "framework.h"
#include "PDBData.h"
#include "PDBParser.h"
#include "PDBExporter.h"
#include "PDBHeaderGenerator.h"
#include "LanguageManager.h"
#include "ConfigManager.h"
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <shlobj.h>
#include <sstream>
#include <winreg.h>
#include <richedit.h>

#define MAX_LOADSTRING 100

#include "Resource.h"

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];
extern HWND g_hMainWindow;

extern HWND hTreeView;
extern HWND hTabCtrl;
extern HWND hListView;
extern HWND hRichEdit;
extern HWND hStatusBar;
extern HWND hEditSearch;
extern HWND hButtonSearch;
extern HWND hButtonSearchHistory;
extern HWND hButtonClearSearch;
extern HWND hButtonCopyHeader;
extern HWND hButtonCancelTask;
extern HWND hProgressTask;
extern HWND hTooltip;
extern HWND hSplitter;

extern bool g_splitterDragging;
extern int g_splitterPos;

extern WCHAR g_szTabText1[64];
extern WCHAR g_szTabText2[64];
extern int g_currentTabIndex;
extern int g_themeMode;

extern ModuleInfo g_moduleInfo;
extern bool g_pdbLoaded;
extern NumberDisplayMode g_numberMode;
extern bool g_expandBaseClasses;
extern std::wstring g_currentLanguage;

extern WNDPROC g_pOldEditProc;
extern WNDPROC g_pOldClearButtonProc;
extern WNDPROC g_pOldRichEditProc;
extern WNDPROC g_pOldSplitterProc;
extern WNDPROC g_pOldListViewProc;

extern int g_lastClickedSubItem;
