#include "Features/Aimbot.hpp"

#include <windows.h>

#include <cmath>

#include "Config.hpp"
#include "Input/Input.hpp"
#include "Memory.hpp"
#include "SDK/Offsets.hpp"
#include "SDK/SDK.hpp"

namespace features::aimbot {

namespace {

double normalize(double angle) {
    while (angle > 180.0) {
        angle -= 360.0;
    }
    while (angle < -180.0) {
        angle += 360.0;
    }
    return angle;
}

sdk::Vector3 aim_point(const sdk::Player& player) {
    const double ratio = config::g.aim_bone == 0   ? config::g.bone_head
                         : config::g.aim_bone == 1 ? config::g.bone_chest
                                                   : config::g.bone_pelvis;
    return {player.location.x, player.location.y,
            player.location.z + config::g.esp_head_offset * ratio};
}

sdk::Rotator rotation_to(const sdk::Vector3& from, const sdk::Vector3& to) {
    const sdk::Vector3 delta = sdk::subtract(to, from);
    const double horizontal = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    sdk::Rotator rotation;
    rotation.pitch = sdk::rad2deg(std::atan2(delta.z, horizontal));
    rotation.yaw = sdk::rad2deg(std::atan2(delta.y, delta.x));
    rotation.roll = 0.0;
    return rotation;
}

} // namespace

void tick() {
    if (!config::g.aim_enabled || !sdk::alive()) {
        return;
    }

    // Clavier ET manette : LT (aim_pad_button) passe par input::down.
    if (config::g.aim_hold && !input::down(config::g.aim_key, config::g.aim_pad_button)) {
        return;
    }

    const sdk::Player* best = nullptr;
    double bestOffset = config::g.aim_fov;
    const int localTeam = sdk::local_team();

    for (const sdk::Player& player : sdk::players()) {
        if (player.isLocal || player.isDBNO) {
            continue;
        }
        if (player.team == localTeam && localTeam != 0) {
            continue;
        }

        sdk::Vector2 screen;
        if (!sdk::world_to_screen(aim_point(player), screen)) {
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

    const sdk::Vector3 target = aim_point(*best);
    const double step = config::g.aim_smooth <= 0.0f ? 1.0 : 1.0 / config::g.aim_smooth;

    if (config::g.aim_memory) {
        const uintptr_t controller = sdk::local_controller();
        if (!controller) {
            return;
        }

        const uintptr_t address = controller + offsets::g.PlayerController_ControlRotation;
        const sdk::Rotator current = memory::read<sdk::Rotator>(address);
        const sdk::Rotator wanted = rotation_to(sdk::camera_location(), target);

        sdk::Rotator next;
        next.pitch = current.pitch + normalize(wanted.pitch - current.pitch) * step;
        next.yaw = current.yaw + normalize(wanted.yaw - current.yaw) * step;
        next.roll = 0.0;

        memory::write<sdk::Rotator>(address, next);
    } else {
        sdk::Vector2 screen;
        if (!sdk::world_to_screen(target, screen)) {
            return;
        }

        INPUT inputEvent{};
        inputEvent.type = INPUT_MOUSE;
        inputEvent.mi.dx = static_cast<LONG>((screen.x - sdk::screen_width / 2.0) * step);
        inputEvent.mi.dy = static_cast<LONG>((screen.y - sdk::screen_height / 2.0) * step);
        inputEvent.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &inputEvent, sizeof(inputEvent));
    }
}

} // namespace features::aimbot
