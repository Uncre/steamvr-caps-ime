#pragma once
#include <windows.h>
#include "input_action.h"

namespace capsime {
struct DesktopInput {
    HWND foreground = nullptr;
    HWND focus = nullptr;
    HKL layout = nullptr;
};

DesktopInput focused_input();
bool toggle_ime(const DesktopInput& target);
bool send_key(const DesktopInput& target, KeyAction action);
}
