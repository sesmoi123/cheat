#include "Features/BulletTP.hpp"

#include <windows.h>

#include <cmath>

#include "Config.hpp"
#include "Input/Input.hpp"
#include "Memory.hpp"
#include "SDK/Offsets.hpp"
#include "SDK/SDK.hpp"

namespace features::bullet_tp {

void tick() {
    if (!config::g.bullet_tp || !sdk::alive()) {
        return;
    }

    // Clavier ET manette : Y (bullet_pad_button) passe par input::down.
    if (!config::g.bullet_always &&
        !input::down(config::g.bullet_key, config::g.bullet_pad_button)) {
        return;
    }

    const uintptr_t pawn = sdk::local_pawn();
    if (!pawn) {
        return;
    }

    const uintptr_t weapon = memory::read<uintptr_t>(pawn + offsets::g.Pawn_CurrentWeapon);
    if (!weapon) {
        return;
    }

    // Cible : le joueur le plus proche du reticule.
    const sdk::Player* best = nullptr;
    double bestOffset = config::g.aim_fov;

    for (const sdk::Player& player : sdk::players()) {
        if (player.isLocal) {
            continue;
        }

        const sdk::Vector3 point{player.location.x, player.location.y,
                                 player.location.z +
                                     config::g.esp_head_offset * config::g.bone_chest};

        sdk::Vector2 screen;
        if (!sdk::world_to_screen(point, screen)) {
            continue;
        }

        const double dx = screen.x - sdk::screen_width / 2.0;
        const double dy = screen.y - sdk::screen_height / 2.0;
        const double offset = std::sqrt(dx * dx + dy * dy);

        if (offset < bestOffset) {
            bestOffset = offset;
            best = &player;
        }
    }

    if (!best) {
        return;
    }

    const sdk::Vector3 spawn{best->location.x, best->location.y,
                             best->location.z +
                                 config::g.esp_head_offset * config::g.bone_chest};

    memory::write<sdk::Vector3>(weapon + offsets::g.Weapon_ProjectileSpawn, spawn);
}

} // namespace features::bullet_tp
