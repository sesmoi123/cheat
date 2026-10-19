#include "Features/Aimbot.hpp"

#include <algorithm>
#include <cmath>

#include <imgui.h>

#include "Config.hpp"
#include "Memory.hpp"
#include "Render/Draw.hpp"

namespace aimbot {

namespace {

sdk::Player currentTarget;
bool targetValid = false;

bool key_down(int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

sdk::FVector bone_for_mode(const sdk::Player& player, int mode) {
    switch (mode) {
    case 1:
        return sdk::get_bone(player.mesh, sdk::bones::Chest);
    case 2:
        return sdk::get_bone(player.mesh, sdk::bones::Pelvis);
    default:
        return player.head;
    }
}

} // namespace

void update(const sdk::Camera& camera, const std::vector<sdk::Player>& players) {
    config::Config& config = cfg();
    targetValid = false;

    if (!config.aim_enabled || !camera.valid) {
        return;
    }

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const double screenWidth = display.x;
    const double screenHeight = display.y;
    const double centerX = screenWidth * 0.5;
    const double centerY = screenHeight * 0.5;

    const double radius = sdk::fov_to_pixels(config.aim_fov, camera.fov, screenWidth);
    const int team = sdk::local_team();

    const sdk::Player* best = nullptr;
    double bestScore = radius;

    for (const sdk::Player& player : players) {
        if (!player.valid || player.is_local) {
            continue;
        }
        if (!config.aim_teammates && team > 0 && player.team == team) {
            continue;
        }
        if (player.distance > config.aim_max_distance) {
            continue;
        }
        if (config.aim_visible_only && !player.visible) {
            continue;
        }

        const sdk::FVector aimPoint = bone_for_mode(player, config.aim_bone);
        sdk::FVector2D screen;
        if (!sdk::world_to_screen(aimPoint, camera, screenWidth, screenHeight, screen)) {
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

    currentTarget = *best;
    targetValid = true;

    if (!key_down(config.aim_key)) {
        return;
    }

    const sdk::FVector aimPoint = bone_for_mode(currentTarget, config.aim_bone);
    const sdk::FRotator wanted = sdk::find_look_at_rotation(camera.location, aimPoint);
    const double smooth = std::max(1.0, static_cast<double>(config.aim_smooth));

    if (config.aim_mode == static_cast<int>(config::AimMode::Memory)) {
        const uintptr_t controller = sdk::get_local_controller();
        if (!controller) {
            return;
        }

        const sdk::FRotator current =
            mem::read<sdk::FRotator>(controller + off::get().Controller_ControlRotation);
        const sdk::FRotator delta = sdk::shortest_delta(current, wanted);

        sdk::FRotator next;
        next.Pitch = current.Pitch + delta.Pitch / smooth;
        next.Yaw = current.Yaw + delta.Yaw / smooth;
        next.Roll = 0.0;

        mem::write(controller + off::get().Controller_ControlRotation, next);
    } else {
        const sdk::FRotator delta = sdk::shortest_delta(camera.rotation, wanted);
        const double degreesPerCount = std::max(0.00005, static_cast<double>(config.aim_sensitivity) * 0.022);

        const long moveX = static_cast<long>(std::lround(delta.Yaw / degreesPerCount / smooth));
        const long moveY = static_cast<long>(std::lround(-delta.Pitch / degreesPerCount / smooth));

        if (moveX || moveY) {
            mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(moveX), static_cast<DWORD>(moveY), 0, 0);
        }
    }
}

void draw_fov(const sdk::Camera& camera) {
    config::Config& config = cfg();
    if (!config.aim_enabled || !config.aim_draw_fov || !camera.valid) {
        return;
    }

    ImDrawList* list = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const double radius = sdk::fov_to_pixels(config.aim_fov, camera.fov, display.x);

    const ImU32 color = targetValid ? IM_COL32(255, 255, 255, 200) : IM_COL32(153, 102, 255, 170);
    draw::circle(list, display.x * 0.5f, display.y * 0.5f, static_cast<float>(radius), color, 1.4f, 64);
}

bool has_target() {
    return targetValid;
}

const sdk::Player& target() {
    return currentTarget;
}

} // namespace aimbot
