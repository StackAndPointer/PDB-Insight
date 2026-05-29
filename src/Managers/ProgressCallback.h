#pragma once

#include <windows.h>
#include <string>
#include <functional>

class IProgressCallback {
public:
    virtual ~IProgressCallback() = default;
    
    virtual void OnProgress(int percent, const std::wstring& message) = 0;
    
    void ProcessMessages() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

class SimpleProgressCallback : public IProgressCallback {
public:
    using CallbackFunc = std::function<void(int, const std::wstring&)>;
    
    SimpleProgressCallback(CallbackFunc callback, bool processMessages = true)
        : m_callback(callback)
        , m_processMessages(processMessages) {}
    
    void OnProgress(int percent, const std::wstring& message) override {
        if (m_callback) {
            m_callback(percent, message);
        }
        if (m_processMessages) {
            ProcessMessages();
        }
    }
    
private:
    CallbackFunc m_callback;
    bool m_processMessages;
};

class NullProgressCallback : public IProgressCallback {
public:
    void OnProgress(int percent, const std::wstring& message) override {}
};