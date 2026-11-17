#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../third_party/openvr/openvr_driver.h"
#pragma GCC diagnostic pop
int wmain(int argc,wchar_t** argv) {
    assert(argc==2);
    HMODULE module=LoadLibraryW(argv[1]);assert(module);
    auto factory=reinterpret_cast<void*(*)(const char*,int*)>(GetProcAddress(module,"HmdDriverFactory"));
    assert(factory && GetProcAddress(module,"CapsImeStart") && GetProcAddress(module,"CapsImeStop"));
    int error=0;assert(!factory("not-a-real-interface",&error));
    assert(error==vr::VRInitError_Init_InterfaceNotFound);
    auto provider=static_cast<vr::IServerTrackedDeviceProvider*>(factory(vr::IServerTrackedDeviceProvider_Version,&error));
    assert(provider && error==vr::VRInitError_None);
    assert(provider->GetInterfaceVersions());assert(!provider->ShouldBlockStandbyMode());
    provider->RunFrame();provider->EnterStandby();provider->LeaveStandby();
    FreeLibrary(module);
    puts("PASS: DLL loads, exports factory, rejects unknown interfaces, provides valid OpenVR vtable");
    return 0;
}
