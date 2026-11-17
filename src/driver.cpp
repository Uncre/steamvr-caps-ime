#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "key_policy.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../third_party/openvr/openvr_driver.h"
#pragma GCC diagnostic pop

namespace {
constexpr ULONG_PTR input_tag = 0x53564349; // SVCI
constexpr UINT toggle_message = WM_APP + 71;
HMODULE module_handle{};
HANDLE thread_handle{}, stop_event{}, ready_event{}, instance_mutex{};
DWORD thread_id{}, startup_error{};
HHOOK keyboard_hook{};
capsime::KeyPolicy policy;
FILE* log_file{};

void log(const char* text) {
    if (!log_file) return;
    SYSTEMTIME t{}; GetLocalTime(&t);
    std::fprintf(log_file,"%04u-%02u-%02u %02u:%02u:%02u %s\n",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,text);
    std::fflush(log_file);
}
LRESULT CALLBACK keyboard_callback(int code, WPARAM message, LPARAM data) {
    if (code == HC_ACTION) {
        const auto& key = *reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
        const auto d = policy.process(key.vkCode, (key.flags & LLKHF_INJECTED) != 0,
                                     message == WM_KEYUP || message == WM_SYSKEYUP);
        if (d.consume) {
            if (d.toggle) PostThreadMessageW(thread_id,toggle_message,
                reinterpret_cast<WPARAM>(GetForegroundWindow()),0);
            return 1;
        }
    }
    return CallNextHookEx(keyboard_hook,code,message,data);
}
void toggle_ime(HWND expected) {
    if (!expected || GetForegroundWindow() != expected) {log("skipped: foreground changed");return;}
    const DWORD tid = GetWindowThreadProcessId(expected,nullptr);
    const auto layout = reinterpret_cast<ULONG_PTR>(GetKeyboardLayout(tid));
    if (LOWORD(layout) != MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
        log("skipped: foreground input language is not Japanese");return;
    }
    // Use the IME's own half/full-width toggle. Never switch the input language.
    INPUT keys[2]{};
    for (auto& key : keys) {
        key.type=INPUT_KEYBOARD; key.ki.wVk=VK_KANJI; key.ki.wScan=0x29;
        key.ki.dwExtraInfo=input_tag;
    }
    keys[1].ki.dwFlags=KEYEVENTF_KEYUP;
    const UINT sent=SendInput(2,keys,sizeof(INPUT));
    if (sent==1) SendInput(1,&keys[1],sizeof(INPUT));
    log(sent==2 ? "IME toggle sent" : "error: SendInput failed (check target privilege)");
}
DWORD WINAPI hook_thread(void*) {
    wchar_t path[32768]{};
    GetModuleFileNameW(module_handle,path,32768);
    std::wstring logfile(path);
    logfile=logfile.substr(0,logfile.find_last_of(L"\\/")+1)+L"caps-ime.log";
    log_file=_wfopen(logfile.c_str(),L"a");
    MSG msg{}; PeekMessageW(&msg,nullptr,0,0,PM_NOREMOVE);
    keyboard_hook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboard_callback,module_handle,0);
    startup_error=keyboard_hook ? ERROR_SUCCESS : GetLastError();
    log(keyboard_hook ? "started: injected Pause -> Japanese IME toggle" : "error: keyboard hook installation failed");
    SetEvent(ready_event);
    if (keyboard_hook) {
        bool done=false;
        while(!done) {
            const DWORD wait=MsgWaitForMultipleObjects(1,&stop_event,FALSE,INFINITE,QS_ALLINPUT);
            if(wait==WAIT_OBJECT_0 || wait==WAIT_FAILED) break;
            while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
                if(msg.message==WM_QUIT) {done=true;break;}
                if(msg.message==toggle_message) toggle_ime(reinterpret_cast<HWND>(msg.wParam));
                else {TranslateMessage(&msg);DispatchMessageW(&msg);}
            }
        }
        UnhookWindowsHookEx(keyboard_hook);keyboard_hook=nullptr;
    }
    log("stopped");
    if(log_file) {std::fclose(log_file);log_file=nullptr;}
    return 0;
}
void stop() {
    if(stop_event) SetEvent(stop_event);
    if(thread_handle) {WaitForSingleObject(thread_handle,INFINITE);CloseHandle(thread_handle);thread_handle=nullptr;}
    if(stop_event) {CloseHandle(stop_event);stop_event=nullptr;}
    if(ready_event) {CloseHandle(ready_event);ready_event=nullptr;}
    if(instance_mutex) {CloseHandle(instance_mutex);instance_mutex=nullptr;}
}
DWORD start() {
    if(thread_handle) return ERROR_SUCCESS;
    instance_mutex=CreateMutexW(nullptr,FALSE,L"Local\\SteamVR.CapsIme.v1");
    if(!instance_mutex) return GetLastError();
    if(GetLastError()==ERROR_ALREADY_EXISTS) {stop();return ERROR_ALREADY_EXISTS;}
    stop_event=CreateEventW(nullptr,TRUE,FALSE,L"Local\\SteamVR.CapsIme.Stop.v1");
    ready_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!stop_event || !ready_event) {const auto e=GetLastError();stop();return e;}
    ResetEvent(stop_event); policy=capsime::KeyPolicy{};
    thread_handle=CreateThread(nullptr,0,hook_thread,nullptr,0,&thread_id);
    if(!thread_handle) {const auto e=GetLastError();stop();return e;}
    WaitForSingleObject(ready_event,INFINITE);
    if(startup_error) {const auto e=startup_error;stop();return e;}
    return ERROR_SUCCESS;
}
class Provider final : public vr::IServerTrackedDeviceProvider {
public:
    vr::EVRInitError Init(vr::IVRDriverContext*) override {
        const DWORD error=start();
        // A standalone test host may already own the hook.
        return error==ERROR_SUCCESS || error==ERROR_ALREADY_EXISTS ? vr::VRInitError_None : vr::VRInitError_Driver_Failed;
    }
    void Cleanup() override {stop();}
    const char* const* GetInterfaceVersions() override {return vr::k_InterfaceVersions;}
    void RunFrame() override {}
    bool ShouldBlockStandbyMode() override {return false;}
    void EnterStandby() override {}
    void LeaveStandby() override {}
} provider;
}
extern "C" __declspec(dllexport) void* HmdDriverFactory(const char* version,int* error) {
    if(version && std::strcmp(version,vr::IServerTrackedDeviceProvider_Version)==0) {
        if(error) *error=vr::VRInitError_None;
        return &provider;
    }
    if(error) *error=vr::VRInitError_Init_InterfaceNotFound;
    return nullptr;
}
extern "C" __declspec(dllexport) DWORD CapsImeStart() {return start();}
extern "C" __declspec(dllexport) void CapsImeStop() {stop();}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {module_handle=instance;DisableThreadLibraryCalls(instance);}
    return TRUE;
}
