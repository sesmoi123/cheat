#include "Input/Pad.hpp"

#include <windows.h>
#include <xinput.h>

#include <imgui.h>

namespace pad {

namespace {

uint32_t g_buttons = 0;
uint32_t g_previous = 0;
bool g_connected = false;
XINPUT_STATE g_state{};

// Ordinal 100 = XInputGetStateEx : seul moyen de voir le bouton Guide.
using GetStateExFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
GetStateExFn g_getStateEx = nullptr;

DWORD read_state(XINPUT_STATE& out) {
    if (!g_getStateEx) {
        if (HMODULE module = LoadLibraryW(L"xinput1_4.dll")) {
            g_getStateEx = reinterpret_cast<GetStateExFn>(
                GetProcAddress(module, reinterpret_cast<LPCSTR>(100)));
        }
    }

    if (g_getStateEx) {
        return g_getStateEx(0, &out);
    }
    return XInputGetState(0, &out);
}

} // namespace

void update() {
    g_previous = g_buttons;

    XINPUT_STATE state{};
    if (read_state(state) != ERROR_SUCCESS) {
        g_connected = false;
        g_buttons = 0;
        return;
    }

    g_connected = true;
    g_state = state;

    uint32_t buttons = state.Gamepad.wButtons;
    if (state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        buttons |= LT;
    }
    if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        buttons |= RT;
    }
    g_buttons = buttons;
}

bool connected() {
    return g_connected;
}

uint32_t held() {
    return g_buttons;
}

uint32_t pressed() {
    return g_buttons & ~g_previous;
}

bool down(uint32_t mask) {
    return mask != 0 && (g_buttons & mask) == mask;
}

bool pressed_combo(uint32_t mask) {
    return mask != 0 && (g_buttons & mask) == mask && (g_previous & mask) != mask;
}

void feed_imgui() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    io.NavInputs[ImGuiNavInput_Activate] = (g_buttons & A) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Cancel] = (g_buttons & B) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Menu] = (g_buttons & Y) ? 1.0f : 0.0f;

    io.NavInputs[ImGuiNavInput_DpadUp] = (g_buttons & DUp) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown] = (g_buttons & DDown) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadLeft] = (g_buttons & DLeft) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadRight] = (g_buttons & DRight) ? 1.0f : 0.0f;

    io.NavInputs[ImGuiNavInput_FocusPrev] = (g_buttons & LStick) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_FocusNext] = (g_buttons & RStick) ? 1.0f : 0.0f;

    io.NavInputs[ImGuiNavInput_TweakSlow] = (g_buttons & LB) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_TweakFast] = (g_buttons & RB) ? 1.0f : 0.0f;

    const int lx = g_state.Gamepad.sThumbLX;
    const int ly = g_state.Gamepad.sThumbLY;
    if (lx > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        io.NavInputs[ImGuiNavInput_LStickRight] = 1.0f;
    }
    if (lx < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        io.NavInputs[ImGuiNavInput_LStickLeft] = 1.0f;
    }
    if (ly > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        io.NavInputs[ImGuiNavInput_LStickUp] = 1.0f;
    }
    if (ly < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        io.NavInputs[ImGuiNavInput_LStickDown] = 1.0f;
    }
}

void shutdown() {
    g_buttons = 0;
    g_previous = 0;
    g_connected = false;
}

} // namespace pad
