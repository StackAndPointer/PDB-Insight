#include "AppState.h"

AppState& AppState::GetInstance() {
    static AppState instance;
    return instance;
}

AppState::AppState()
    : m_pdbLoaded(false)
    , m_hMainWindow(nullptr)
    , m_hTreeView(nullptr)
    , m_hListView(nullptr)
    , m_hRichEdit(nullptr)
    , m_hTabCtrl(nullptr)
    , m_hSearchEdit(nullptr)
    , m_hSearchButton(nullptr)
    , m_hStatusBar(nullptr) {
}

void AppState::Clear() {
    m_moduleInfo = ModuleInfo();
    m_pdbLoaded = false;
}