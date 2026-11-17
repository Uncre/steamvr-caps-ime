#pragma once
#include <string>
namespace capsime {
bool install_ui_patch(const std::wstring& dll_path, std::string& status);
bool remove_ui_patch(std::string& status);
}
