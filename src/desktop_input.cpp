#define WIN32_LEAN_AND_MEAN
#include "desktop_input.h"
#include <array>

namespace capsime {
DesktopInput focused_input() {
    const HWND foreground = GetForegroundWindow();
    if (!foreground) return {};
    const DWORD thread = GetWindowThreadProcessId(foreground, nullptr);
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    const HWND focus = GetGUIThreadInfo(thread, &info) && info.hwndFocus ? info.hwndFocus : foreground;
    const DWORD focus_thread = GetWindowThreadProcessId(focus, nullptr);
    return {foreground, focus, focus_thread ? GetKeyboardLayout(focus_thread) : nullptr};
}

namespace {
constexpr ULONG_PTR input_tag = 0x53564349;
bool held(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }

INPUT physical_scan(UINT scan, bool released) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = LOBYTE(scan);
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (HIBYTE(scan) == 0xe0 ? KEYEVENTF_EXTENDEDKEY : 0)
        | (released ? KEYEVENTF_KEYUP : 0);
    input.ki.dwExtraInfo = input_tag;
    return input;
}

bool submit(const DesktopInput& target, INPUT* inputs, UINT count) {
    const auto current = focused_input();
    if (!target.layout || current.foreground != target.foreground ||
        current.focus != target.focus || current.layout != target.layout) return false;
    const UINT sent = SendInput(count, inputs, sizeof(INPUT));
    if (sent && sent < count) {
        for (UINT i = sent; i < count; ++i) {
            if (inputs[i].ki.dwFlags & KEYEVENTF_KEYUP) SendInput(1, &inputs[i], sizeof(INPUT));
        }
    }
    return sent == count;
}

bool tap(const DesktopInput& target, UINT scan, UINT modifier_scan = 0) {
    if (!scan) return false;
    if (!modifier_scan) {
        INPUT keys[] = {physical_scan(scan, false), physical_scan(scan, true)};
        return submit(target, keys, 2);
    }
    INPUT keys[] = {physical_scan(modifier_scan, false), physical_scan(scan, false),
        physical_scan(scan, true), physical_scan(modifier_scan, true)};
    return submit(target, keys, 4);
}

UINT virtual_key(Key key) {
    switch (key) {
        case Key::space: return VK_SPACE;
        case Key::tab: return VK_TAB;
        case Key::enter: return VK_RETURN;
        case Key::backspace: return VK_BACK;
        case Key::del: return VK_DELETE;
        case Key::escape: return VK_ESCAPE;
        case Key::left: return VK_LEFT;
        case Key::right: return VK_RIGHT;
        case Key::up: return VK_UP;
        case Key::down: return VK_DOWN;
        default: return 0;
    }
}
}

bool toggle_ime(const DesktopInput& target) {
    if (!target.layout || LOWORD(reinterpret_cast<ULONG_PTR>(target.layout)) != 0x0411 ||
        held(VK_CONTROL) || held(VK_MENU) || held(VK_LWIN) || held(VK_RWIN)) return false;

    // The JIS scan goes through Windows' NLS keyboard tables, which turn it into
    // the IME's open/close action. Injecting VK_KANJI directly skips that mapping.
    constexpr UINT half_full_scan = 0x29;
    const UINT mapped = MapVirtualKeyExW(half_full_scan, MAPVK_VSC_TO_VK_EX, target.layout);
    if (mapped == 0xf3 || mapped == 0xf4 || mapped == VK_KANJI) return tap(target, half_full_scan);
    // Microsoft IME uses Alt+backquote for an English physical layout.
    if (mapped == VK_OEM_3 && !held(VK_SHIFT)) return tap(target, half_full_scan, 0x38);
    return false;
}

bool send_key(const DesktopInput& target, KeyAction action) {
    if (!target.layout) return false;
    UINT vk = virtual_key(action.key);
    bool shift = action.shift;
    if (action.key == Key::ascii) {
        const SHORT mapping = VkKeyScanExW(static_cast<unsigned char>(action.character), target.layout);
        if (mapping == -1 || (HIBYTE(mapping) & ~1) != 0) {
            // Some layouts cannot type all ASCII symbols with a single key.
            INPUT keys[2]{};
            for (auto& input : keys) {
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = static_cast<unsigned char>(action.character);
                input.ki.dwFlags = KEYEVENTF_UNICODE;
                input.ki.dwExtraInfo = input_tag;
            }
            keys[1].ki.dwFlags |= KEYEVENTF_KEYUP;
            return submit(target, keys, 2);
        }
        vk = LOBYTE(mapping);
        shift = (HIBYTE(mapping) & 1) != 0;
    }
    const UINT scan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX, target.layout);
    return tap(target, scan, shift && !held(VK_SHIFT) ? 0x2a : 0);
}
}
