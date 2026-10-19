#include "Features/BulletTP.hpp"

#include <algorithm>
#include <cmath>

#include <imgui.h>

#include "Config.hpp"
#include "Memory.hpp"

namespace bullet_tp {

namespace {

bool key_down(int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

} // namespace

void update(const sdk::Camera& camera, const std::vector<sdk::Player>& players) {
    config::Config& config = cfg();

    if (!config.bullet_tp || !camera.valid) {
        return;
    }
    if (!config.bullet_tp_always && !key_down(config.bullet_tp_key)) {
        return;
    }

    const uintptr_t pawn = sdk::get_local_pawn();
    if (!pawn) {
        return;
    }

    const uintptr_t weapon = mem::read<uintptr_t>(pawn + off::get().Pawn_CurrentWeapon);
    if (!weapon) {
        return;
    }

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const double screenWidth = display.x;
    const double screenHeight = display.y;
    const double centerX = screenWidth * 0.5;
    const double centerY = screenHeight * 0.5;

    const double radius = sdk::fov_to_pixels(config.bullet_tp_fov, camera.fov, screenWidth);
    const int team = sdk::local_team();

    const sdk::Player* best = nullptr;
    double bestScore = radius;

    for (const sdk::Player& player : players) {
        if (!player.valid || player.is_local) {
            continue;
        }
        if (team > 0 && player.team == team) {
            continue;
        }

        sdk::FVector2D screen;
        if (!sdk::world_to_screen(player.head, camera, screenWidth, screenHeight, screen)) {
            continue;
        }

        const double dx = screen.X - centerX;
        const double dy = screen.Y - centerY;
        const double distance = std::sqrt(dx * dx + dy * dy);

        if (distance < bestScore) {
            bestScore = distance;
            best = &player;
        }
    }

    if (!best) {
        return;
    }

    sdk::FVector spawn = best->head;
    spawn.Z += static_cast<double>(config.bullet_tp_height);

    mem::write(weapon + off::get().Weapon_FireStartLocation, spawn);
    mem::write(weapon + off::get().Weapon_LastFireLocation, spawn);
}

} // namespace bullet_tp
