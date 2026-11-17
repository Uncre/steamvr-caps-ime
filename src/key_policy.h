#pragma once
#include <cstdint>

namespace capsime {
constexpr std::uint32_t marker_key = 0xe7; // VK_PACKET
constexpr std::uint32_t marker_character = 0xe0fe; // Private-use character, consumed before WM_CHAR.
struct Decision { bool consume; bool toggle; };
class KeyPolicy {
    bool down_ = false;
public:
    Decision process(std::uint32_t key, std::uint32_t scan, bool injected, bool released) {
        if (key != marker_key || scan != marker_character || !injected) return {false, false};
        const bool toggle = !released && !down_;
        down_ = !released;
        return {true, toggle};
    }
};
}
