#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdio>

int wmain(int argc,wchar_t** argv) {
    if(argc==2 && std::wstring(argv[1])==L"--stop") {
        HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,L"Local\\SteamVR.CapsIme.Stop.v1");
        if(!event) return 1;
        SetEvent(event);CloseHandle(event);return 0;
    }
    wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);
    std::wstring dll(path);dll=dll.substr(0,dll.find_last_of(L"\\/")+1)+L"driver_capsime.dll";
    HMODULE module=LoadLibraryW(dll.c_str());if(!module) return 2;
    const auto start=reinterpret_cast<DWORD(*)()>(GetProcAddress(module,"CapsImeStart"));
    const auto stop=reinterpret_cast<void(*)()>(GetProcAddress(module,"CapsImeStop"));
    if(!start || !stop) return 3;
    const DWORD result=start();
    if(result) {FreeLibrary(module);return static_cast<int>(result);}
    HANDLE event=OpenEventW(SYNCHRONIZE,FALSE,L"Local\\SteamVR.CapsIme.Stop.v1");
    if(event) {WaitForSingleObject(event,INFINITE);CloseHandle(event);}
    stop();FreeLibrary(module);return 0;
}
