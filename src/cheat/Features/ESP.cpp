#include "Features/ESP.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "Config.hpp"
#include "Render/Draw.hpp"

namespace esp {

namespace {

constexpr int kSkeletonSegments[][2] = {
    {sdk::bones::Head, sdk::bones::Neck},
    {sdk::bones::Neck, sdk::bones::Chest},
    {sdk::bones::Chest, sdk::bones::Pelvis},

    {sdk::bones::Neck, sdk::bones::LeftShoulder},
    {sdk::bones::LeftShoulder, sdk::bones::LeftElbow},
    {sdk::bones::LeftElbow, sdk::bones::LeftHand},

    {sdk::bones::Neck, sdk::bones::RightShoulder},
    {sdk::bones::RightShoulder, sdk::bones::RightElbow},
    {sdk::bones::RightElbow, sdk::bones::RightHand},

    {sdk::bones::Pelvis, sdk::bones::LeftThigh},
    {sdk::bones::LeftThigh, sdk::bones::LeftKnee},
    {sdk::bones::LeftKnee, sdk::bones::LeftFoot},

    {sdk::bones::Pelvis, sdk::bones::RightThigh},
    {sdk::bones::RightThigh, sdk::bones::RightKnee},
    {sdk::bones::RightKnee, sdk::bones::RightFoot},
};

void draw_skeleton(ImDrawList* list, const sdk::Player& player, const sdk::Camera& camera,
                   double screenWidth, double screenHeight, ImU32 color, float thickness) {
    for (const auto& segment : kSkeletonSegments) {
        const sdk::FVector from = sdk::get_bone(player.mesh, segment[0]);
        const sdk::FVector to = sdk::get_bone(player.mesh, segment[1]);
        if (from.empty() || to.empty()) {
            continue;
        }

        sdk::FVector2D fromScreen;
        sdk::FVector2D toScreen;
        if (!sdk::world_to_screen(from, camera, screenWidth, screenHeight, fromScreen)) {
            continue;
        }
        if (!sdk::world_to_screen(to, camera, screenWidth, screenHeight, toScreen)) {
            continue;
        }

        draw::line(list, static_cast<float>(fromScreen.X), static_cast<float>(fromScreen.Y),
                   static_cast<float>(toScreen.X), static_cast<float>(toScreen.Y), color, thickness);
    }
}

} // namespace

void draw(const sdk::Camera& camera, const std::vector<sdk::Player>& players) {
    config::Config& config = cfg();
    if (!config.esp_enabled || !camera.valid) {
        return;
    }

    ImDrawList* list = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const double screenWidth = display.x;
    const double screenHeight = display.y;

    const int team = sdk::local_team();
    const float thickness = config.esp_thickness;

    for (const sdk::Player& player : players) {
        if (!player.valid) {
            continue;
        }
        if (player.is_local && !config.esp_local) {
            continue;
        }

        const bool teammate = !player.is_local && team > 0 && player.team == team;
        if (teammate && !config.esp_teammates) {
            continue;
        }
        if (player.distance > config.esp_max_distance) {
            continue;
        }

        sdk::FVector2D headScreen;
        sdk::FVector2D feetScreen;
        if (!sdk::world_to_screen(player.head, camera, screenWidth, screenHeight, headScreen)) {
            continue;
        }
        if (!sdk::world_to_screen(player.feet, camera, screenWidth, screenHeight, feetScreen)) {
            continue;
        }

        const double boxHeight = feetScreen.Y - headScreen.Y;
        if (boxHeight < 4.0) {
            continue;
        }

        const double boxWidth = boxHeight * 0.55;
        const double boxX = headScreen.X - boxWidth * 0.5;
        const double boxY = headScreen.Y;

        const ImVec4 baseColor = teammate ? config.color_teammate : config.color_box;
        const ImU32 boxColor = draw::to_color(baseColor);
        const ImU32 skeletonColor = draw::to_color(config.color_skeleton);
        const ImU32 nameColor = draw::to_color(config.color_name);
        const ImU32 distanceColor = draw::to_color(config.color_distance);

        if (config.esp_snapline) {
            draw::line(list, static_cast<float>(screenWidth * 0.5), static_cast<float>(screenHeight),
                       static_cast<float>(feetScreen.X), static_cast<float>(feetScreen.Y),
                       draw::to_color(config.color_snapline), thickness);
        }

        if (config.esp_box) {
            draw::rect(list, static_cast<float>(boxX), static_cast<float>(boxY),
                       static_cast<float>(boxWidth), static_cast<float>(boxHeight), boxColor, thickness, 1.0f);
        }

        if (config.esp_skeleton) {
            draw_skeleton(list, player, camera, screenWidth, screenHeight, skeletonColor, thickness);
        }

        if (config.esp_health) {
            const double barX = boxX - 6.0;
            const double ratio = std::clamp(static_cast<double>(player.health) / 100.0, 0.0, 1.0);

            draw::filled_rect(list, static_cast<float>(barX), static_cast<float>(boxY), 3.0f,
                              static_cast<float>(boxHeight), IM_COL32(0, 0, 0, 180), 1.0f);
            draw::filled_rect(list, static_cast<float>(barX),
                              static_cast<float>(boxY + boxHeight * (1.0 - ratio)), 3.0f,
                              static_cast<float>(boxHeight * ratio), IM_COL32(70, 230, 120, 255), 1.0f);
        }

        char label[160];

        if (config.esp_name) {
            const std::string name = player.name.empty() ? std::string("joueur") : player.name;
            draw::text(list, static_cast<float>(headScreen.X), static_cast<float>(boxY - 18.0f),
                       nameColor, name.c_str(), true, true);
        }

        if (config.esp_distance) {
            snprintf(label, sizeof(label), "%.0f m", player.distance);
            draw::text(list, static_cast<float>(headScreen.X),
                       static_cast<float>(boxY + boxHeight + 3.0f), distanceColor, label, true, true);
        }

        if (config.esp_knocked && player.knocked) {
            draw::text(list, static_cast<float>(boxX + boxWidth + 6.0f), static_cast<float>(boxY),
                       IM_COL32(255, 200, 60, 255), "DBNO", false, true);
        }
    }
}

} // namespace esp
