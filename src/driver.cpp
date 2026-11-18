#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "desktop_input.h"
#include "http_policy.h"
#include "ui_patch.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../third_party/openvr/openvr_driver.h"
#pragma GCC diagnostic pop

namespace {
HMODULE module_handle{};
HANDLE thread_handle{}, stop_event{}, ready_event{}, instance_mutex{};
DWORD thread_id{}, startup_error{};
FILE* log_file{};

void log(const char* text) {
    if (!log_file) return;
    SYSTEMTIME t{}; GetLocalTime(&t);
    std::fprintf(log_file,"%04u-%02u-%02u %02u:%02u:%02u %s\n",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,text);
    std::fflush(log_file);
}
void log_toggle(const capsime::DesktopInput& target, bool sent, bool cached_adapter) {
    DWORD pid = 0;
    GetWindowThreadProcessId(target.focus, &pid);
    char path[2048]{};
    const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process) {
        DWORD size = sizeof(path);
        QueryFullProcessImageNameA(process, 0, path, &size);
        CloseHandle(process);
    }
    const char* name = std::strrchr(path, '\\');
    char message[256]{};
    std::snprintf(message, sizeof(message), "Caps: %s app=%.120s layout=%04x scan29=%02x ui=%s",
        sent ? "scan sent" : "not sent", name ? name + 1 : "(unavailable)",
        LOWORD(reinterpret_cast<ULONG_PTR>(target.layout)),
        target.layout ? MapVirtualKeyExW(0x29, MAPVK_VSC_TO_VK_EX, target.layout) : 0,
        cached_adapter ? "cached (reload Steam UI)" : "3");
    log(message);
}
void log_conversion_key(capsime::Key key, bool sent) {
    const char* name = nullptr;
    switch (key) {
        case capsime::Key::space: name = "Space"; break;
        case capsime::Key::tab: name = "Tab"; break;
        case capsime::Key::enter: name = "Enter"; break;
        default: return;
    }
    char message[64]{};
    std::snprintf(message, sizeof(message), "%s: %s", name, sent ? "scan sent" : "not sent");
    log(message);
}
void respond(SOCKET client,const char* status,const char* body,bool cors) {
    std::string response="HTTP/1.1 "+std::string(status)+"\r\nConnection: close\r\nContent-Type: application/json\r\n";
    if(cors) response+="Access-Control-Allow-Origin: https://steamloopback.host\r\nVary: Origin\r\nAccess-Control-Allow-Methods: POST, OPTIONS\r\nAccess-Control-Allow-Headers: X-SteamVR-Caps-IME\r\nAccess-Control-Allow-Private-Network: true\r\n";
    response+="Content-Length: "+std::to_string(std::strlen(body))+"\r\n\r\n"+body;
    send(client,response.data(),static_cast<int>(response.size()),0);
}
void serve(SOCKET server) {
    while(WaitForSingleObject(stop_event,0)==WAIT_TIMEOUT) {
        fd_set readable;FD_ZERO(&readable);FD_SET(server,&readable);
        timeval timeout{0,100000};
        const int ready=select(0,&readable,nullptr,nullptr,&timeout);
        if(ready==SOCKET_ERROR) break;
        if(!ready) continue;
        SOCKET client=accept(server,nullptr,nullptr);if(client==INVALID_SOCKET) continue;
        DWORD limit=500;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&limit),sizeof(limit));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&limit),sizeof(limit));
        std::string request;char buffer[2048];
        while(request.size()<8192 && request.find("\r\n\r\n")==std::string::npos) {
            const int bytes=recv(client,buffer,sizeof(buffer),0);if(bytes<=0) break;
            request.append(buffer,static_cast<size_t>(bytes));
        }
        capsime::KeyAction action;
        const auto command = capsime::classify_request(request, &action);
        switch(command) {
            case capsime::Request::health: respond(client,"200 OK","{\"ready\":true,\"uiProtocol\":3}",false);break;
            case capsime::Request::preflight: respond(client,"204 No Content","",true);break;
            case capsime::Request::toggle:
            case capsime::Request::cached_toggle:
            case capsime::Request::key: {
                const auto target = capsime::focused_input();
                const bool is_toggle = command != capsime::Request::key;
                const bool sent = is_toggle ? capsime::toggle_ime(target) : capsime::send_key(target, action);
                if (is_toggle) log_toggle(target, sent, command == capsime::Request::cached_toggle);
                else log_conversion_key(action.key, sent);
                if(sent) respond(client,"200 OK","{\"ok\":true}",true);
                else respond(client,"409 Conflict","{\"ok\":false}",true);
                break;
            }
            default: respond(client,"403 Forbidden","{\"ok\":false}",false);break;
        }
        closesocket(client);
    }
}
DWORD WINAPI bridge_thread(void*) {
    wchar_t path[32768]{};
    GetModuleFileNameW(module_handle,path,32768);
    std::wstring logfile(path);
    logfile=logfile.substr(0,logfile.find_last_of(L"\\/")+1)+L"caps-ime.log";
    log_file=_wfopen(logfile.c_str(),L"a");
    WSADATA wsa{};startup_error=WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET server=INVALID_SOCKET;
    if(!startup_error) {
        server=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(server==INVALID_SOCKET) startup_error=WSAGetLastError();
        else {
            BOOL exclusive=TRUE;
            setsockopt(server,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));
            sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(27391);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
            if(bind(server,reinterpret_cast<sockaddr*>(&address),sizeof(address)) || listen(server,4)) startup_error=WSAGetLastError();
        }
    }
    std::string patch_status;bool patched=false;
    if(!startup_error) {
        patched=capsime::install_ui_patch(path,patch_status);log(patch_status.c_str());
        if(!patched) startup_error=ERROR_INVALID_DATA;
    }
    log(startup_error ? "error: local bridge startup failed" : "started: loopback bridge on 127.0.0.1:27391");
    SetEvent(ready_event);
    if(!startup_error) serve(server);
    if(server!=INVALID_SOCKET) closesocket(server);
    WSACleanup();
    if(patched) {capsime::remove_ui_patch(patch_status);log(patch_status.c_str());}
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
    ResetEvent(stop_event);
    thread_handle=CreateThread(nullptr,0,bridge_thread,nullptr,0,&thread_id);
    if(!thread_handle) {const auto e=GetLastError();stop();return e;}
    WaitForSingleObject(ready_event,INFINITE);
    if(startup_error) {const auto e=startup_error;stop();return e;}
    return ERROR_SUCCESS;
}
class Provider final : public vr::IServerTrackedDeviceProvider {
public:
    vr::EVRInitError Init(vr::IVRDriverContext*) override {
        const DWORD error=start();
        // A standalone test host may already own the local bridge.
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
