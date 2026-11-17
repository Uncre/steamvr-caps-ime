#pragma once
#include <cstdint>

namespace capsime {
constexpr std::uint32_t marker_key = 0x13; // VK_PAUSE, reserved for the Steam UI adapter.
struct Decision { bool consume; bool toggle; };
class KeyPolicy {
    bool down_ = false;
public:
    Decision process(std::uint32_t key, bool injected, bool released) {
        if (key != marker_key || !injected) return {false, false};
        const bool toggle = !released && !down_;
        down_ = !released;
        return {true, toggle};
    }
};
}
