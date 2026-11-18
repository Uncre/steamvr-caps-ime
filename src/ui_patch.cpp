#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include "ui_patch.h"

namespace capsime {
namespace {
constexpr char marker[]="/* SteamVR Caps IME v3 */";
constexpr char anchor[]="const g=u.s_keyToggleData[t];if(g)";
std::wstring target_path;
std::string adapter;
std::wstring parent(const std::wstring& p) {return p.substr(0,p.find_last_of(L"\\/"));}
bool read_file(const std::wstring& path,std::string& text) {
    HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                         nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(h,&size) && size.QuadPart>=0 && size.QuadPart<=32*1024*1024;
    if(ok) {
        text.resize(static_cast<size_t>(size.QuadPart));DWORD count=0;
        ok=ReadFile(h,text.data(),static_cast<DWORD>(text.size()),&count,nullptr) && count==text.size();
    }
    CloseHandle(h);return ok;
}
bool write_file(const std::wstring& path,const std::string& text) {
    const auto tmp=path+L".capsime.tmp";
    HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return false;
    DWORD count=0;bool ok=WriteFile(h,text.data(),static_cast<DWORD>(text.size()),&count,nullptr) && count==text.size();
    if(ok) ok=FlushFileBuffers(h)!=0;
    CloseHandle(h);
    if(ok) ok=MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok) DeleteFileW(tmp.c_str());
    return ok;
}
bool exactly_once(const std::string& text,const std::string& needle) {
    const auto p=text.find(needle);
    return p!=std::string::npos && text.find(needle,p+needle.size())==std::string::npos;
}
}
bool install_ui_patch(const std::wstring& dll_path,std::string& status) {
    wchar_t steam[32768]{};DWORD bytes=sizeof(steam);
    if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,
                    nullptr,steam,&bytes)!=ERROR_SUCCESS) {status="UI error: SteamPath registry value unavailable";return false;}
    const auto package=parent(parent(parent(dll_path)));
    target_path=std::wstring(steam)+L"\\steamui\\virtualkeyboard.js";
    if(!read_file(package+L"\\caps-adapter.js",adapter) || adapter.find(marker)!=0) {
        status="UI error: missing or invalid caps-adapter.js";return false;
    }
    std::string text;
    if(!read_file(target_path,text)) {status="UI error: cannot read virtualkeyboard.js";return false;}
    if(exactly_once(text,adapter+anchor)) {status="UI adapter already installed";return true;}
    if(text.find("SteamVR Caps IME")!=std::string::npos || !exactly_once(text,anchor) ||
       text.find("var Rt=p(90759)")==std::string::npos ||
       text.find("TypeKeyInternal(e){let{strKey:t")==std::string::npos) {
        status="UI error: unsupported Steam UI version; file left unchanged";return false;
    }
    const auto backups=package+L"\\backups";CreateDirectoryW(backups.c_str(),nullptr);
    const auto backup=backups+L"\\virtualkeyboard.js.native-"+std::to_wstring(GetTickCount64())+L".bak";
    if(!CopyFileW(target_path.c_str(),backup.c_str(),TRUE)) {status="UI error: backup failed; file left unchanged";return false;}
    text.insert(text.find(anchor),adapter);
    if(!write_file(target_path,text)) {status="UI error: cannot write virtualkeyboard.js";return false;}
    status="UI adapter installed for this SteamVR session";return true;
}
bool remove_ui_patch(std::string& status) {
    if(target_path.empty() || adapter.empty()) {status="UI adapter not installed";return true;}
    std::string text;
    if(!read_file(target_path,text)) {status="UI restore error: cannot read virtualkeyboard.js";return false;}
    if(text.find(marker)==std::string::npos) {status="UI adapter already absent";return true;}
    if(!exactly_once(text,adapter+anchor)) {status="UI restore error: adapter changed; file left unchanged";return false;}
    text.erase(text.find(adapter+anchor),adapter.size());
    if(!write_file(target_path,text)) {status="UI restore error: cannot write virtualkeyboard.js";return false;}
    status="UI adapter removed; original Caps behavior restored on disk";return true;
}
}
