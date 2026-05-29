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
#include <shlobj.h>
#include <sstream>
#include <winreg.h>
#include <richedit.h>

#define MAX_LOADSTRING 100

#include "Resource.h"

#ifndef ID_TREEVIEW
#define ID_TREEVIEW 1001
#endif

#ifndef ID_TABCTRL
#define ID_TABCTRL 1002
#endif

#ifndef ID_LISTVIEW
#define ID_LISTVIEW 1003
#endif

#ifndef ID_RICHEDIT
#define ID_RICHEDIT 1004
#endif

#ifndef ID_STATUSBAR
#define ID_STATUSBAR 1005
#endif

#ifndef ID_EDIT_SEARCH
#define ID_EDIT_SEARCH 1006
#endif

#ifndef ID_BUTTON_SEARCH
#define ID_BUTTON_SEARCH 1007
#endif

#ifndef ID_BUTTON_SEARCH_HISTORY
#define ID_BUTTON_SEARCH_HISTORY 3011
#endif

#ifndef ID_INFO_TEXT
#define ID_INFO_TEXT 3012
#endif



#ifndef ID_MENU_OPEN
#define ID_MENU_OPEN 2001
#endif

#ifndef ID_MENU_OPEN_DLL
#define ID_MENU_OPEN_DLL 2011
#endif

#ifndef ID_MENU_EXPORT_CSV
#define ID_MENU_EXPORT_CSV 2002
#endif

#ifndef ID_MENU_EXPORT_XML
#define ID_MENU_EXPORT_XML 2003
#endif

#ifndef ID_MENU_EXPORT_FUNCTIONS_CSV
#define ID_MENU_EXPORT_FUNCTIONS_CSV 2004
#endif

#ifndef ID_MENU_EXPORT_CLASSES_CSV
#define ID_MENU_EXPORT_CLASSES_CSV 2005
#endif

#ifndef ID_MENU_EXPORT_HEADER
#define ID_MENU_EXPORT_HEADER 2006
#endif

#ifndef ID_MENU_EXPORT_ALL_HEADERS
#define ID_MENU_EXPORT_ALL_HEADERS 2007
#endif

#ifndef ID_MENU_EXIT
#define ID_MENU_EXIT 2008
#endif

#ifndef ID_OFFSET_HEX
#define ID_OFFSET_HEX 3001
#endif

#ifndef ID_OFFSET_DEC
#define ID_OFFSET_DEC 3002
#endif

#ifndef ID_OFFSET_BOTH
#define ID_OFFSET_BOTH 3003
#endif

#ifndef ID_EXPAND_BASE_CLASSES
#define ID_EXPAND_BASE_CLASSES 3004
#endif

#ifndef ID_LANGUAGE_ZH_CN
#define ID_LANGUAGE_ZH_CN 3005
#endif

#ifndef ID_LANGUAGE_EN_US
#define ID_LANGUAGE_EN_US 3006
#endif

#ifndef ID_ASSOCIATE_PDB
#define ID_ASSOCIATE_PDB 3007
#endif

#ifndef ID_UNASSOCIATE_PDB
#define ID_UNASSOCIATE_PDB 3008
#endif

#ifndef ID_MENU_EXPORT_ENUMS_H
#define ID_MENU_EXPORT_ENUMS_H 2009
#endif

#ifndef ID_MENU_CLOSE
#define ID_MENU_CLOSE 2010
#endif

#ifndef ID_SEARCH_HISTORY_FIRST
#define ID_SEARCH_HISTORY_FIRST 4000
#endif

#ifndef ID_SEARCH_HISTORY_LAST
#define ID_SEARCH_HISTORY_LAST 4099
#endif

#ifndef ID_CLEAR_SEARCH_HISTORY
#define ID_CLEAR_SEARCH_HISTORY 4100
#endif

#ifndef ID_NUMBER_HEX
#define ID_NUMBER_HEX 3101
#endif

#ifndef ID_NUMBER_DEC
#define ID_NUMBER_DEC 3102
#endif

#ifndef ID_NUMBER_BOTH
#define ID_NUMBER_BOTH 3103
#endif

#ifndef ID_MENU_SETTINGS
#define ID_MENU_SETTINGS 3004
#endif

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];

extern HWND hTreeView;
extern HWND hTabCtrl;
extern HWND hListView;
extern HWND hRichEdit;
extern HWND hStatusBar;
extern HWND hEditSearch;
extern HWND hButtonSearch;
extern HWND hButtonSearchHistory;
extern HWND hSplitter;
extern HWND hInfoText;

extern bool g_splitterDragging;
extern int g_splitterPos;

extern WCHAR g_szTabText1[64];
extern WCHAR g_szTabText2[64];
extern int g_currentTabIndex;

extern PDBParser g_parser;
extern ModuleInfo g_moduleInfo;
extern bool g_pdbLoaded;
extern NumberDisplayMode g_numberMode;
extern bool g_expandBaseClasses;
extern std::wstring g_currentLanguage;

extern WNDPROC g_pOldEditProc;
extern WNDPROC g_pOldRichEditProc;
extern WNDPROC g_pOldSplitterProc;
extern WNDPROC g_pOldListViewProc;

extern int g_lastClickedSubItem;
