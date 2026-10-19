#pragma once

#include <cstdint>

namespace pad {

// Les 16 bits bas correspondent exactement a XINPUT_GAMEPAD_*, LT/RT sont
// des bits synthetiques ajoutes au-dessus.
enum Button : uint32_t {
    DUp = 0x0001, DDown = 0x0002, DLeft = 0x0004, DRight = 0x0008,
    Start = 0x0010, Back = 0x0020, LStick = 0x0040, RStick = 0x0080,
    LB = 0x0100, RB = 0x0200,
    A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000,
    LT = 0x10000, RT = 0x20000,
};

void update();                      // une fois par frame, avant ImGui::NewFrame
bool connected();
uint32_t held();                    // boutons actuellement enfonces
uint32_t pressed();                 // boutons enfonces cette frame
bool down(uint32_t mask);
bool pressed_combo(uint32_t mask);  // front montant du combo complet
void feed_imgui();
void shutdown();

} // namespace pad
