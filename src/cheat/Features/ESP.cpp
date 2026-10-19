#include "Features/ESP.hpp"

#include "Config.hpp"
#include "Render/Draw.hpp"
#include "SDK/SDK.hpp"

#include <imgui.h>

#include <cstdio>

namespace features::esp {

namespace {

// Chaine approximative du squelette humain Fortnite (indices de bones).
struct BoneLink {
    int from;
    int to;
};

const BoneLink kSkeleton[] = {
    {0, 1},   {1, 2},   {2, 3},   {3, 4},   {4, 5},   {5, 6},   {6, 7},
    {5, 8},   {8, 9},   {9, 10},  {10, 11}, {5, 12},  {12, 13}, {13, 14},
    {14, 15}, {0, 16},  {16, 17}, {17, 18}, {0, 19},  {19, 20}, {20, 21},
};

} // namespace

void tick() {
    if (!config::g.esp_enabled || !sdk::alive()) {
        return;
    }

    const ImU32 color = draw::rgba(config::g.esp_color);
    const ImU32 teamColor = draw::rgba(config::g.esp_team_color);
    const int localTeam = sdk::local_team();

    for (const sdk::Player& player : sdk::players()) {
        if (player.isLocal || player.distance > config::g.esp_max_distance) {
            continue;
        }

        const bool teammate = player.team == localTeam && localTeam != 0;
        if (config::g.esp_team_filter && teammate) {
            continue;
        }

        const ImU32 playerColor = teammate ? teamColor : color;

        const sdk::Vector3 head = {player.location.x, player.location.y,
                                   player.location.z + config::g.esp_head_offset};
        const sdk::Vector3 feet = {player.location.x, player.location.y,
                                   player.location.z - config::g.esp_head_offset};

        sdk::Vector2 headScreen;
        sdk::Vector2 feetScreen;
        if (!sdk::world_to_screen(head, headScreen) || !sdk::world_to_screen(feet, feetScreen)) {
            continue;
        }

        const float height = feetScreen.y - headScreen.y;
        const float width = height * 0.55f;
        const float x = headScreen.x - width / 2.0f;

        if (config::g.esp_box) {
            draw::box(x, headScreen.y, width, height, playerColor, 1.5f);
        }

        if (config::g.esp_snapline) {
            draw::line(static_cast<float>(sdk::screen_width) / 2.0f,
                       static_cast<float>(sdk::screen_height), headScreen.x, feetScreen.y,
                       playerColor, 1.0f);
        }

        if (config::g.esp_skeleton) {
            for (const BoneLink& link : kSkeleton) {
                sdk::Vector3 from;
                sdk::Vector3 to;
                if (!sdk::bone_position(player.mesh, link.from, from) ||
                    !sdk::bone_position(player.mesh, link.to, to)) {
                    continue;
                }

                sdk::Vector2 fromScreen;
                sdk::Vector2 toScreen;
                if (!sdk::world_to_screen(from, fromScreen) ||
                    !sdk::world_to_screen(to, toScreen)) {
                    continue;
                }

                draw::line(fromScreen.x, fromScreen.y, toScreen.x, toScreen.y, playerColor, 1.2f);
            }
        }

        if (config::g.esp_health) {
            const float ratio = player.health / 100.0f;
            draw::filled_box(x - 6.0f, headScreen.y, 3.0f, height, IM_COL32(0, 0, 0, 180));
            draw::filled_box(x - 6.0f, headScreen.y + height * (1.0f - ratio), 3.0f, height * ratio,
                             IM_COL32(80, 230, 120, 220));
        }

        float textY = headScreen.y - 16.0f;

        if (config::g.esp_name && !player.name.empty()) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%ls", player.name.substr(0, 20).c_str());
            draw::outlined_text(headScreen.x, textY, buffer, playerColor, true);
            textY -= 14.0f;
        }

        if (config::g.esp_distance) {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.0fm", player.distance);
            draw::outlined_text(headScreen.x, feetScreen.y + 2.0f, buffer, playerColor, true);
        }

        if (player.isDBNO) {
            draw::outlined_text(headScreen.x, textY - 14.0f, "DBNO", IM_COL32(255, 180, 60, 255),
                                true);
        }
    }
}

} // namespace features::esp
