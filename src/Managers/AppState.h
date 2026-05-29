#pragma once

#include <windows.h>
#include <string>
#include "../PDB/PDBData.h"

class AppState {
public:
    static AppState& GetInstance();
    
    ModuleInfo& GetModuleInfo() { return m_moduleInfo; }
    const ModuleInfo& GetModuleInfo() const { return m_moduleInfo; }
    
    bool IsPdbLoaded() const { return m_pdbLoaded; }
    void SetPdbLoaded(bool loaded) { m_pdbLoaded = loaded; }
    
    HWND GetMainWindow() const { return m_hMainWindow; }
    void SetMainWindow(HWND hWnd) { m_hMainWindow = hWnd; }
    
    HWND GetTreeView() const { return m_hTreeView; }
    void SetTreeView(HWND hWnd) { m_hTreeView = hWnd; }
    
    HWND GetListView() const { return m_hListView; }
    void SetListView(HWND hWnd) { m_hListView = hWnd; }
    
    HWND GetRichEdit() const { return m_hRichEdit; }
    void SetRichEdit(HWND hWnd) { m_hRichEdit = hWnd; }
    
    HWND GetTabCtrl() const { return m_hTabCtrl; }
    void SetTabCtrl(HWND hWnd) { m_hTabCtrl = hWnd; }
    
    HWND GetSearchEdit() const { return m_hSearchEdit; }
    void SetSearchEdit(HWND hWnd) { m_hSearchEdit = hWnd; }
    
    HWND GetSearchButton() const { return m_hSearchButton; }
    void SetSearchButton(HWND hWnd) { m_hSearchButton = hWnd; }
    
    HWND GetStatusBar() const { return m_hStatusBar; }
    void SetStatusBar(HWND hWnd) { m_hStatusBar = hWnd; }
    
    void Clear();
    
private:
    AppState();
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
    
    ModuleInfo m_moduleInfo;
    bool m_pdbLoaded;
    HWND m_hMainWindow;
    HWND m_hTreeView;
    HWND m_hListView;
    HWND m_hRichEdit;
    HWND m_hTabCtrl;
    HWND m_hSearchEdit;
    HWND m_hSearchButton;
    HWND m_hStatusBar;
};