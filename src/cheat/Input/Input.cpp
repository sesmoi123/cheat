#include "Input/Input.hpp"

#include <windows.h>

#include "Config.hpp"
#include "Input/Pad.hpp"

namespace input {

bool down(int virtualKey, uint32_t padMask) {
    if (config::g.pad_enabled && pad::down(padMask)) {
        return true;
    }
    return virtualKey != 0 && (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

bool pressed(int virtualKey, uint32_t padMask) {
    static bool previous[256]{};

    bool edge = false;
    if (virtualKey > 0 && virtualKey < 256) {
        const bool keyboard = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        edge = keyboard && !previous[virtualKey];
        previous[virtualKey] = keyboard;
    }

    if (config::g.pad_enabled && pad::pressed_combo(padMask)) {
        return true;
    }
    return edge;
}

} // namespace input
