#pragma once
#include <string_view>

namespace capsime {
enum class Key { none, ascii, space, tab, enter, backspace, del, escape, left, right, up, down };
struct KeyAction {
    Key key = Key::none;
    char character = 0;
    bool shift = false;
};

inline KeyAction parse_key(std::string_view name) {
    if (name.size() == 8 && name.substr(0, 6) == "ascii-") {
        const auto hex = [](char c) -> int {
            return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        };
        const int high = hex(name[6]), low = hex(name[7]);
        if (high < 0 || low < 0) return {};
        const int value = high * 16 + low;
        if (value < 0x21 || value > 0x7e) return {};
        return {Key::ascii, static_cast<char>(value)};
    }
    const bool shift = name.size() > 6 && name.substr(name.size() - 6) == "-shift";
    if (shift) name.remove_suffix(6);
    constexpr struct { std::string_view name; Key key; } named[] = {
        {"space", Key::space}, {"tab", Key::tab}, {"enter", Key::enter},
        {"backspace", Key::backspace}, {"delete", Key::del}, {"escape", Key::escape},
        {"left", Key::left}, {"right", Key::right}, {"up", Key::up}, {"down", Key::down}
    };
    for (const auto& entry : named) {
        if (entry.name == name) return {entry.key, 0, shift};
    }
    return {};
}
}
